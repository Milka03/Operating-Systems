//program has parent and one child process, each with its own pipe
//parent and child separately choose a random substring of program argument,
//write it to its own pipe and then read from pipe, printing result to stdout
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

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                     exit(EXIT_FAILURE))

//MAX_BUFF must be in one byte range
#define MAX_BUFF 512

void usage(char * name){
        fprintf(stderr,"USAGE: %s n\n",name);
        fprintf(stderr,"invalid argument\n");
        exit(EXIT_FAILURE);
}

void child_work(int chpinds, int chpoutds, char *buf) {
        srand(getpid());
        char c;
        int status;
        int rlen = rand()% (strlen(buf)) + 1; 
        int pos = rand()% (strlen(buf));
        if(pos + rlen > strlen(buf)) rlen = strlen(buf)-pos;
        int tmp=pos+rlen;
        printf("Child writing: ");
        while(pos < tmp) { 
                status=TEMP_FAILURE_RETRY(write(chpinds,&buf[pos],1));
                if(status!=1) {
                        if(TEMP_FAILURE_RETRY(close(chpinds))) ERR("close");
                        chpinds=0;
                }
                printf("%c", buf[pos]);
                pos++;
        }
        printf("\nChild reading: ");
        while(rlen > 0){
                status=read(chpoutds,&c,1);
                if(status!=1) {
                    if(TEMP_FAILURE_RETRY(close(chpoutds))) ERR("close");
                    chpoutds=0;
                }
                printf("%c",c);
                rlen--;
        }
        if(status<0) ERR("read from pipe");
        printf("\n");
        if(close(chpinds)) ERR("close");
        printf("PID %d, close fn, Child[1]=%d\n", getpid(),chpinds);
        if(close(chpoutds)) ERR("close");
        printf("PID %d, close fn, Child[0]=%d\n", getpid(), chpoutds);
        exit(EXIT_SUCCESS);
}

void parent_work(int pchin,int pchout, char *arr) {
        char c;
        int status;
        srand(getpid());

        int rlen = rand()% (strlen(arr)) + 1;
        int pos = rand()% (strlen(arr));
        if(pos + rlen > strlen(arr)) rlen = strlen(arr)-pos;
        int tmp=pos+rlen;
        printf("\nParent writing: ");
        while(pos < tmp) { 
            status=TEMP_FAILURE_RETRY(write(pchin,&arr[pos],1));
            if(status!=1) {
                    if(TEMP_FAILURE_RETRY(close(pchin))) ERR("close");
                    pchin=0;
            }
            printf("%c", arr[pos]);
            pos++;
        }
        printf("\nParent reading: ");
        while(rlen > 0){
                status=read(pchout,&c,1);
                if(status!=1) {
                    if(TEMP_FAILURE_RETRY(close(pchout))) ERR("close");
                    pchout=0;
                }
                printf("%c",c);
                rlen--;
        }
        if(status<0) ERR("read from pipe");
        printf("\n");
}

int main(int argc, char** argv) {
        int Parent[2], ChP[2]; //Pipe descriptors Parent->Child, Child->Parent
        char *buffer;
        if(2!=argc) usage(argv[0]);
        if(NULL == (buffer=(char*)malloc(sizeof(char)*strlen(argv[1])))) ERR("malloc");
        strcpy(buffer, argv[1]);

        if(pipe(Parent)) ERR("pipe");
        printf("PID %d, pipe fn, Parent[0]=%d, Parent[1]=%d\n", getpid(), Parent[0], Parent[1]);
        //create 1 child, its pipe and pass the buffer to it
        pid_t pid;
	    if((pid=fork())<0) ERR("fork");
	    if(0==pid) { 
                if(pipe(ChP)) ERR("pipe");
                printf("PID %d, pipe fn, Child[0]=%d, Child[1]=%d\n", getpid(), ChP[0], ChP[1]);
                child_work(ChP[1], ChP[0], buffer);
        }
        while(wait(NULL)>0);
        parent_work(Parent[1],Parent[0], buffer);  //writing to pipe, reading from it and printing

        if(close(Parent[1])) ERR("close");
        printf("PID %d, close fn, Parent[1]=%d\n", getpid(),Parent[1]);
        if(close(Parent[0])) ERR("close");
        printf("PID %d, close fn, Parent[0]=%d\n", getpid(), Parent[0]);
        
        return EXIT_SUCCESS;
}