//Program walks through all objects and subdirectories in a directory specified
//by environmental variable DIR and finds the biggest file in this directory tree.
//If environmental variable OUTPUT is set to "yes" then the result is printed to 
//newly created file "output.txt", else result is printed to stdout.

#define _XOPEN_SOURCE 500
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <ftw.h>
#define MAX_PATH 20


#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

long int maxsize = 0;
char m_name[1024];

int walk(const char *name, const struct stat *s, int type, struct FTW *f)
{
	if (f->level <= atoi(getenv("DEPTH"))) {
		switch (type) {
		case FTW_DNR:
		case FTW_D:
			printf("directory:  %s   size:  %ld\n", name, s->st_size);
			break;
		case FTW_F:
			printf("file:  %s  size:  %ld\n", name, s->st_size);
			if (s->st_size > maxsize) {
				maxsize = s->st_size;
				strcpy(m_name, name);
			}
			break;
		case FTW_SL:
			printf("link:  %s   size:  %ld\n", name, s->st_size);
			break;
		default:
			printf("other:  %s   size:  %ld\n", name, s->st_size);
			break;
		}
	}
    return 0;
}



 int main(int argc, char** argv){

    FILE *f1;
    f1 = fopen("output.txt", "w+");
    if(strcmp(getenv("OUTPUT"), "yes") == 0){
        fprintf(f1,"Directory: %s\nDirectory analysis:\n", getenv("DIR"));
        if(nftw(getenv("DIR"),walk,MAX_PATH,FTW_PHYS)==0){
            fprintf(f1, "Biggest file: %s with size: %ld\n", m_name, maxsize);
        }
        else  fprintf(f1, "%s: brak dostępu\n", getenv("DIR"));
    }
    else{
        printf("TEST\n");
        printf("Directory: %s\nDirectory analysis:\n", getenv("DIR"));
        if(nftw(getenv("DIR"),walk,MAX_PATH,FTW_PHYS)==0)
            printf("Biggest file: %s with size: %ld\n", m_name, maxsize);
        else 
            printf("%s: brak dostępu\n", getenv("DIR"));
    }
    fclose(f1);
    
    return EXIT_SUCCESS;
 }