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
#define ERR(source) (perror(source),\
                fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                exit(EXIT_FAILURE))

#define BACKLOG 3
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
// int bind_local_socket(char *name){
//         struct sockaddr_un addr;
//         int socketfd;
//         if(unlink(name) <0&&errno!=ENOENT) ERR("unlink");
//         socketfd = make_socket(PF_UNIX,SOCK_STREAM);
//         memset(&addr, 0, sizeof(struct sockaddr_un));
//         addr.sun_family = AF_UNIX;
//         strncpy(addr.sun_path,name,sizeof(addr.sun_path)-1);
//         if(bind(socketfd,(struct sockaddr*) &addr,SUN_LEN(&addr)) < 0)  ERR("bind");
//         if(listen(socketfd, BACKLOG) < 0) ERR("listen");
//         return socketfd;
// }

int bind_tcp_socket(uint16_t port){
        struct sockaddr_in addr;
        int socketfd,t=1;
        socketfd = make_socket(PF_INET,SOCK_STREAM);
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR,&t, sizeof(t))) ERR("setsockopt");
        if(bind(socketfd,(struct sockaddr*) &addr,sizeof(addr)) < 0)  ERR("bind");
        if(listen(socketfd, BACKLOG) < 0) ERR("listen");
        return socketfd;
}

int add_new_client(int sfd){
        int nfd;
        if((nfd=TEMP_FAILURE_RETRY(accept(sfd,NULL,NULL)))<0) {
                if(EAGAIN==errno||EWOULDBLOCK==errno) return -1;
                ERR("accept");
        }
        return nfd;
}

void usage(char * name){
        fprintf(stderr,"USAGE: %s socket port\n",name);
}
ssize_t bulk_read(int fd, char *buf, size_t count){
        int c;
        size_t len=0;
        do{
                c=TEMP_FAILURE_RETRY(read(fd,buf,count));
                if(c<0) return c;
                if(0==c) return len;
                buf+=c;
                len+=c;
                count-=c;
        }while(count>0);
        return len ;
}
ssize_t bulk_write(int fd, char *buf, size_t count){
        int c;
        size_t len=0;
        do{
                c=TEMP_FAILURE_RETRY(write(fd,buf,count));
                if(c<0) return c;
                buf+=c;
                len+=c;
                count-=c;
        }while(count>0);
        return len ;
}

int16_t calculate(int16_t data[1]){
        int16_t sum = 0;
        int16_t tmp;
        //tmp=ntohl(data[0]);

        char *str;
        str = (char*)malloc(sizeof(int16_t*));
        sprintf(str, "%d", data[0]);

        for(int i=0; i<strlen(str); i++){
            sum+= (str[i] - 48);
            //printf("%d",sum);
        }
        data[0]=sum;
        return sum;
}



///---------------------
int16_t doServer(int fdL){
        int cfd;
        int16_t maxsum=0;
        int16_t tmpsum;
        int16_t data[1];

        ssize_t size;
        fd_set base_rfds, rfds ;
        sigset_t mask, oldmask;
        FD_ZERO(&base_rfds);
        FD_SET(fdL, &base_rfds);
        sigemptyset (&mask);
        sigaddset (&mask, SIGINT);
        sigprocmask (SIG_BLOCK, &mask, &oldmask);
        while(do_work){
                rfds=base_rfds;
                if(pselect(fdL+1,&rfds,NULL,NULL,NULL,&oldmask)>0){
                        if((cfd=add_new_client(fdL))>=0){;
                                if((size=bulk_read(cfd,(char *)data,sizeof(int16_t[1])))<0) ERR("read:");
                                if(size==(int)sizeof(int16_t[1])){
                                        tmpsum=calculate(data);
                                        if(bulk_write(cfd,(char *)data,sizeof(int16_t[1]))<0&&errno!=EPIPE) ERR("write:");
                                        if(tmpsum>maxsum){
                                                maxsum=tmpsum;
                                        }
                                }
                                if(TEMP_FAILURE_RETRY(close(cfd))<0)ERR("close");
                        }
                }else{
                        if(EINTR==errno) continue;
                        ERR("pselect");
                }
        }
        sigprocmask (SIG_UNBLOCK, &mask, NULL);
        return maxsum;
}

int main(int argc, char** argv) {
        int fdL;//fdT;
        int new_flags;
        int16_t MAX;
        if(argc!=2) {
                usage(argv[0]);
                return EXIT_FAILURE;
        }
        if(sethandler(SIG_IGN,SIGPIPE)) ERR("Seting SIGPIPE:");
        if(sethandler(sigint_handler,SIGINT)) ERR("Seting SIGINT:");

        fdL=bind_tcp_socket(atoi(argv[1]));
        new_flags = fcntl(fdL, F_GETFL) | O_NONBLOCK;
        fcntl(fdL, F_SETFL, new_flags);

        MAX=doServer(fdL);
        printf("HIGH SUM=%d\n",MAX);
        if(TEMP_FAILURE_RETRY(close(fdL))<0)ERR("close");
        //if(unlink(argv[1])<0)ERR("unlink");
        fprintf(stderr,"Server has terminated.\n");
        return EXIT_SUCCESS;
}