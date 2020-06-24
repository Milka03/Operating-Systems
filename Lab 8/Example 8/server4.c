#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <signal.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))
#define FS_NUM 10 
#define THREAD_NUM 4

volatile sig_atomic_t do_work=1 ;

void sigint_handler(int sig) {
        do_work=0;
}
int sethandler( void (*f)(int), int sigNo) {
        struct sigaction act;
        memset(&act, 0, sizeof(struct sigaction));
        act.sa_handler = f;
        if (-1==sigaction(sigNo, &act, NULL))
                return -1;
        return 0;
}
int make_socket(int domain, int type){
        int sock;
        sock = socket(domain,type,0);
        if(sock < 0) ERR("socket");
        return sock;
}
int bind_inet_socket(uint16_t port,int type){
        struct sockaddr_in addr;
        int socketfd,t=1;
        socketfd = make_socket(PF_INET,type);
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
        if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0)  ERR("bind");
        return socketfd;
}


void usage(char * name){
        fprintf(stderr,"USAGE: %s  port\n",name);
}
struct arguments{
        //int fd;
        //struct sockaddr_in addr[7];
        pthread_t tid;
        int16_t digits[4];
        int result;
};

struct commThreadInfo {
    int fd;
    struct sockaddr_in addr;
    int16_t matrices[8];
    int result_matrix[4];
};

void* calculation_thread(void *arg){
        struct arguments *args= (struct arguments*) arg;
        int16_t* result;
        if(NULL==(result=malloc(sizeof(int16_t)))) ERR("malloc");
        //printf("%hd %hd %hd %hd\n", (args->digits[0]),(args->digits[1]),(args->digits[2]), (args->digits[3]));
        *result= (int16_t)(args->digits[0]*args->digits[2] + args->digits[1]*args->digits[3]); 
        return result;
}

void* communicatinThread(void *arg){
        struct commThreadInfo  *args= (struct commThreadInfo*) arg;
        int tt;
        int16_t numbers[8];
        int16_t matrix[THREAD_NUM];
        int16_t *res;
        struct arguments childstruct[THREAD_NUM];
        struct sockaddr_in addr;
        socklen_t size=sizeof(struct sockaddr_in);
        //printf("Communication thread created\n");
        while(do_work){
                if(recvfrom(args->fd,(char *)&numbers,sizeof(int16_t[8]),0,&addr,&size)<0) {
                        if(errno==EINTR) continue ;
                        ERR("recvfrom:");
                }
                childstruct[0].digits[0]=numbers[0]; childstruct[0].digits[1]=numbers[2]; childstruct[0].digits[2]=numbers[4]; childstruct[0].digits[3]=numbers[5];
                childstruct[1].digits[0]=numbers[0]; childstruct[1].digits[1]=numbers[2]; childstruct[1].digits[2]=numbers[6]; childstruct[1].digits[3]=numbers[7];
                childstruct[2].digits[0]=numbers[1]; childstruct[2].digits[1]=numbers[3]; childstruct[2].digits[2]=numbers[4]; childstruct[2].digits[3]=numbers[5];
                childstruct[3].digits[0]=numbers[1]; childstruct[3].digits[1]=numbers[3]; childstruct[3].digits[2]=numbers[6]; childstruct[3].digits[3]=numbers[7];
                
                // tmp[0]=numbers[0];
                // tmp[1]=numbers[1];
                // tmp[2]=numbers[2];
                // tmp[3]=numbers[3];

                for (int i = 0; i < THREAD_NUM; i++) {
                    int err = pthread_create(&(childstruct[i].tid), NULL, calculation_thread, &childstruct[i]);
                    if (err != 0) ERR("Couldn't create thread");
                }
                for (int i = 0; i < THREAD_NUM; i++) {
                        int err = pthread_join(childstruct[i].tid, (void*)&res);
                        if (err != 0) ERR("Can't join with a thread");
                        if(res!=NULL) { matrix[i]=*res; free(res); }
                }
                //printf("\n matrix: %d %d %d %d\n", (matrix[0]),(matrix[1]),(matrix[2]), (matrix[3]));
                if(TEMP_FAILURE_RETRY(sendto(args->fd,(char *)&matrix,sizeof(int16_t[4]),0,&addr,sizeof(addr)))<0) {
                    ERR("sendto:");
                }
        }
        free(args);
        return NULL;
}



int main(int argc, char** argv) {
        int fd;
        pthread_t thread;
        struct commThreadInfo *args;
        if(argc!=2) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }
        if(sethandler(sigint_handler,SIGINT)) ERR("Seting SIGINT:");
        fd=bind_inet_socket(atoi(argv[1]),SOCK_DGRAM);

        if((args=(struct commThreadInfo*)malloc(sizeof(struct commThreadInfo)))==NULL) ERR("malloc:");
        args->fd=fd;

        if (pthread_create(&thread, NULL,communicatinThread, (void *)args) != 0) ERR("pthread_create");
        if(pthread_join(thread, NULL)) ERR("Can't join with thread");

        if(TEMP_FAILURE_RETRY(close(fd))<0)ERR("close");
        fprintf(stderr,"Server has terminated.\n");
        free(args);
        return EXIT_SUCCESS;
}