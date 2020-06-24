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

void sethandler( void (*f)(int, siginfo_t*, void*), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_sigaction = f;
	act.sa_flags=SA_SIGINFO;
	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sigint_handler(int sig, siginfo_t *s, void *p) {
	mq_close(pq);
	if(mq_unlink(mq_name))ERR("mq unlink");
    printf("Queues destroyed\n");
}


char* build_name(){
    char* name;
    int length = snprintf(NULL,0,"%d",getpid());
    if((name =(char*)malloc(sizeof(char)*(length+1+strlen("/mq_"))))==NULL) ERR("malloc");
    snprintf(name, length+1, "%d",getpid());

    size_t len = strlen("/mq_"); //prepend 
    memmove(name + len, name, strlen(name) + 1);
    memcpy(name, "/mq_", len);

    return name;
}


void usage(void){
	fprintf(stderr,"USAGE: signals n k p l\n");
	fprintf(stderr,"100 >n > 0 - number of children\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	int n; 
    char *mq_name = build_name();

	if(argc!=1) usage();
    printf("%s\n",mq_name);

	mqd_t pq;
	struct mq_attr attr;
	attr.mq_maxmsg=10;
	attr.mq_msgsize=1;
    //strcat(name,"_s");
	if((pq=TEMP_FAILURE_RETRY(mq_open(mq_name, O_RDWR | O_CREAT, 0600, &attr)))==(mqd_t)-1) ERR("mq open sum");

	sethandler(sigint_handler,SIGINT);
	// sethandler(mq_handler,SIGRTMIN);
	// create_children(n,pin,pout);

	static struct sigevent not;
	not.sigev_notify=SIGEV_SIGNAL;
	not.sigev_signo=SIGRTMIN;
	not.sigev_value.sival_ptr=&pq;
	//if(mq_notify(pin, &not)<0)ERR("mq_notify");

    char *line = NULL;
    char *endptr;
    ssize_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, stdin)) != -1) {
        printf("Retrieved line of length %zu: ", read);
        if(line[0] >= 48 && line[0]<=57){
            int value = strtol(line, &endptr, 10);
            printf("%d\n", value);
        }
        else printf("%s\n", line);
    }

	//server_work();

    free(line);
	return EXIT_SUCCESS;
}