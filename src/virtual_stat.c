#include "virtual_stat.h"
#include "rootelements.h"

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

int is_root_element(const char* path)
{
    int i=0;
    for (i=0;i<ROOTDIRELEMENTS_NUMBER;i++)
        if (strlen(path)>1 && !strcmp(ROOTDIRELEMENTS[i],&path[1]))
            return 1;
    return 0;
}

// Amiga stat returns days from january first 1978, minutes and ticks, this function converts it to pc format
time_t amigadate_to_pc(const int days, const int minutes, const int ticks)
{
    struct tm date = {0} ;
    time_t timer;
    div_t output=div(minutes, 60);
    
    timer=time(NULL);
    date = *gmtime( &timer ) ;
    date.tm_year = 1978-1900;
    date.tm_mon = 0;
    date.tm_mday = 1 + days;
    date.tm_hour = output.quot-1;
    date.tm_min = output.rem;
    date.tm_sec = (int)ticks/50 ;
    
    return  mktime( &date ) ;
//    date = *gmtime( &timer ) ;
    
    
}
