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
#define NUM 1024

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
	last_signal = sig;
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


ssize_t bulk_read(int fd, char *buf, size_t count){
	ssize_t c;
	ssize_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(read(fd,buf,count));
		if(c<0) return c;
		if(c==0) return len; //EOF
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}

ssize_t bulk_write(int fd, char *buf, size_t count){
	ssize_t c;
	ssize_t len=0;
	do{
		c=TEMP_FAILURE_RETRY(write(fd,buf,count));
		if(c<0) return c;
		buf+=c;
		len+=c;
		count-=c;
	}while(count>0);
	return len ;
}


//==========================================================

void child_work(char c, int n){
    struct timespec t = {0, n*1000000};
	nanosleep(&t,NULL);
	if(c == '.'){
	    if(kill(getppid(),SIGUSR1))ERR("kill");}
    else {
        if(kill(getppid(),SIGUSR2))ERR("kill"); 
    }
    printf("PROCESS [%d]  bit: %c  interval: %d\n", getpid(), c, n);
    //printf("PROCESS with pid %d chose s: %d\n",getpid(), s);
}


void parent_work(sigset_t oldmask, int len, char* tmp){
    char curr;
    int allmax=0;
    int l=0;
    char bits[5]={'n','n','n','n','n'};
    char match[NUM];
	while(1)
    {    
        last_signal=0;
        if(last_signal != SIGUSR1 && last_signal != SIGUSR2) sigsuspend(&oldmask);
        if(last_signal == SIGUSR1) curr='.';   
        if(last_signal == SIGUSR2) curr='-';
            
        bits[4]=bits[3];
        bits[3]=bits[2];
        bits[2]=bits[1];
        bits[1]=bits[0];
        bits[0]=curr;
        printf("[ %c %c %c %c %c ]\n",bits[0],bits[1],bits[2],bits[3],bits[4]);

        if(l == len){
            printf("FULL MATCH: %d\n", len);
            break;
		 	exit(EXIT_SUCCESS);
        }
        match[l]=curr;
        if(strncmp(match,tmp,l+1)==0) {
            l++; 
            printf("TEST3: %d\n", l);
            if(l > allmax) { allmax=l;  printf("longest match: %d\n", allmax);}
        }
        else { l=0; printf("TEST4: %d\n", l); }

    }
}

void usage(char *name){
    fprintf(stderr,"USAGE: %s 0<n\n",name);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv){
    if(argc!=3) usage(argv[0]);
    sethandler(sigchld_handler,SIGCHLD);
    sethandler(sig_handler, SIGUSR1);
    sethandler(sig_handler,SIGUSR2);

    int ind, inm;
	ssize_t count, count2;
    sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    int *del2=malloc(sizeof(int)*NUM);
    char *buf=malloc(NUM);
    char *delays=malloc(NUM);
	if(!buf) ERR("malloc");
    if(!delays) ERR("malloc");

	if((inm=TEMP_FAILURE_RETRY(open(argv[1],O_RDONLY,0777)))<0)ERR("open");
	if((count=bulk_read(inm,buf,NUM))<0) ERR("read");
    printf("%ld, %s\n", count, buf);
    if((ind=TEMP_FAILURE_RETRY(open(argv[2],O_RDONLY,0777)))<0)ERR("open");
	if((count2=bulk_read(ind,delays,NUM))<0) ERR("read");
    //printf("%ld, %s\n", count2, delays);
    int j=0,i;
    char *token = strtok(delays, "\n");
    while( token != NULL ) {
      del2[j]=atoi(token);
      token = strtok(NULL, "\n");
      j++;
    }

    pid_t s;
    for (i=0;i < count;i++) {
        if((s=fork())<0) ERR("Fork:");
        if(!s) {
            sethandler(SIG_IGN, SIGUSR1);
            sethandler(SIG_IGN, SIGUSR2);
            child_work(buf[i], del2[i]);
            exit(EXIT_SUCCESS);
        }
    }
    parent_work(oldmask, count, buf);
    if(TEMP_FAILURE_RETRY(close(ind)))ERR("close");
    if(TEMP_FAILURE_RETRY(close(inm)))ERR("close");
	free(buf);
    free(delays);
    while(wait(NULL)>0);

    return EXIT_SUCCESS;
}