//Program takes pairs of arguments: ("path" to folder, "limit" of folder content in bytes)
//and appends the name of the folder given by "path" to the file out.txt only if the total sum 
//of contained objects sizes is above the limit given by "limit" (it does not count objects in sub-folders)

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

#define MAX_PATH 20
#define ERR(source) (perror(source),\
		     fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
		     exit(EXIT_FAILURE))

void usage(char* pname){
	fprintf(stderr,"USAGE:%s directory size\n",pname);
	exit(EXIT_FAILURE);
}


void short_print_dir(const char* dir_name);

void print_dir (){
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;
	//int dirs=0,files=0,links=0,other=0;
	if(NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (lstat(dp->d_name, &filestat)) ERR("lstat");
			if (S_ISDIR(filestat.st_mode)) //dirs++;
                printf("Directory: %s\n", dp->d_name);
			else if (S_ISREG(filestat.st_mode)) //files++;  
                printf("File: %s\n", dp->d_name);
			else if (S_ISLNK(filestat.st_mode)) //links++;
                printf("Link: %s\n", dp->d_name);
			else printf("Other: %s\n", dp->d_name); //other++;
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
	//printf("Files: %d, Dirs: %d, Links: %d, Other: %d\n",files,dirs,links,other);
}

void print_size(){
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;
	if(NULL == (dirp = opendir("."))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (stat(dp->d_name, &filestat) == 0) //ERR("lstat");
				printf("size: %ld\n", filestat.st_size);
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
}

off_t fsize(const char* filename){
    struct stat st;
    if(stat(filename, &st) == 0)
        return st.st_size;
    return -1;
}


void print_alldir (const char* dir_name){
	DIR *dirp;
	struct dirent *dp;
	//struct stat filestat;
	if(NULL == (dirp = opendir(dir_name))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			printf("%s\n", dp->d_name);
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
}

void short_print_dir(const char* dir_name){ 
    DIR *d;
	struct dirent *dir;
    d = opendir(dir_name);  //if dir_name = "." the program prints current directory
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            printf("%s\n", dir->d_name);
        }
    }
    closedir(d);
}

int is_above_limit(const char* dir_name, long limit){
	DIR *dirp;
	struct dirent *dp;
	struct stat filestat;
	long sum =0;
	if(NULL == (dirp = opendir(dir_name))) ERR("opendir");
	do {
		errno = 0;
		if ((dp = readdir(dirp)) != NULL) {
			if (stat(dp->d_name, &filestat) == 0) 
				sum += filestat.st_size;
		}
	} while (dp != NULL);
	if (errno != 0) ERR("readdir");
	if(closedir(dirp)) ERR("closedir");
	if(sum > limit) return 1;
	return 0;
}


int main(int argc, char** argv){

	if(argc%2 != 1) usage(argv[0]);
	int i;
	long j;
	FILE* s1;

	if((s1=fopen("out.txt","w+"))==NULL) ERR("fopen");

	for (i=1; i<argc; i=i+2){
		j = atoi(argv[i+1]);
		if(is_above_limit(argv[i], j) == 1)
			fprintf(s1,"%s\n",argv[i]);
	}
	if(fclose(s1))ERR("fclose");

    return EXIT_SUCCESS;
}



