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

#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                     exit(EXIT_FAILURE))

//MAX_BUFF must be in one byte range
#define MAX_BUFF 512
volatile sig_atomic_t last_signal = 0;

void usage(char * name){
    fprintf(stderr,"USAGE: %s n\n",name);
    fprintf(stderr,"invalid argument\n");
    exit(EXIT_FAILURE);
}

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

void sig_killme(int sig) {
    if(rand()%5==0)
        exit(EXIT_SUCCESS);
}

char* remove_letter(char* str, int index) {
    char *result;
    result =(char*)malloc(sizeof(char)*strlen(str));
    if(result == NULL) ERR("malloc");
    //int toBeRemoved=2;
    //memmove(&[toBeRemoved],&a[toBeRemoved+1],strlen(a)-toBeRemoved);
    memmove(&str[index], &str[index + 1], strlen(str) - index);
    return str;
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

void child_work(int Chin, int Chout) {
    srand(getpid());
    char buff[MAX_BUFF];
    int status, index;
    sleep(1);
    for(;;) {
        status = read(Chin,&buff,MAX_BUFF);
        printf("%s\n", buff);
        if(status <0 && errno==EBADF) {
            printf("Child [%d] dies\n", getpid()); 
            break; 
        } 
        if(status > 0) {
            if(buff[0]=='0') { 
                printf("\nChild [%d]: \t", getpid()); 
                if(TEMP_FAILURE_RETRY(close(Chout))) ERR("Closing descriptor");
                break; 
            }
            printf("\nChild [%d]: %s\t", getpid(), buff); 
            index = rand()%strlen(buff);
            memmove(&buff[index], &buff[index + 1], strlen(buff) - index);
            printf("(%s)%ld\n",buff, strlen(buff));
            if(strlen(buff) ==0) { 
                snprintf(buff, 2, "%c",'0'); 
                if(write(Chout,buff,2) <0) ERR("write to parent"); }
            else { if(write(Chout,buff,MAX_BUFF) <0) ERR("write to parent"); }
        }
        //else { printf("Child [%d] dies\n", getpid()); return; }
    }
}

void parent_work(char *buff, int Pout, int Pin) {
    srand(getpid());
    char tmp[MAX_BUFF];
    int status, index;
    //int i = 0, len = 0;
    if(TEMP_FAILURE_RETRY(write(Pout,buff,MAX_BUFF)) <0) ERR("write to first child");
    for(;;) {
        status = read(Pin,&tmp,MAX_BUFF);
        //printf("%d\n", status);
        if(status <0 && errno ==EBADF) { //&& strlen(tmp)<=2) { 
            printf("%d\n", status); 
            if(TEMP_FAILURE_RETRY(close(Pout))) ERR("Closing descriptor");
            break; 
        }
        if(status > 0) {
            if(tmp[0]=='0') { 
                printf("\nParent [%d]: \t", getpid()); 
                if(TEMP_FAILURE_RETRY(close(Pout))) ERR("Closing descriptor");
                break; 
            }
            printf("\nParent [%d]: %s\t", getpid(), tmp); 
            index = rand()%strlen(tmp);
            memmove(&tmp[index], &tmp[index + 1], strlen(tmp) - index);
            printf("(%s) %ld\n",tmp, strlen(tmp));
            if(strlen(tmp) ==0) { 
                snprintf(tmp, 2, "%c",'0'); 
                if(write(Pout,tmp,2) <0) ERR("write to child"); }
            else { if(write(Pout,tmp,MAX_BUFF) <0) ERR("write to child");}
        }
    }

}

        
int main(int argc, char** argv) {
    int n, P1[2], P2[2];
    char *buffer;
    if(2!=argc) usage(argv[0]);
    
    if(NULL == (buffer=(char*)malloc(sizeof(char)*strlen(argv[1])+1))) ERR("malloc");
    strcpy(buffer, argv[1]);

    create_pipes(P1, P2);
    printf("Pipes: P1[0]=%d, P1[1]=%d, P2[0]=%d, P2[1]=%d\n", P1[0], P1[1], P2[0], P2[1]);
    pid_t pid;
    if((pid=fork())<0) ERR("fork");
    if(0==pid) { 
        printf("Child [%d] created\n", getpid());
        child_work(P1[0], P2[1]);
        //exit(EXIT_SUCCESS);
    }
    parent_work(buffer, P1[1],P2[0]);

    //printf("Closing descriptor %d\n", P1[1]);
    //if(TEMP_FAILURE_RETRY(close(P1[1]))) ERR("close");
    printf("Closing descriptor %d\n", P2[0]);
    if(TEMP_FAILURE_RETRY(close(P2[0]))) ERR("close");
    //printf("Closing descriptor %d\n", P2[1]);
    //if(TEMP_FAILURE_RETRY(close(P2[1]))) ERR("close");
    printf("Closing descriptor %d\n", P1[0]);
    if(TEMP_FAILURE_RETRY(close(P1[0]))) ERR("close");
    printf("Parent dies\n");
    return EXIT_SUCCESS;
}
        
