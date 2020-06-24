#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <mqueue.h>
#include <pthread.h>
#include <stddef.h>

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

typedef struct myMessage {
	uint8_t digit1;
	uint8_t digit2;
	pid_t ProcessID;
} Message;

typedef struct threadstruct {
	mqd_t mqdes;
	char operation;
	char* Qname;
} threadstruct_t;

// void sethandler( void (*f)(int, siginfo_t*, void*), int sigNo) {
// 	struct sigaction act;
// 	memset(&act, 0, sizeof(struct sigaction));
// 	act.sa_sigaction = f;
// 	act.sa_flags=SA_SIGINFO;
// 	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
// }
void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sig_handler(int sig) {
	last_signal = sig;
}


char* build_name(const char* sufix){
    char* name;
    int length = snprintf(NULL,0,"%d",getpid());
    if((name =(char*)malloc(sizeof(char)*(length+2+strlen(sufix))))==NULL) ERR("malloc");
    snprintf(name, length+1, "%d",getpid());

    size_t len = strlen("/"); //prepend 
    memmove(name + len, name, strlen(name) + 1);
    memcpy(name, "/", len);

    strcat(name,sufix);
    return name;
}

char* build_client_name(pid_t client){
    char* name;
    int length = snprintf(NULL,0,"%d",client);
    if((name =(char*)malloc(sizeof(char)*(length+2)))==NULL) ERR("malloc");
    snprintf(name, length+1, "%d",client);

    size_t len = strlen("/"); //prepend 
    memmove(name + len, name, strlen(name) + 1);
    memcpy(name, "/", len);
    return name;
}

void server_work(__sigval_t arg) {
    threadstruct_t* tArr = (threadstruct_t *)arg.sival_ptr;
	size_t length = sizeof(Message);
	Message buff;
	uint8_t ni;
	if(TEMP_FAILURE_RETRY(mq_receive(tArr->mqdes,(char*)&buff,length,NULL))<1)ERR("mq_receive");
	printf("Server Received %d, %d, client PID: [%d]\n",buff.digit1, buff.digit2, buff.ProcessID);
	
	if(tArr->operation=='+') ni=buff.digit1+buff.digit2;
	if(tArr->operation=='/') ni=buff.digit1+buff.digit2;
	if(tArr->operation=='%') ni=buff.digit1%buff.digit2;
	
	char *clientname = build_client_name(buff.ProcessID);
	printf("%s\n", clientname);
	mqd_t pout;
	struct mq_attr attr;
	attr.mq_maxmsg=10;
	attr.mq_msgsize=1;
	if((pout=TEMP_FAILURE_RETRY(mq_open(clientname, O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) ERR("mq open client");

	if(TEMP_FAILURE_RETRY(mq_send(pout,(const char*)&ni,1,1)))ERR("mq_send");
	mq_close(pout);
}


void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"100 >n > 0 - number of children\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	int n; 
    char *sum_name = build_name("_s");
    char *div_name = build_name("_d");
    char *mod_name = build_name("_m");

	if(argc!=1) usage();

	size_t length = sizeof(Message);
	mqd_t psum,pdiv,pmod;
	struct mq_attr attr;
	attr.mq_maxmsg=length+1;
	attr.mq_msgsize=length;
	
	if((psum=TEMP_FAILURE_RETRY(mq_open(sum_name, O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) ERR("mq open sum");
	if((pdiv=TEMP_FAILURE_RETRY(mq_open(div_name, O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) ERR("mq open div");
	if((pmod=TEMP_FAILURE_RETRY(mq_open(mod_name, O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) ERR("mq open mod");

    printf("%s\n",sum_name);
    printf("%s\n",div_name);
    printf("%s\n",mod_name);

	sethandler(sig_handler,SIGINT);
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, NULL);

	threadstruct_t* tArr = (threadstruct_t*) malloc(sizeof(threadstruct_t) * 3);
	if (tArr == NULL) ERR("Malloc error");
	tArr[0].Qname = sum_name; tArr[0].mqdes = psum; tArr[0].operation = '+';
	tArr[1].Qname = div_name; tArr[1].mqdes = pdiv; tArr[1].operation = '/';
	tArr[2].Qname = mod_name; tArr[2].mqdes = pmod; tArr[2].operation = '%';
	
	struct sigevent *events = (struct sigevent*)malloc(sizeof(struct sigevent)*3);
	if(events==NULL) ERR("Malloc error");
	for(int i =0; i<3; i++) {
		events[i].sigev_notify=SIGEV_THREAD;
		events[i].sigev_notify_function = server_work;
		events[i].sigev_value.sival_ptr=&tArr[i];
		//if(mq_notify(tArr[i].mqdes, &events[i])<0)ERR("mq_notify");
	}
	//if(mq_notify(psum, &not)<0)ERR("mq_notify");
	int sig;
	while(1) {
		sigwait(&mask, &sig);
		if(sig == SIGINT) break;
	}
	printf("[SERVER] Terminates \n");

	mq_close(psum);
	mq_close(pdiv);
    mq_close(pmod);
	if(mq_unlink(sum_name))ERR("mq unlink");
	if(mq_unlink(div_name))ERR("mq unlink");
	if(mq_unlink(mod_name))ERR("mq unlink");
	free(events);
	free(tArr);
    printf("Queues destroyed\n");
	return EXIT_SUCCESS;
}