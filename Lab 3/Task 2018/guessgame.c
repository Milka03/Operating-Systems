#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>

#define DEFAULT_T 5  //num of threads
#define DEFAULT_N 10
#define NEXT_DOUBLE(seedptr) ((double) rand_r(seedptr) / (double) RAND_MAX)
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct trheadstruct {
        pthread_t tid;
        UINT seed;
        int *vectorsize;
        int *VectorArr;
        int *GV;
        pthread_mutex_t *mxVector; //single mutex to block whole array
        pthread_mutex_t *mxGV;
} threadstruct_t;

typedef struct mainthread {
        pthread_t tid;
        UINT seed;
        int *vectorsize;
        int *VectorArr;
        int *GV;
        pthread_mutex_t *mxVector; //single mutex to block whole array
        pthread_mutex_t *mxGV;
        sigset_t *pMask;
        bool *QuitFlag;
        pthread_mutex_t *pmxQuitFlag; 
        int *idxToDel;
        pthread_mutex_t *mxToDel;
} mainthread_t;


void ReadArguments(int argc, char **argv, int *threadnum, int *arraysize);
void* thread_work(void *args);
void* signal_handling(void* marg);
void print_tid(pthread_t id);
void printArray(int* array, int arraySize);
void milisleep(UINT milisec);

int main(int argc, char** argv) {
        int threadnum, vectorsize;
        ReadArguments(argc, argv, &threadnum, &vectorsize);

        srand(time(NULL));
        bool quitFlag = false;
        int threadToDel = -1;
        pthread_mutex_t mutexToDel = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_t mxQuitFlag = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_t mutexGV = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_t mxArray = PTHREAD_MUTEX_INITIALIZER;
        //pthread_mutex_t *mutexArr = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * vectorsize);
        int *VECTOR = (int*) malloc(sizeof(int) * vectorsize);
        int GUESSED_VALUE = 0;
        for(int i = 0; i < vectorsize; i++){
            VECTOR[i] = 0;
            //if(pthread_mutex_init(&mutexArr[i], NULL))ERR("Couldn't initialize mutex!");
        }

        sigset_t oldMask, newMask;
        sigemptyset(&newMask);
        sigaddset(&newMask, SIGINT);
        sigaddset(&newMask, SIGUSR2);
        sigaddset(&newMask, SIGQUIT);
        if (pthread_sigmask(SIG_BLOCK, &newMask, &oldMask)) ERR("SIG_BLOCK error");

        mainthread_t mainarg;
        threadstruct_t* tArr = (threadstruct_t*) malloc(sizeof(threadstruct_t) * threadnum);
        if (tArr == NULL) ERR("Malloc error for estimation arguments!");
        for (int i = 0; i < threadnum; i++) {
                tArr[i].seed = rand();
                tArr[i].vectorsize = &vectorsize;
                tArr[i].VectorArr = VECTOR;
                tArr[i].GV = &GUESSED_VALUE;
                tArr[i].mxVector = &mxArray;
                tArr[i].mxGV = &mutexGV;
        }
        mainarg.seed = rand();
        mainarg.vectorsize = &vectorsize;
        mainarg.VectorArr = VECTOR;
        mainarg.GV = &GUESSED_VALUE;
        mainarg.mxVector = &mxArray;
        mainarg.mxGV = &mutexGV;
        mainarg.QuitFlag = &quitFlag;
        mainarg.pmxQuitFlag = &mxQuitFlag;
        mainarg.pMask = &newMask;
        mainarg.idxToDel = &threadToDel;
        mainarg.mxToDel = &mutexToDel;
        for (int i = 0; i < threadnum; i++) {
                int err = pthread_create(&(tArr[i].tid), NULL, thread_work, &tArr[i]);
                if (err != 0) ERR("Couldn't create thread");
        }
        if(pthread_create(&mainarg.tid, NULL, signal_handling, &mainarg)) ERR("Error signal handling thread!");
        while (true) {
                pthread_mutex_lock(&mxQuitFlag);
                if (quitFlag == true) {
                        pthread_mutex_unlock(&mxQuitFlag);
                        break;
                } else {
                        pthread_mutex_unlock(&mxQuitFlag);
                        if(threadToDel != -1) {
                                int err = pthread_cancel(tArr[threadToDel].tid); }
                        pthread_mutex_lock(&mxArray);
                        printArray(VECTOR, vectorsize);
                        pthread_mutex_unlock(&mxArray);
                        milisleep(500);
                }
        }
        for (int i = 0; i < threadnum; i++) {
                int err = pthread_cancel(tArr[i].tid);
                //if (err != 0) ERR("Can't join with a thread");
        }
        printf("\n");
        free(tArr);
        free(VECTOR);
        if (pthread_sigmask(SIG_UNBLOCK, &newMask, &oldMask)) ERR("SIG_BLOCK error");
        exit(EXIT_SUCCESS);
}


