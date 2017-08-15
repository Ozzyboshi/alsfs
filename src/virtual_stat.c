#include "virtual_stat.h"
#include "rootelements.h"
#include "params.h"
#include <magic.h>

void create_file_element(struct stat* statbuf,int year,int mon,int day,int hour,int min,int sec,int size)
{
    return create_stat_element(statbuf,year,mon,day,hour,min,sec,0,size);
}
void create_dir_element(struct stat* statbuf,int year,int mon,int day,int hour,int min,int sec)
{
    return create_stat_element(statbuf,year,mon,day,hour,min,sec,1,4096);
}
void create_stat_element(struct stat* statbuf,int year,int mon,int day,int hour,int min,int sec,int dir,int size)
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
    statbuf->st_size=size;
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
    
    statbuf->st_blksize = ALSFS_BLK_SIZE;
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
}

char* fd_to_filename(int fd)
{
    char* filename = malloc(0xFFF);
    int MAXSIZE = 0xFFF;
    char proclnk[0xFFF];
    sprintf(proclnk, "/proc/self/fd/%d", fd);
    ssize_t r = readlink(proclnk, filename, MAXSIZE);
    filename[r] = '\0';
    return filename;
}

int get_filetype(const char* filename,char** magic_full,size_t length)
{
    magic_t magic_cookie;
    magic_cookie = magic_open(MAGIC_CHECK);
    if (magic_cookie == NULL) 
    {
            return -1;
    }
    if (magic_load(magic_cookie, NULL) != 0) 
    {
            magic_close(magic_cookie);
            return -2;
    }
    snprintf(*magic_full,length,"%s",magic_file(magic_cookie, filename));
    magic_close(magic_cookie);
    return 0;
}
int is_adf(const char* filename)
{
    int ret=0;
    char* fileType=malloc(100);
    switch (get_filetype(filename,&fileType,100))
    {
        case -1: return -1;
        case -2: return -2;
    }
    if (!strcmp(fileType,"Amiga DOS disk")||!strcmp(fileType,"Amiga Fastdir FFS disk")) ret=1;
    free(fileType);
    return ret;
}

int is_zip(const char* filename)
{
    int ret=0;
    char* fileType=malloc(100);
    switch (get_filetype(filename,&fileType,100))
    {
        case -1: return -1;
        case -2: return -2;
    }
    if (strstr(fileType,"Zip archive data")) ret=1;
    free(fileType);
    return ret;
}

// Assuming that path has depth at least 2 it returns the trackdevce, for example , /adf/DF0/whatever returns 0, /adf/DF1/whatever returns 1, -1 on error
int get_trackdevice(const char* path)
{
    char* first_occ = index(path,'/')+1;
    if (first_occ==NULL) return -1;

    char* second_occ = index(first_occ,'/')+1;
    if (second_occ==NULL) return -1;

    char trackdevicenumber[2];
    trackdevicenumber[0] = second_occ[2];
    trackdevicenumber[1]=0;

    return atoi(trackdevicenumber);
}
