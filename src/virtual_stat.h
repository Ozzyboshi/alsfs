#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void create_dir_element(struct stat*,int,int,int,int,int,int);
void create_stat_element(struct stat*,int,int,int,int,int,int,int);
int is_root_element(const char*);
time_t amigadate_to_pc(const int, const int, const int);