#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#define NUM 30

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
		     		     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

void sethandler( void (*f)(int), int sigNo) {
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));
	act.sa_handler = f;
	if (-1==sigaction(sigNo, &act, NULL)) ERR("sigaction");
}

void sig_handler(int sig) {
	last_signal = sig;;
}

void sigchld_handler(int sig) {
	pid_t pid;
	for(;;){
		pid=waitpid(0, NULL, WNOHANG);
		if(pid==0) return;
		if(pid<=0) {
			if(errno==ECHILD) return;
			ERR("waitpid");
		}
	}
}


void child_work(int m) {
    srand(getpid());     
    int s=100+rand()%101;
    struct timespec t = {0, s*1000000};
	for(int n =0; n<NUM; n++){
		nanosleep(&t,NULL);
		if(kill(getppid(),SIGUSR1))ERR("kill");
        printf("PROCESS [%d]  %d) *\n", getpid(), n+1);
	}
    //printf("PROCESS with pid %d chose s: %d\n",getpid(), s);
}


void parent_work(sigset_t oldmask) {
	int count=0;
	while(count < 100){
		last_signal=0;
		while(last_signal!=SIGUSR1){
			sigsuspend(&oldmask);
        }
		count++;
		printf("[PARENT] received %d SIGUSR1\n", count);
	}
    if(kill(0,SIGUSR2))ERR("kill");
}


void create_children(int n) {
    sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);
    pid_t s;
    for (n--;n>=0;n--) {
        if((s=fork())<0) ERR("Fork:");
        printf("%d\n", s);
        if(!s) {
            child_work(n);
            exit(EXIT_SUCCESS);
        }
    }
    sethandler(SIG_IGN,SIGUSR2);
    parent_work(oldmask);
    //while(waitpid(0,NULL, WNOHANG)>0);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);
}


void usage(char *name){
    fprintf(stderr,"USAGE: %s 0<n\n",name);
    exit(EXIT_FAILURE);
}


int main(int argc, char** argv) {
    int n;
    if(argc<2)  usage(argv[0]);
    n=atoi(argv[1]);
    if(n<=0)  usage(argv[0]);

    sethandler(sigchld_handler,SIGCHLD);
	sethandler(sig_handler,SIGUSR1);

    create_children(n);
    
    return EXIT_SUCCESS;
}