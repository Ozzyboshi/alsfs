#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void create_file_element(struct stat*,int,int,int,int,int,int,int);
void create_dir_element(struct stat*,int,int,int,int,int,int);
void create_stat_element(struct stat*,int,int,int,int,int,int,int,int);
int is_root_element(const char*);
time_t amigadate_to_pc(const int, const int, const int);
char* fd_to_filename(int);
int get_filetype(const char* ,char** ,size_t );
int is_adf(const char*);
int is_zip(const char*);
int get_trackdevice(const char*);
