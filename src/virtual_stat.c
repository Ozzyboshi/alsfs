#include <virtual_stat.h>


void create_dir_element(struct stat* statbuf,int year,int mon,int day,int hour,int min,int sec)
{
    return create_stat_element(statbuf,year,mon,day,hour,min,sec,1);
}
void create_stat_element(struct stat* statbuf,int year,int mon,int day,int hour,int min,int sec,int dir)
{
    struct tm info;
    time_t timeinfo;
		
    info.tm_year = year - 1900;
    info.tm_mon = mon - 1;
    info.tm_mday = day;
    info.tm_hour = hour;
    info.tm_min = min;
    info.tm_sec = sec;
    info.tm_isdst = -1;
    timeinfo = mktime(&info);

    memset (statbuf,0,sizeof(struct stat));
    statbuf->st_size=4096;
    statbuf->st_dev = 0;
    statbuf->st_ino = 0;
    if (dir)
        statbuf->st_mode = 0040644;
    else
        statbuf->st_mode = 0100644;
    statbuf->st_nlink = 0;
    statbuf->st_uid = 0;
    statbuf->st_gid = 0;
    statbuf->st_rdev = 0;
    
    statbuf->st_blksize = 4096;
    statbuf->st_blocks = 0;
    statbuf->st_atime = timeinfo;
    statbuf->st_mtime = timeinfo;
    statbuf->st_ctime = timeinfo;
    
    return ;
}

int is_root_element(char* path)
{
    int i=0;
    for (i=0;i<ROOTDIRELEMENTS_NUMBER;i++)
        if (strlen(path)>1 && !strcmp(ROOTDIRELEMENTS[i],&path[1]))
            return 1;
    return 0;
}