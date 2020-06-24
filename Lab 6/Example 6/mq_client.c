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

#define LIFE_SPAN 10
#define MAX_NUM 10

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

typedef struct myMessage {
	uint8_t digit1;
	uint8_t digit2;
	pid_t ProcessID;
} Message;

char* build_name(){
    char* name;
    int length = snprintf(NULL,0,"%d",getpid());
    if((name =(char*)malloc(sizeof(char)*(length+2)))==NULL) ERR("malloc");
    snprintf(name, length+1, "%d",getpid());

    size_t len = strlen("/"); //prepend 
    memmove(name + len, name, strlen(name) + 1);
    memcpy(name, "/", len);
    return name;
}

int client_work(mqd_t pout, char* message, mqd_t pin){
    //sleep(1);
	char *endptr;
	uint8_t ni;
	Message buff;
	size_t length = sizeof(Message);
	buff.ProcessID = getpid();
	buff.digit1=strtol(message,&endptr,10);
	buff.digit2=strtol(endptr,&endptr,10);
	
	printf("%d %d [%d]\n", buff.digit1, buff.digit2, buff.ProcessID);
	if(TEMP_FAILURE_RETRY(mq_send(pout,(const char*)&buff,length,1)))ERR("mq_send");
	
	struct timespec time;
	time.tv_sec = 0;
	time.tv_nsec = 100000000;
	printf("d\n");
	if(TEMP_FAILURE_RETRY(mq_receive(pin,(char*)&ni,10,NULL))<1)ERR("mq_receive");
	//if(errno==ETIMEDOUT) return -1;
	printf("Client Received result %d\n",ni);
	return 0;
}



void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"100 >n > 0 - number of children\n");
	exit(EXIT_FAILURE);
}
int main(int argc, char** argv) {
	int n; 
	char *name = build_name();
    
	if(argc!=2) usage();
	if(argv[1][0] != '/') usage();

	size_t length = sizeof(Message);
	mqd_t pout, pin;
	struct mq_attr attr;
	attr.mq_maxmsg=length+1;
	attr.mq_msgsize=length;
	if((pout=TEMP_FAILURE_RETRY(mq_open(argv[1], O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) ERR("mq open out");
	struct mq_attr myattr;
	myattr.mq_maxmsg=10;
	myattr.mq_msgsize=1;
	if((pin=TEMP_FAILURE_RETRY(mq_open(name, O_RDWR | O_CREAT, 0600, &myattr)))==(mqd_t)-1) ERR("mq open out");

	printf("%s\n", name);
    printf("%s\n",argv[1]);

	// sethandler(sigchld_handler,SIGCHLD);
	// sethandler(mq_handler,SIGRTMIN);
	// create_children(n,pin,pout);

	static struct sigevent not;
	not.sigev_notify=SIGEV_SIGNAL;
	not.sigev_signo=SIGRTMIN;
	not.sigev_value.sival_ptr=&pin;
	//if(mq_notify(pin, &not)<0)ERR("mq_notify");

	char *line = NULL;
    char *endptr;
    ssize_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, stdin)) != -1) {
        printf("Retrieved line of length %zu: ", read);
        printf("%s\n", line);
		int test = client_work(pout,line, pin);
		printf("%d\n",test);
		if(test==-1) break;
    }

	printf("Client terminates [%d]\n",getpid());

	mq_close(pout);
	mq_close(pin);
	if(mq_unlink(argv[1])) ERR("mq unlink");
	if(mq_unlink(name)) ERR("mq_unlink");
    printf("Queue destroyed\n");
	return EXIT_SUCCESS;
}