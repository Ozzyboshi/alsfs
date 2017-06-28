#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include "rootelements.h"

void create_dir_element(struct stat*,int,int,int,int,int,int);
void create_stat_element(struct stat*,int,int,int,int,int,int,int);
int is_root_element(char*);