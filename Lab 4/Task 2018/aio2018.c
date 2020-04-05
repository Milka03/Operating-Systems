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

#define BLOCKS 2
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),exit(EXIT_FAILURE))

typedef struct {
    struct aiocb *controlBlock;
    pthread_t mainTID;
} thread_struct;

void ReadData(struct aiocb * aiocbs, off_t offset);
void WriteData(struct aiocb *aiocbs, off_t offset);
void cleanup(struct aiocb *aiocbList, int blocksnum);
off_t getFileLength (int fd);
void FillAioStructs(struct aiocb *aiocbList, int fd, int blocksize, int blocksNum);
void threadWork(__sigval_t arg);


int main(int argc, char *argv[]){
        int blockSize, fd, blocksnum, cycles;
        struct aiocb *aiocbList;
        off_t fileSize, *positions;
        char *filename;
        off_t lastCyclePos = 0;

        if(argc % 2 != 0) ERR("Wrong number of arguments");
        cycles = (argc / 2) - 1;
        filename = argv[1];

        for(int i = 0; i < cycles; i++)
        {
                if ((blocksnum = atoi(argv[2*i+2])) < 1) ERR("Invalid blocks number");
                if ((blockSize = atoi(argv[2*i+3])) < 1) ERR("Invalid block size");

                if ((fd = TEMP_FAILURE_RETRY(open(filename, O_RDWR))) == -1)
                        ERR("Cannot open file");
                fileSize = getFileLength(fd);

                if ((aiocbList = malloc(blocksnum* sizeof(struct aiocb))) == NULL) ERR("Aiocb malloc");
                if ((positions = malloc(blocksnum* sizeof(off_t))) == NULL) ERR("Malloc for positions");
                FillAioStructs(aiocbList, fd, blockSize, blocksnum);
                printf("\tCYCLE %d\nNumber of blocks: %d, Block size: %d\n", i+1, blocksnum, blockSize);

                sigset_t sigset;
                sigemptyset(&sigset);
                sigaddset(&sigset, SIGIO);
                sigprocmask(SIG_BLOCK, &sigset, NULL);
                int sig;
                for(int j = 0; j < blocksnum; j++) 
                {
                        positions[j] = lastCyclePos + j*blockSize;
                        printf("Block %d Position: %ld\t", j+1, positions[j]);
                        ReadData(aiocbList + j, positions[j]);
                        sigwait(&sigset, &sig);
                }
                lastCyclePos += blocksnum*blockSize;
                cleanup(aiocbList, blocksnum);
                free(positions);
                sleep(1);
        }
        sleep(1);

        
        if (TEMP_FAILURE_RETRY(close(fd)) == -1)
                ERR("Cannot close file");
        return EXIT_SUCCESS;
}


off_t getFileLength(int fd){
        struct stat buf;
        if (fstat(fd, &buf) == -1)
                ERR("Cannot fstat file");
        return buf.st_size;
}

void FillAioStructs(struct aiocb *aiocbList, int fd, int blocksize, int blocksNum)
{
        char *buff;
        thread_struct *thstr;
        for (int i = 0; i < blocksNum; i++)
        {
                if ((thstr = malloc(sizeof(thread_struct))) == NULL) ERR("Malloc for thread attribute");
                if ((buff = calloc(blocksize, sizeof(char))) == NULL) ERR("Calloc for a buffer");
                memset(&aiocbList[i], 0, sizeof(struct aiocb));
                aiocbList[i].aio_fildes = fd;
                aiocbList[i].aio_offset = 0; 
                aiocbList[i].aio_nbytes = blocksize;
                aiocbList[i].aio_buf = (volatile void *) (buff);
                aiocbList[i].aio_sigevent.sigev_notify = SIGEV_THREAD;
                aiocbList[i].aio_sigevent.sigev_notify_function = threadWork;
                thstr->controlBlock = &aiocbList[i];
                thstr->mainTID = pthread_self();
                aiocbList[i].aio_sigevent.sigev_value.sival_ptr = thstr;
        }
}


void ReadData(struct aiocb *aiocbs, off_t offset)
{
        aiocbs->aio_offset = offset;
        if (aio_read(aiocbs) == -1)
                ERR("Cannot read");
}

void WriteData(struct aiocb *aiocbs, off_t offset)
{
        aiocbs->aio_offset = offset;
        if (aio_write(aiocbs) == -1)
                ERR("Cannot write");
}


void cleanup(struct aiocb *aiocbList, int blocksnum)
{
        for (int i = 0; i < blocksnum; ++i)
        {
                free((void *)aiocbList[i].aio_buf);
                free(aiocbList[i].aio_sigevent.sigev_value.sival_ptr);
        }
}


void threadWork(__sigval_t arg)
{
        thread_struct* myarg = (thread_struct *)arg.sival_ptr;
        char * buff = (char*)myarg->controlBlock->aio_buf;
        for(int i = 0; i < myarg->controlBlock->aio_nbytes; i++)
        { 
            if(isalpha(buff[i])) {
                if(isupper(buff[i])) buff[i] = (char)tolower(buff[i]);
                else buff[i] = (char)toupper(buff[i]);
            }
            printf("%c", buff[i]);
        }
        printf("\n");
        if (lseek(myarg->controlBlock->aio_fildes, myarg->controlBlock->aio_offset, SEEK_SET) == -1) ERR("Seeking");
        if(TEMP_FAILURE_RETRY(write(myarg->controlBlock->aio_fildes, buff, myarg->controlBlock->aio_nbytes)) == -1)
                ERR("Writing");
        pthread_kill(myarg->mainTID, SIGIO);
}
