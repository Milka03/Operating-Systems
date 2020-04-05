#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <aio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#define BLOCKS 1
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),exit(EXIT_FAILURE))

typedef struct {
    struct aiocb *controlBlock;
    pthread_t mainTID;
} thread_struct;

void ReadArguments(int argc, char **argv, int *blocksnum, char **filename);
void ReadData(struct aiocb * block, off_t Position);
void cleanup(struct aiocb *aiocbList);
off_t getFileLength (int fd);
void FillAioStructs(struct aiocb *aiocbList, int fd, int blocksize);
void threadWork(__sigval_t arg);


int main(int argc, char *argv[]){
        int blockSize, fd, blocksnum, remainderSize;
        struct aiocb aiocbList[BLOCKS];
        off_t fileSize, *positions, remainderPosition;
        char * filename;

        ReadArguments(argc, argv, &blocksnum, &filename);
        if ((fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1)
                ERR("Cannot open file");
        fileSize = getFileLength(fd);
        blockSize = fileSize / blocksnum;
        remainderSize = fileSize % blocksnum;
        remainderPosition = (off_t)blocksnum * (off_t)blockSize;
        printf("Blocksize: %d\nReminder: %d\n", blockSize, remainderSize);   
        if ((positions = malloc(blocksnum* sizeof(off_t))) == NULL) ERR("Malloc for positions");
        FillAioStructs(aiocbList, fd, blockSize);
     
        printf("File size: %ld\n", fileSize);
        for(int i = 0; i < blocksnum; i++) 
        {
                positions[i] = i*blockSize;
                printf("Block %d, position: %ld\n", i+1, positions[i]);
        }
        if(remainderSize) printf("Remainder size: %d, position: %ld\n", remainderSize, remainderPosition);
               
        sleep(3);
        sigset_t sigset;
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGIO);
        sigprocmask(SIG_BLOCK, &sigset, NULL);
        int sig;
        for (int j = 0; j < blocksnum; ++j)
        {
                ReadData(aiocbList, positions[j]);
                sigwait(&sigset, &sig);
        }
        aiocbList[0].aio_nbytes = remainderSize;
        ReadData(aiocbList, remainderPosition);
        sigwait(&sigset, &sig);

        cleanup(aiocbList);
        free(positions);
        if (TEMP_FAILURE_RETRY(close(fd)) == -1)
                ERR("Cannot close file");
        return EXIT_SUCCESS;
}


//functions
off_t getFileLength(int fd){
        struct stat buf;
        if (fstat(fd, &buf) == -1)
                ERR("Cannot fstat file");
        return buf.st_size;
}

void ReadArguments(int argc, char **argv, int *blocksnum, char **filename)
{
        if(argc != 3) ERR("Wrong number of arguments");
        *filename = argv[1];
        if((*blocksnum = atoi(argv[2])) < 1) ERR("Invalid block size");
}

void FillAioStructs(struct aiocb *aiocbList, int fd, int blocksize)
{
        char *buffer;
        thread_struct *thstr;
        for (int i = 0; i < BLOCKS; i++)
        {
                if ((thstr = malloc(sizeof(thread_struct))) == NULL) ERR("Malloc for thread attribute");
                if ((buffer = calloc(blocksize, sizeof(char))) == NULL) ERR("Calloc for a buffer");
                memset(&aiocbList[i], 0, sizeof(struct aiocb));
                aiocbList[i].aio_fildes = fd;
                aiocbList[i].aio_offset = 0; //blocksize*i;
                aiocbList[i].aio_nbytes = blocksize;
                aiocbList[i].aio_buf = (volatile void *) (buffer);
                aiocbList[i].aio_sigevent.sigev_notify = SIGEV_THREAD;
                aiocbList[i].aio_sigevent.sigev_notify_function = threadWork;
                thstr->controlBlock = &aiocbList[i];
                thstr->mainTID = pthread_self();
                aiocbList[i].aio_sigevent.sigev_value.sival_ptr = thstr;
        }
}

void cleanup(struct aiocb *aiocbList)
{
        for (int i = 0; i < BLOCKS; ++i)
        {
                free((void *)aiocbList[i].aio_buf);
                free(aiocbList[i].aio_sigevent.sigev_value.sival_ptr);
        }
}


void ReadData(struct aiocb *aiocbs, off_t offset){
        aiocbs->aio_offset = offset;
        if (aio_read(aiocbs) == -1)
                ERR("Cannot read");
}


void threadWork(__sigval_t arg){
        thread_struct* myarg = (thread_struct *)arg.sival_ptr;
        char * buff = (char*)myarg->controlBlock->aio_buf;
        for(int i = 0; i < myarg->controlBlock->aio_nbytes; i++)
        {
                if(!isalpha(buff[i])) buff[i] = ' ';
                printf("%c", buff[i]);
        }
        printf("\n");
         
        if(TEMP_FAILURE_RETRY(write(myarg->controlBlock->aio_fildes, buff, myarg->controlBlock->aio_nbytes)) == -1)
                ERR("Writing");
        pthread_kill(myarg->mainTID, SIGIO);
}
