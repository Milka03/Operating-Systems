#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))
#define HERR(source) (fprintf(stderr,"%s(%d) at %s:%d\n",source,h_errno,__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

#define TIMEOUT 15

volatile sig_atomic_t last_signal=0 ;

void sigalrm_handler(int sig) {
        last_signal=sig;
}
int sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;
        if (-1==sigaction(sigNo, &act, NULL))
                return -1;
        return 0;
}
int make_socket(void){
        int sock;
        sock = socket(PF_INET,SOCK_DGRAM,0);
        if(sock < 0) ERR("socket");
        return sock;
}
struct sockaddr_in make_address(char *address, char *port){
        int ret;
        struct sockaddr_in addr;
        struct addrinfo *result;
        struct addrinfo hints = {};
        hints.ai_family = AF_INET;
        if((ret=getaddrinfo(address,port, &hints, &result))){
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret));
                exit(EXIT_FAILURE);
        }
        addr = *(struct sockaddr_in *)(result->ai_addr);
        freeaddrinfo(result);
        return addr;
}
void usage(char * name){
        fprintf(stderr,"USAGE: %s domain port time \n",name);
}

void print_matrix(int16_t data[4]){
    printf("[%d  %d]\n[%d  %d]\n", (data[0]), (data[2]), (data[1]), (data[3]));
}

int main(int argc, char** argv) {
        int fd;
        struct sockaddr_in addr;
        int16_t data[8];
        int16_t result[4];

        if(argc!=3) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }
        if(sethandler(sigalrm_handler,SIGALRM)) ERR("Seting SIGALRM:");
        fd = make_socket();
        addr=make_address(argv[1],argv[2]);
        //time=htons(atoi(argv[3]));

        char *line = NULL;
        char *endptr;
        ssize_t len = 0;
        ssize_t read;
        int counter = 0;
        
        scanf("%hd",&data[0]);// ,&data[1],&data[2],&data[3],&data[4],&data[5],&data[6],&data[7]);
        scanf("%hd",&data[1]);
        scanf("%hd",&data[2]);
        scanf("%hd",&data[3]);
        scanf("%hd",&data[4]);
        scanf("%hd",&data[5]);
        scanf("%hd",&data[6]);
        scanf("%hd",&data[7]);
        printf("---THE END OF SCANF---\nMatrix 1:\n");

        printf("[%d  %d]\n[%d  %d]\nMatrix 2:\n", data[0], data[1], data[2], data[3]);
        printf("[%d  %d]\n[%d  %d]\n\n", data[4], data[5], data[6], data[7]);

        if(TEMP_FAILURE_RETRY(sendto(fd,(char *)data,sizeof(int16_t[8]),0,&addr,sizeof(addr)))<0)
                ERR("sendto:");
        //alarm(TIMEOUT);

        socklen_t size=sizeof(struct sockaddr_in);
        if(recvfrom(fd,(char *)result,sizeof(int16_t[4]),0,&addr,&size)<0 && errno!=EINTR) {
                ERR("recvfrom:");
        }
        //printf("Received: %d  %d  %d  %d\n", result[0],result[1],result[2],result[3]);
        print_matrix(result);

        if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
        return EXIT_SUCCESS;
}