void ReadArguments(int argc, char **argv, int *threadnum, int *vectorsize) {
        *threadnum = DEFAULT_T;
        *vectorsize = DEFAULT_N;
        if (argc >= 2) {
                *threadnum = atoi(argv[1]);
                if (*threadnum <= 0) {
                        printf("Invalid value for 'threadCount'");
                        exit(EXIT_FAILURE);
                }
        }
        if (argc >= 3) {
                *vectorsize = atoi(argv[2]);
                if (*vectorsize <= 0) {
                        printf("Invalid value for 'samplesCount'");
                        exit(EXIT_FAILURE);
                }
        }
}

void* thread_work(void *args) {
        threadstruct_t* tArr = args;
        int index;
        int i = 0; 
        while (1) {
                index = rand()%*tArr->vectorsize;
                if(tArr->VectorArr[index] != 0 && tArr->VectorArr[index] != *tArr->GV ) {
                    pthread_mutex_lock(tArr->mxGV);
                    *tArr->GV = tArr->VectorArr[index];
                    pthread_mutex_unlock(tArr->mxGV);
                    //printf("New Guessed Value: %d\n", *tArr->GV);
                }
                if(tArr->VectorArr[index] == *tArr->GV){
                    kill(getpid(), SIGUSR2);
                }
                //else printf("No changes... Index: %d\n", index);
                sleep(1);
        }
        return NULL;
}

void* signal_handling(void* marg) {
        mainthread_t* mainarg = marg;
        int signo, index;
        srand(time(NULL));
        for (;;) {
                if(sigwait(mainarg->pMask, &signo)) ERR("sigwait failed.");
                switch (signo) {
                        case SIGINT:
                                pthread_mutex_lock(mainarg->mxVector);
                                index = rand()% *mainarg->vectorsize;
                                mainarg->VectorArr[index] = 1 + rand() % 255;
                                pthread_mutex_unlock(mainarg->mxVector);
                                break;
                        case SIGQUIT:
                                pthread_mutex_lock(mainarg->pmxQuitFlag);
                                *mainarg->QuitFlag = true;
                                pthread_mutex_unlock(mainarg->pmxQuitFlag);
                                return NULL;
                        case SIGUSR2:
                                pthread_mutex_lock(mainarg->mxGV);
                                printf("GUESSED VALUE: %d\n", *mainarg->GV);
                                pthread_mutex_unlock(mainarg->mxGV);
                                pthread_mutex_lock(mainarg->mxToDel);
                                *mainarg->idxToDel = rand()% *mainarg->vectorsize;
                                pthread_mutex_unlock(mainarg->mxToDel);
                                break;
                        default:
                                printf("unexpected signal %d\n", signo);
                                exit(1);
                }
        }
        return NULL;
}

void milisleep(UINT milisec) {
    time_t sec= (int)(milisec/1000);
    milisec = milisec - (sec*1000);
    struct timespec req;
    req.tv_sec = sec;
    req.tv_nsec = milisec * 1000000L;
    if(nanosleep(&req,&req)) ERR("nanosleep");
}


void printArray(int* array, int arraySize) {
        printf("[");
        for (int i =0; i < arraySize; i++)
                printf(" %d", array[i]);
        printf(" ]\n");
}


void print_tid(pthread_t id) {
    printf("Thread ID: ");
    size_t i;
    for (i = sizeof(i); i; --i)
            printf("%02x", *(((unsigned char*) &id) + i - 1));
    printf("\n");
}