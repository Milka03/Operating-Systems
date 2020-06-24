#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>

#define MAX_BUFF 5
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                     exit(EXIT_FAILURE))

volatile sig_atomic_t last_signal = 0;

int sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;
        if (-1==sigaction(sigNo, &act, NULL))
                return -1;
        return 0;
}

void sig_handler(int sig) {
	last_signal = sig;
}

void sigQUIT_handler(int sig) {
	last_signal = 1;
}

void sigINT_handler(int sig) {
	last_signal = 0;
}
void sigPIPE_handler(int sig) {
	last_signal = sig;
	printf("Broken pipe\nExiting...\n");

}

void sigchld_handler(int sig) {
    pid_t pid;
    for(;;){
        pid=waitpid(0, NULL, WNOHANG);
            if(0==pid) return;
            if(0>=pid) {
                if(ECHILD==errno) return;
                ERR("waitpid:");
            }
    }
}

void child_work(int Pin, int CHout, int CHin, int Pout) {
        char buf[MAX_BUFF];
        char buffer[MAX_BUFF];
        int status, sec;
        int tmp = 0; int tmp2=0;
        if(sethandler(SIG_IGN,SIGINT)) ERR("Setting SIGINT handler in child");
        if(sethandler(SIG_IGN,SIGQUIT)) ERR("Setting SIGINT handler in child");
        
        printf("CHILD [%d], Pin: %d, CHout: %d, CHin: %d, Pout: %d\n", getpid(), Pin,CHout,CHin,Pout);
        for(;;) {
        //from parent to child
        status = read(Pin,&buffer,MAX_BUFF);
        if(status ==0) return;//ERR("read from Parent"); ->close descriptors!!
        if(status > 0) {
            tmp = atoi(buffer);
            if(tmp>0) { 
                printf("\nChild [%d] received: %d\n", getpid(), tmp); 
                tmp--;
            }
            snprintf(buffer, MAX_BUFF, "%d",tmp);
            if(write(CHout,buffer,MAX_BUFF) <0) { if(errno != EBADF) ERR("write to child"); }
        }
        //from child to parent
        sec = read(CHin,&buffer,MAX_BUFF);
        if(sec > 0) { //ERR("read from child");
            tmp = atoi(buffer);
            if(tmp > 0) {
                printf("\nWorker [%d] received: %d\n", getpid(), tmp);
                tmp--; 
            }
        }
        if(sec < 0) sleep(1);
        snprintf(buffer, MAX_BUFF, "%d",tmp);
        if(write(Pout,buffer,MAX_BUFF) <0) ERR("write to Parent");
        }
}

void parent_work(int number,int Pout,int Pin) {
    char buf[MAX_BUFF];
    int i,status,counter=0;
    if(sethandler(sig_handler,SIGINT)) ERR("Setting SIGINT handler in parent");
    if(sethandler(sig_handler,SIGQUIT)) ERR("Setting SIGQUIT handler");
    sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    int sig;
	while(1) {
		sigwait(&mask, &sig);
		if(sig == SIGINT) {
            printf("Closing descriptor %d\n", Pout);
            if(TEMP_FAILURE_RETRY(close(Pout))) ERR("close");
            printf("Closing descriptor %d\n", Pin);
            if(TEMP_FAILURE_RETRY(close(Pin))) ERR("close");
            break;
        }
        if(sig == SIGQUIT) {
            snprintf(buf, MAX_BUFF, "%d",number);
            if(TEMP_FAILURE_RETRY(write(Pout,buf,MAX_BUFF)) <0) ERR("write to first child");

            // while(1) {
            status = read(Pin,&buf,MAX_BUFF);
            if(status > 0) {
                i = atoi(buf);
                if(i>0) printf("\nParent [%d] received: %d\n", getpid(), i); 
                if(write(Pout,buf,MAX_BUFF) <0) ERR("write to first child");
            }
            //if(status==0 || status == -1) break;
            //if(i>0) printf("\nParent [%d] received: %d\n", getpid(), i);
        }
	}
}

void create_pipes(int *p1, int *p2) {
	if (pipe(p1) == -1)
		ERR("pipe");
    fcntl(p1[0], F_SETFL, O_NONBLOCK);
    fcntl(p1[1], F_SETFL, O_NONBLOCK);
	if (pipe(p2) == -1)
		ERR("pipe");
    fcntl(p2[0], F_SETFL, O_NONBLOCK);
    fcntl(p2[1], F_SETFL, O_NONBLOCK);
}

void create_children_and_pipes(int n, int c, int *P1,int *P2) {
    int chP1[2], chP2[2];
    int m = c;
    if(m<n)
        create_pipes(chP1, chP2);
    switch (fork()) {
        case 0:
            printf("Child nr %d created\n", m++);
            printf("Closing descriptor %d\n", P1[1]);
            if(TEMP_FAILURE_RETRY(close(P1[1]))) ERR("close");
            printf("Closing descriptor %d\n", P2[0]);
            if(TEMP_FAILURE_RETRY(close(P2[0]))) ERR("close");
            if(m<=n){
                //printf("Closing descriptor %d\n", chP2[1]);
                //if(TEMP_FAILURE_RETRY(close(chP2[1]))) ERR("close");
                create_children_and_pipes(n, m, chP1, chP2);
                child_work(P1[0], chP1[1], chP2[0], P2[1]);
                printf("Closing descriptor %d\n", chP2[0]);
                if(TEMP_FAILURE_RETRY(close(chP2[0]))) ERR("close");
            }
            else 
                child_work(P1[0],-1,-1,P2[1]); 
            wait(NULL);
            if(m<=n){
                printf("Closing descriptor %d\n", chP2[1]);
                if(TEMP_FAILURE_RETRY(close(chP2[1]))) ERR("close");
                printf("Closing descriptor %d\n", chP2[0]);
                if(TEMP_FAILURE_RETRY(close(chP2[0]))) ERR("close");
                printf("Closing descriptor %d\n", chP1[1]);
                if(TEMP_FAILURE_RETRY(close(chP1[1]))) ERR("close");
                printf("Closing descriptor %d\n", chP1[0]);
                if(TEMP_FAILURE_RETRY(close(chP1[0]))) ERR("close");
            }
            printf("Closing descriptor %d\n", chP2[0]);
            if(TEMP_FAILURE_RETRY(close(chP2[0]))) ERR("close");
            printf("Child nr %d dies\n", m-1);
            exit(EXIT_SUCCESS);
            
        case -1: ERR("Fork:");
    }
}


void usage(char * name){
    fprintf(stderr,"USAGE: %s n\n",name);
    fprintf(stderr,"0<n<=10 - number of children\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    int n, P1[2], P2[2], c=1;
    if(2!=argc) usage(argv[0]);
    n = atoi(argv[1]);
    if (n<=0||n>10) usage(argv[0]);

    if(sethandler(sigINT_handler,SIGINT)) ERR("Seting parent SIGINT:");
    if(sethandler(sigQUIT_handler,SIGQUIT)) ERR("Seting parent SIGQUIT:");
    create_pipes(P1, P2);
    create_children_and_pipes(n,c,P1, P2);
    parent_work(100, P1[1],P2[0]);

    //printf("Closing descriptor %d\n", P1[1]);
    //if(TEMP_FAILURE_RETRY(close(P1[1]))) ERR("close");
    //printf("Closing descriptor %d\n", P2[0]);
    //if(TEMP_FAILURE_RETRY(close(P2[0]))) ERR("close");
    wait(NULL);
    printf("Parent dies\n");
    return EXIT_SUCCESS;
}