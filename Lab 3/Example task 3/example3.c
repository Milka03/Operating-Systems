#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#define DEFAULT_N 3
#define DEFAULT_K 10
#define NEXT_DOUBLE(seedptr) ((double) rand_r(seedptr) / (double) RAND_MAX)
#define ERR(source) (perror(source),\
                     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     exit(EXIT_FAILURE))

typedef unsigned int UINT;
typedef struct trheadstruct {
        pthread_t tid;
        UINT seed;
        int arraysize;
        float *floatArr;
        float *resultArr;
        pthread_mutex_t *mxArr;
} threadstruct_t;


void ReadArguments(int argc, char **argv, int *threadnum, int *arraysize);
void* thread_work(void *args);

int main(int argc, char** argv) {
        int threadnum, arraysize;
        ReadArguments(argc, argv, &threadnum, &arraysize);

        srand(time(NULL));
        float *TASKARRAY = (float*) malloc(sizeof(float) * arraysize);
        float *RESULT = (float*) malloc(sizeof(float) * arraysize);
        pthread_mutex_t *mutexArr = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * arraysize);
        float scale = 0.0;
        for(int i = 0; i < arraysize; i++) {
            scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
            TASKARRAY[i] = 1 + rand()%60 + scale;
            RESULT[i] = 0;
            if(pthread_mutex_init(&mutexArr[i], NULL))ERR("Couldn't initialize mutex!");
            printf("%f\t", TASKARRAY[i]);
        }
        printf("\n");
        threadstruct_t* tArr = (threadstruct_t*) malloc(sizeof(threadstruct_t) * threadnum);
        if (tArr == NULL) ERR("Malloc error for estimation arguments!");
        
        for (int i = 0; i < threadnum; i++) {
                tArr[i].seed = rand();
                tArr[i].arraysize = arraysize;
                tArr[i].floatArr = TASKARRAY;
                tArr[i].resultArr = RESULT;
                tArr[i].mxArr = mutexArr;
        }
        for (int i = 0; i < threadnum; i++) {
                int err = pthread_create(&(tArr[i].tid), NULL, thread_work, &tArr[i]);
                if (err != 0) ERR("Couldn't create thread");
        }

        for (int i = 0; i < threadnum; i++) {
                int err = pthread_join(tArr[i].tid, NULL);
                if (err != 0) ERR("Can't join with a thread");
        }
        for(int i = 0; i < arraysize; i++) {
                printf("%f\t", RESULT[i]);
        }
        printf("\n");
        return(EXIT_SUCCESS);
}


void ReadArguments(int argc, char **argv, int *threadnum, int *arraysize) {
        *threadnum = DEFAULT_N;
        *arraysize = DEFAULT_K;
        if (argc >= 2) {
                *threadnum = atoi(argv[1]);
                if (*threadnum <= 0) {
                        printf("Invalid value for 'threadCount'");
                        exit(EXIT_FAILURE);
                }
        }
        if (argc >= 3) {
                *arraysize = atoi(argv[2]);
                if (*arraysize <= 0) {
                        printf("Invalid value for 'samplesCount'");
                        exit(EXIT_FAILURE);
                }
        }
}


void* thread_work(void *args) {
        threadstruct_t* tArr = args;
        int index;
        int i = 0; 
        while (i < tArr->arraysize) {
                index = rand()%tArr->arraysize;
                if(tArr->resultArr[index] == 0) {
                    pthread_mutex_lock(&tArr->mxArr[index]);
                    tArr->resultArr[index] = (float) sqrt(tArr->floatArr[index]);
                    pthread_mutex_unlock(&tArr->mxArr[index]);
                    printf("Index: %d  Sqrt Value: %f \n", index, tArr->resultArr[index]);
                }
                i++;
        }
        //printf("Index: %d  Sqrt Value: %f \n", index, tArr->resultArr[index]);
        return NULL;
}
