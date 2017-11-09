/*
  Amiga Linux Serial File System
  Copyright (C) 2017 Alessio Garzi <gun101@email.it>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.
  A copy of that code is included in the file fuse.h
  
  The point of this FUSE filesystem is to navigate an amiga filesystem tree from a linux machine.
  In order to work, alsfs needs to interface with alsnodews who is in charge of relaying and translating http requests
  coming from alsfs to the amiga by a serial connection.
  
*/
#define _GNU_SOURCE
#include "params.h"
#include "rootelements.h"
#include "virtual_stat.h"
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <curl/curl.h>
#include <json.h>
#include <strings.h>
#include <zip.h>

#include "alsfs_curl.h"


int WGET;


#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

int countPathDepth(const char* );
//char* urlToAmiga(const char*,char*);


struct url_data {
    size_t size;
    char* data;
};
size_t write_data(void *, size_t , size_t , struct url_data *);

int countPathDepth(const char* path)
{
	int i=0;
	int cont=0;
	for (i=0;i<strlen(path);i++)
		if (path[i]=='/')
			cont++;
	return cont;
}

/*char* urlToAmiga(const char* path,char* out)
{
	int depth;
	depth = countPathDepth(path);
	strcpy(out,"");

	// Case of something like /volumes/Games
	if (depth==2)
	{
		char* last_occ = rindex(path,'/')+1;
		strcpy(out,last_occ);
		strcat(out,":");
		
	}
	// Case of something like /volumes/Games/volumes6
	else if (depth>2)
	{
		char* first_occ = index(path,'/')+1;
		char* second_occ = index(first_occ,'/')+1;
		char* third_occ = index(second_occ,'/')+1;
		snprintf(out,third_occ-second_occ,"%s",second_occ);
		strcat(out,":");
		strcat(out,third_occ);
	}
	return out;	
}*/

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
    int retstat;
    char fpath[PATH_MAX];
    int found=0;
    int i=0;
    char* url;
    char* out;
    CURL *curl;
    CURLcode res;
    struct url_data data;
    data.size = 0;
    data.data = malloc(4096);
    long http_code = 0;
    char* httpbody=NULL;

    log_msg("\nbb_getattr(path=\"%s\", statbuf=0x%08x)\n",path, statbuf);

    if (!strcmp(path,"/")) 
    {
    	create_dir_element(statbuf,AMIGA_DEFAULT_YEAR,AMIGA_DEFAULT_MONTH,AMIGA_DEFAULT_DAY,AMIGA_DEFAULT_HOUR,AMIGA_DEFAULT_MINUTE,AMIGA_DEFAULT_SECOND);
    	return 0;
    }
    
    if (countPathDepth(path)==1 && !is_root_element(path))
    {
        log_msg("\nbb_getattr(path=\"%s\", skip because first level invalid dir)\n",path);
        return 0;
    }

    if (is_root_element(path))
    {
        create_dir_element(statbuf,AMIGA_DEFAULT_YEAR,AMIGA_DEFAULT_MONTH,AMIGA_DEFAULT_DAY,AMIGA_DEFAULT_HOUR,AMIGA_DEFAULT_MINUTE,AMIGA_DEFAULT_SECOND);
        return 0;
    }
    else
	{
		int pathCounter=countPathDepth(path);
		if (pathCounter>2 && !strncmp(path,"/adf/DF",7))
		{
			char adfPath[100];
			int trackdevice=get_trackdevice(path);
			sprintf(adfPath,"/adf/DF%d/DF%d.adf",trackdevice,trackdevice);
			if (WGET == 1 )
			{
				create_file_element(statbuf,2017,7,4,10,20,30,0);
				log_msg("Created virtual file of 0 bytes\n");
				return 0;
			}
			if (pathCounter==3 && !strcmp(path,adfPath))
			{
				create_file_element(statbuf,2017,7,4,10,20,30,901120);
				log_msg("Created virtual file of 901120 bytes\n");
				return 0;
			}
			log_msg("### File not found ###\n");
			memset (statbuf,0,sizeof(struct stat));
			return -2;
		}
		for (found=0,i=0;found==0&&i<ROOTDIRELEMENTS_NUMBER;i++)
		{
			log_msg("comparing \"%s\" with %s)\n",ROOTDIRELEMENTS[i],&path[1]);
			if (strlen(path)>0 && path[0]=='/' && !strncmp(ROOTDIRELEMENTS[i],&path[1],strlen(ROOTDIRELEMENTS[i])))
			{
				log_msg("counting slashes for path \"%s\" : %d)\n",path,countPathDepth(path));
				if (countPathDepth(path)==2&& !strcmp(ROOTDIRELEMENTS[i],"adf"))
				{
					char* httpbody;
					long http_response = curl_get_list_floppies(&httpbody);
					if (http_response == 200)
    				{
    					if (httpbody==NULL)
						{
							log_msg("httpbody is NULL \n");
							return -1;
						}
						json_object * jobj = json_tokener_parse(httpbody);
						int arraylen = json_object_array_length(jobj);
						int j=0;
						for (j = 0; j < arraylen; j++) 
						{
							json_object* medi_array_obj = json_object_array_get_idx(jobj, j);
							log_msg("#%s# #%s#\n",json_object_get_string(medi_array_obj),rindex(path,'/')+1);
							if (!strcmp(json_object_get_string(medi_array_obj),rindex(path,'/')+1))
							{
								log_msg("Path corresponds to a valid floppy drive\n");
								create_dir_element(statbuf,2017,7,4,10,20,30);
								return 0;
							}
						}
					}
					log_msg("Path does not correspond to a valid floppy drive\n");
					return -2;
				}
				if (countPathDepth(path)==2) found=1;
			}
		}	

		// If the requested path matches one element in ROOTDIRELEMENTS array		
		if (found)
		{
			create_dir_element(statbuf,2017,7,4,10,20,30);
			return 0;
		}
		else
		{
			int ret = curl_stat_amiga_file(path,statbuf);
			log_msg("statbuf st size: %d\n",statbuf->st_size);
			return ret;
		}
	}
    
    
    log_stat(statbuf);
    
    return retstat;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
int bb_readlink(const char *path, char *link, size_t size)
{
    int retstat;
    char fpath[PATH_MAX];
    
    log_msg("bb_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
	  path, link, size);

	    retstat = log_syscall("fpath", readlink(fpath, link, size - 1), 0);
	    if (retstat >= 0) {
		link[retstat] = '\0';
		retstat = 0;
    }
    
    return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
    int retstat;
    char fpath[PATH_MAX];
    char* out;
    char* bname;

    log_msg("\nbb_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);
    if (!strncmp(path,"/adf/DF",7))
    {
    	WGET = 1;
    	return 0;
    	/*mkdir("/tmp/alsfs");
    	asprintf(out,"%s","/tmp/alsfs/")
    	retstat = log_syscall("mkfifo", mkfifo(out, mode), 0);*/
	}
    
    // On Linux this could just be 'mknod(path, mode, dev)' but this
    // tries to be be more portable by honoring the quote in the Linux
    // mknod man page stating the only portable use of mknod() is to
    // make a fifo, but saying it should never actually be used for
    // that.
    if (S_ISREG(mode)) {
		out=malloc(strlen(path)+1);
		trans_urlToAmiga(path,out);
		long http_response=curl_post_create_empty_file(out);
		free(out);
		if (http_response=200) return 0;
		return -1;
		
		/*retstat = log_syscall("open", open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode), 0);
		if (retstat >= 0)
			retstat = log_syscall("close", close(retstat), 0);*/
    } else
	if (S_ISFIFO(mode))
	    retstat = log_syscall("mkfifo", mkfifo(fpath, mode), 0);
	else
	    retstat = log_syscall("mknod", mknod(fpath, mode, dev), 0);
    
    return retstat;
}

/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    char* out;
    log_msg("\nbb_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
	    
	out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
	long http_response=curl_post_create_empty_drawer(out);
	free(out);
	if (http_response=200) return 0;
	return -1;
}

/** Remove a file */
int bb_unlink(const char *path)
{
	char* out;
    char fpath[PATH_MAX];
    
    log_msg("bb_unlink(path=\"%s\")\n",
	    path);
	
	out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
	long http_response=curl_delete_delete_file(out);
	free(out);
	if (http_response=200) return 0;
	return -1;
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
	char* out;
    char fpath[PATH_MAX];
    
    log_msg("bb_rmdir(path=\"%s\")\n",
	    path);
	    
	out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
	long http_response=curl_delete_delete_file(out);
	free(out);
	if (http_response=200) return 0;
	return -1;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int bb_symlink(const char *path, const char *link)
{
    char flink[PATH_MAX];
    
    log_msg("\nbb_symlink(path=\"%s\", link=\"%s\")\n",
	    path, link);

    return log_syscall("symlink", symlink(path, flink), 0);
}

/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
    char fpath[PATH_MAX];
    char fnewpath[PATH_MAX];
    char* out;
    char* out2;
    int i=0;
    int j=0;
    int found=0;

    log_msg("\nbb_rename(fpath=\"%s\", newpath=\"%s\")\n",
	    path, newpath);
    if (countPathDepth(newpath)==3)
    {
	    for (found=0,i=0;found==0&&i<ROOTDIRELEMENTS_NUMBER;i++)
		{
			log_msg("comparing \"%s\" with %s)\n",ROOTDIRELEMENTS[i],&path[1]);
			if (strlen(path)>0 && path[0]=='/' && !strncmp(ROOTDIRELEMENTS[i],&path[1],strlen(ROOTDIRELEMENTS[i])))
			{
				if (countPathDepth(path)==2&& !strcmp(ROOTDIRELEMENTS[i],"volumes"))
				{
					char* oldName,*newName;
					if (asprintf(&oldName,"%s",rindex(path,'/')+1)==-1)
						log_msg("asprintf() failed at file alsfs.c:%d",__LINE__);
					newName=malloc(strlen(newpath)+1);
					trans_urlToAmiga(newpath,newName);

					for (j=0;newName[j]!=0;j++)
						if (newName[j]==':') { newName[j]=0; break; }
					
					log_msg("oldname:#%s#\n",oldName);
					log_msg("newname:#%s#\n",newName);
					long http_response = curl_put_relabel(oldName,newName);
					free(oldName);
					free(newName);
					log_msg("http_response:#%d#\n",http_response);
					if (http_response=200) return 0;
					else if (http_response==404) return -2;
					return -1;
				}
			}
		}
	}
   
    out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
	out2=malloc(strlen(newpath)+1);
	trans_urlToAmiga(newpath,out2);
    long http_response = curl_put_rename_file_drawer(out,out2);
	free(out);
	free(out2);
	if (http_response=200) return 0;
	return -1;
}

/** Create a hard link to a file */
int bb_link(const char *path, const char *newpath)
{
    char fpath[PATH_MAX], fnewpath[PATH_MAX];
    
    log_msg("\nbb_link(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);


    return log_syscall("link", link(fpath, fnewpath), 0);
}

/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chmod(fpath=\"%s\", mode=0%03o)\n",
	    path, mode);

    return log_syscall("chmod", chmod(fpath, mode), 0);
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)
  
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);

    return log_syscall("chown", chown(fpath, uid, gid), 0);
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);
    return 0;
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_utime(path=\"%s\", ubuf=0x%08x)\n",
	    path, ubuf);
	    
    struct tm info;
                time_t lol;
     
                info.tm_year = 2001 - 1900;
            info.tm_mon = 7 - 1;
            info.tm_mday = 4;
            info.tm_hour = 10;
            info.tm_min = 20;
            info.tm_sec = 30;
            info.tm_isdst = -1;
                lol = mktime(&info);
                ubuf->actime=lol;
                ubuf->modtime=lol;
    return 0;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    int fd;

    if (!strncmp(path,"/adf/DF",7))
    {
    	char template[] = "/tmp/alsfsXXXXXX";
    	fd = mkstemp(template);	
    	close(fd);
	    fd = log_syscall("open", open(template, fi->flags), 0);
	    if (fd < 0)
			retstat = log_error("mkstemp");
		
	    fi->fh = fd;
	    return retstat;
    }
    
    log_msg("\nbb_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);
    return 0;

    

    log_fi(fi);
    
    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    char* out;
    char* httpbody;
    int contBytes=0;
    
    log_msg("\nbb_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);

    if (!strncmp(path,"/adf/DF",7))
    {
    	if (asprintf(&out,"%c",path[strlen(path)-1])==-1)
    		log_msg("asprintf() failed at file alsfs.c:%d",__LINE__);
    	long http_response = curl_get_read_adf(atoi(out),size,offset,&httpbody);
    	free(out);
    	if (http_response == 200)
    	{
    		if (httpbody==NULL)
			{
				log_msg("httpbody is NULL ");
				return 0;
			}
			char* base64Message=strtok(httpbody,"\n");
			if (base64Message==NULL)
			{
				log_msg("Base64message is NULL (no LF found on %s)",httpbody);
				return 0;
			}
			char* strip;
			if (asprintf(&strip,"%s",base64Message)==-1)
				log_msg("asprintf() failed at file alfs:%d",__LINE__);
			size_t rawdataLength=0;
			log_msg("%s",strip);
			log_msg("Strip lungo %d",strlen(strip));
			//char *output = unbase64(strip, strlen(strip),&rawdataLength);
			unsigned char* output;
			Base64Decode(strip, &output, &rawdataLength);

			
			log_msg("Output binario: %d %d %d %d %d - length %d\n",(unsigned char) output[0],(unsigned char)output[1],(unsigned char)output[2],(unsigned char)output[3],(unsigned char)output[4],rawdataLength);
			memcpy(buf,output,rawdataLength);
			contBytes+=rawdataLength;
			free(output);
			free(strip);
			while (base64Message=strtok(NULL,"\n"))
			{
				//log_msg("Lunghezza messaggio successivo%d\n",strlen(base64Message));
				//log_msg(" messaggio successivo ##%s##\n",base64Message);
				if (asprintf(&strip,"%s",base64Message)==-1)
					log_msg("asprintf() failed at file alfs.c:%d",__LINE__);
					
				rawdataLength=0;
				//output = unbase64(strip, strlen(strip),&rawdataLength);
				Base64Decode(strip, &output, &rawdataLength);
				memcpy(buf+contBytes,output,rawdataLength);
				contBytes+=rawdataLength;
				free(output);
				//log_msg("##%s##",base64Message);
			}
			return contBytes;
    	}
    	return size;
    }
    
    out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
    
    long http_response = curl_get_read_file(out,size,offset,&httpbody);
    log_msg("HTTP RESPONSE: %d\n",http_response);
    //log_msg("Buf vale : %s\n",httpbody);
    //char* e = strchr(httpbody,10);
    //log_msg("Il primo invio è alla posizione %d\n",(int)(e-httpbody));
	free(out);
	//if ((int)(e-httpbody)<=0){ log_msg("Ritorno senza fare nulla perche non ho trovato l'invio"); return 0;}
	if (http_response == 200)
	{
		if (httpbody==NULL)
		{
			log_msg("httpbody is NULL ");
			return 0;
		}
		char* base64Message=strtok(httpbody,"\n");
		if (base64Message==NULL)
		{
			log_msg("Base64message is NULL (no LF found on %s)",httpbody);
			return 0;
		}
		//unsigned char* base64DecodeOutput;
		//size_t test;
		char* strip;
		if (asprintf(&strip,"%s",base64Message)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
		//strip[strlen(strip)-1]=0;
		//Base64Decode(strip, &base64DecodeOutput, &test);
		// vecchio int rawdataLength=0;
		size_t rawdataLength=0;
		log_msg("Lunghezza messaggio %d\n",strlen(base64Message));
		if ( strlen(base64Message) == 2)
		{
		    log_msg("Messaggio di 2 rilevato: %s\n",base64Message);
		    return 0;
		}
		//log_msg(" messaggio ##%s##\n",base64Message);
		// vecchio char *output = unbase64(strip, strlen(strip),&rawdataLength);
		unsigned char* output;
		/* nuovo */  Base64Decode(strip, &output, &rawdataLength);
		log_msg("Output binario: %d %d %d %d %d - length %d\n",(unsigned char) output[0],(unsigned char)output[1],(unsigned char)output[2],(unsigned char)output[3],(unsigned char)output[4],rawdataLength);
		memcpy(buf,output,rawdataLength);
		contBytes+=rawdataLength;
		free(output);
		free(strip);
		
		while (base64Message=strtok(NULL,"\n"))
		{
			if (asprintf(&strip,"%s",base64Message)==-1)
				log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
				
			rawdataLength=0;
			// vecchio output = unbase64(strip, strlen(strip),&rawdataLength);
			Base64Decode(strip, &output, &rawdataLength);
			memcpy(buf+contBytes,output,rawdataLength);
			contBytes+=rawdataLength;
			free(output);
			log_msg("##%s##",base64Message);
		}
		//memset(buf,EOF,size);
		//memcpy(buf,httpbody,strlen(httpbody));
		//memcpy(buf,"lollamelo",9);
		log_msg("Contbytes %d",contBytes);
		return contBytes;
	}
	return EOF;
    
    
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);

    return log_syscall("pread", pread(fi->fh, buf, size, offset), 0);
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    char* out;
	char* rawdata;
    
    log_msg("\nbb_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",path, buf, size, offset, fi);

    if (!strcmp(path,"/adf")) return 0;
    if (!strncmp(path,"/adf/DF",7))
    {
    	if (pwrite(fi->fh, buf, size, offset)==-1)
			log_msg("Pwrite failed at line %d",__LINE__);
		return size;
    }
	
	out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
	rawdata=malloc(size+1);
	memset(rawdata,0,size+1);
	memcpy(rawdata,buf,size);
	long res=curl_post_create_mknode(out,rawdata,size,offset);
	free(out);
	free(rawdata);
	return size;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bb_statfs(const char *path, struct statvfs *statv)
{
    int retstat = 0;
    
    
    log_msg("\nbb_statfs(path=\"%s\", statv=0x%08x)\n",
	    path, statv);
    bzero(statv,sizeof(struct statvfs));

    statv->f_bsize=1;
	statv->f_frsize=1;
	statv->f_blocks=1;
	statv->f_bfree=1;
	statv->f_bavail=1;
	statv->f_files=1;
	statv->f_ffree=1;
	statv->f_favail=1;
	statv->f_fsid=1009;
	statv->f_flag=1;
	statv->f_namemax=1;
	//statv->f_type=1;
	//strcpy(statv->f_basetype,"LOL");
	//strcpy(statv->f_str,"LIL");

    return 0;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
// this is a no-op in BBFS.  It just logs the call and returns success
int bb_flush(const char *path, struct fuse_file_info *fi)
{
	char* filename = NULL;
	char* zipfilename = NULL;
    log_msg("\nbb_flush(path=\"%s\", fi=0x%08x)\n", path, fi);
    if (!strncmp(path,"/adf/DF",7))
    {
		filename = fd_to_filename(fi->fh);
		log_msg("\nfilename %s\n",filename);
		
		if (is_zip(filename))
		{
			log_msg("\nZip file detected, decompressing...\n", path, fi);
			struct zip *za;
			struct zip_file *zf;
			char buf[100];
			int err;
			struct zip_stat sb;
			if ((za = zip_open(filename, 0, &err)) == NULL) 
			{
        		zip_error_to_str(buf, sizeof(buf), err, errno);
        		log_msg( "can't open zip archive `%s': %s/n",filename, buf);
        		return -1;
    		}
    		if (zip_stat_index(za, 0, 0, &sb) == 0)
    		{
				int len = strlen(sb.name);

    			if (sb.name[len - 1] != '/') 
    			{
    				zf = zip_fopen_index(za, 0, 0);
	                if (!zf) 
	                {
	                    log_msg("boese, boese/n");
	                    return -1;
	                }
	                if (asprintf(&zipfilename,"%s.adf",filename)==-1)
	                	log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
	                int zfd = open(zipfilename, O_RDWR | O_TRUNC | O_CREAT , 0644);
                	if (zfd < 0) 
                	{
                    	log_msg("boese, boese/n");
                    	return -1;
                	}
                	int sum = 0;
                	while (sum != sb.size) 
                	{
                    	len = zip_fread(zf, buf, 100);
                    	if (len < 0) 
                    	{
                        	log_msg("boese, boese/n");
                        	return -1;
                    	}
                    	if (write(zfd, buf, len)<=0)
                    	{
                    		log_msg("boese, boese/n");
                        	return -1;
                    	}
                    	sum += len;
                	}
                	close(zfd);
                	zip_fclose(zf);
    			}
    		}
    		zip_close(za);
    		free(filename);
    		if (asprintf(&filename,"%s",zipfilename)==-1)
    			log_msg("asprintf() failed at file alfs.c:%d",__LINE__);
    		free(zipfilename);
		}
		if (is_adf(filename))
		{
			char* first_occ = index(path,'/')+1;
			char* second_occ = index(first_occ,'/')+1;
			char trackdevicenumber[2];
			trackdevicenumber[0] = second_occ[2];
			trackdevicenumber[1]=0;
			log_msg("\ntrackdevice %d\n",atoi(trackdevicenumber));
			long http_result = 0;
			
			//curl_post_create_adf(atoi(trackdevicenumber),filename);
			curl_post_create_adf_b64(atoi(trackdevicenumber),filename);
			log_msg("\nHttp res : %d\n",http_result);
			unlink(filename);
		}
		else log_msg("File %s not a valid adf file",filename);
		free(filename);
		close(fi->fh);
		//if (filename) unlink(filename);
		WGET = 0;
	}
    // no need to get fpath on this one, since I work from fi->fh not the path
    log_fi(fi);
	
    return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
    log_msg("\nbb_release(path=\"%s\", fi=0x%08x)\n",path, fi);
    
    return 0;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    log_msg("\nbb_fsync(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
    if (datasync)
	return log_syscall("fdatasync", fdatasync(fi->fh), 0);
    else
#endif	
	return log_syscall("fsync", fsync(fi->fh), 0);
}

#ifdef HAVE_SYS_XATTR_H
/** Set extended attributes */
int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_setxattr(path=\"%s\", name=\"%s\", value=\"%s\", size=%d, flags=0x%08x)\n",
	    path, name, value, size, flags);
    bb_fullpath(fpath, path);

    return log_syscall("lsetxattr", lsetxattr(fpath, name, value, size, flags), 0);
}

/** Get extended attributes */
int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nbb_getxattr(path = \"%s\", name = \"%s\", value = 0x%08x, size = %d)\n",
	    path, name, value, size);
    bb_fullpath(fpath, path);

    retstat = log_syscall("lgetxattr", lgetxattr(fpath, name, value, size), 0);
    if (retstat >= 0)
	log_msg("    value = \"%s\"\n", value);
    
    return retstat;
}

/** List extended attributes */
int bb_listxattr(const char *path, char *list, size_t size)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    char *ptr;
    
    log_msg("bb_listxattr(path=\"%s\", list=0x%08x, size=%d)\n",
	    path, list, size
	    );
    bb_fullpath(fpath, path);

    retstat = log_syscall("llistxattr", llistxattr(fpath, list, size), 0);
    if (retstat >= 0) {
	log_msg("    returned attributes (length %d):\n", retstat);
	for (ptr = list; ptr < list + retstat; ptr += strlen(ptr)+1)
	    log_msg("    \"%s\"\n", ptr);
    }
    
    return retstat;
}

/** Remove extended attributes */
int bb_removexattr(const char *path, const char *name)
{
    char fpath[PATH_MAX];
    
    log_msg("\nbb_removexattr(path=\"%s\", name=\"%s\")\n",
	    path, name);
    bb_fullpath(fpath, path);

    return log_syscall("lremovexattr", lremovexattr(fpath, name), 0);
}
#endif

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];

	return 0;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
size_t write_data(void *ptr, size_t size, size_t nmemb, struct url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    data->size += (size * nmemb);
    tmp = realloc(data->data, data->size + 1);

    if(tmp) {
        data->data = tmp;
    } else {
        if(data->data) {
            free(data->data);
        }
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
	int i=0;
	int arraylen;
	int retstat = 0;
	DIR *dp;
	struct dirent *de;
	char* url;
	CURL *curl;
	CURLcode res;
	struct url_data data;
	data.size = 0;
	data.data = malloc(4096);
	char* out=NULL;
	
	log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);

	curl = curl_easy_init();
  	
	if (!strcmp(path,"/"))
	{
		if (filler(buf, ".", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}
		if (filler(buf, "..", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}
		for (i=0;i<ROOTDIRELEMENTS_NUMBER;i++)
		{
			if (filler(buf, ROOTDIRELEMENTS[i], NULL, 0) != 0) 
			{
			    log_msg("    ERROR bb_readdir filler:  buffer full");
				if (curl) curl_easy_cleanup(curl);
			    return -ENOMEM;
			}
		}
		return 0;
	}
	else if (!strcmp(path,"/volumes"))
	{
/*		printf("%s\n",BB_DATA->alsfs_webserver);
		url=malloc(strlen(URL_LISTVOLUMES)+1);
		strcpy(url,URL_LISTVOLUMES);*/
		if (asprintf(&url,"http://%s/%s",ALSFS_DATA->alsfs_webserver,LISTVOLUMES)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
	}
	else if (!strcmp(path,"/adf"))
	{
/*		printf("%s\n",BB_DATA->alsfs_webserver);
		url=malloc(strlen(URL_LISTVOLUMES)+1);
		strcpy(url,URL_LISTVOLUMES);*/
		if (asprintf(&url,"http://%s/%s",ALSFS_DATA->alsfs_webserver,LISTFLOPPIES)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
		/*long amiga_js_call(LISTFLOPPIES,NULL,"GET",NULL);*/

	}
	else if (!strncmp(path,"/adf",4))
	{
		if (filler(buf, ".", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}
		if (filler(buf, "..", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}
		char dfNumber[2];
		char* buf2=malloc(2);
		sprintf(dfNumber,"%c",path[strlen(path)-1]);
		log_msg("DFNUMBER is %d",atoi(dfNumber));
		int status = curl_get_test_floppy_disk(atoi(dfNumber),&buf2);
		log_msg("Floppy drive test returned %c with status %d",buf2[0],status);

		// 7 means a disk is present so i return an DFX.adf file for downloading the image
		if (buf2[0]=='7')
		{
			char* fileName;
			if (asprintf(&fileName,"DF%c.adf",path[strlen(path)-1])==-1)
				log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);

			if (filler(buf,fileName, NULL, 0) != 0) 
			{
		    	log_msg("    ERROR bb_readdir filler:  buffer full");
		    	free(buf2);
		    	free(fileName);
		    	return -ENOMEM;
			}
			free(fileName);
		}

		free(buf2);
		return 0;
	}

	// A full path inside volumes is requested
	else
	{
		if (filler(buf, ".", NULL, 0) != 0 || filler(buf, "..", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}
		char* httpbody;
		out=malloc(strlen(path)+1);
		trans_urlToAmiga(path,out);
		long http_response = curl_get_content(out,&httpbody);
		free(out);
		if (http_response == 200)
    	{
    		if (httpbody==NULL)
			{
				log_msg("httpbody is NULL \n");
				return -ENOMEM;
			}
			json_object * jobj = json_tokener_parse(httpbody);
			free(httpbody);
			int arraylen = json_object_array_length(jobj);
			int j=0;
			for (j = 0; j < arraylen; j++) 
			{
				json_object* medi_array_obj = json_object_array_get_idx(jobj, j);
				log_msg("#%s#\n",json_object_get_string(medi_array_obj));
				if (filler(buf, json_object_get_string(medi_array_obj), NULL, 0) != 0) 
				{
        			log_msg("    ERROR bb_readdir filler:  buffer full");
        			return -ENOMEM;
    			}
    		}
			return 0;
		}
		return -ENOMEM;

		/*url=malloc(strlen(URL_LISTCONTENT)+strlen(path)+1);
		strcpy(url,URL_LISTCONTENT);*/
		out=malloc(strlen(path)+1);
		trans_urlToAmiga(path,out);
		char *urlEncoded = curl_easy_escape(curl, out, 0);
		if (urlEncoded) log_msg("urlencoded : ##%s##",urlEncoded);
		//strcat(url,out);
		
		if (asprintf(&url,"http://%s/%s?path=%s",ALSFS_DATA->alsfs_webserver,LISTCONTENT,urlEncoded)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
		free(out);
		log_msg("url : ##%s##",url);
		curl_free(urlEncoded);
	}
	
  	if(curl && strlen(url)) 
	{
		log_msg("%s",url);
		curl_easy_setopt(curl, CURLOPT_URL,url);
		free(url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
		   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
		 	fprintf(stderr, "curl_easy_perform() failed: %s\n",
		curl_easy_strerror(res));
		curl_easy_cleanup(curl);
  	}
    
    log_msg("\nbb_readdir(path=\"%s\", buf=0x%08x, filler=0x%08x, offset=%lld, fi=0x%08x)\n",
	    path, buf, filler, offset, fi);

	if (filler(buf, ".", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}
		if (filler(buf, "..", NULL, 0) != 0) 
		{
		    log_msg("    ERROR bb_readdir filler:  buffer full");
		    return -ENOMEM;
		}


	json_object * jobj = json_tokener_parse(data.data);
	arraylen = json_object_array_length(jobj);
	for (i = 0; i < arraylen; i++) {
		json_object* medi_array_obj = json_object_array_get_idx(jobj, i);
		log_msg("calling filler with name %s\n", json_object_get_string(medi_array_obj));
    		if (filler(buf, json_object_get_string(medi_array_obj), NULL, 0) != 0) 
		{
        		log_msg("    ERROR bb_readdir filler:  buffer full");
        		return -ENOMEM;
    		}
//		fprintf(fd,"name : %s\n",json_object_get_string(medi_array_obj));
	}
    
    log_fi(fi);
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
	return 0;
    int retstat = 0;
    
    log_msg("\nbb_releasedir(path=\"%s\", fi=0x%08x)\n",
	    path, fi);
    log_fi(fi);
    
    closedir((DIR *) (uintptr_t) fi->fh);
    
    return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ??? >>> I need to implement this...
int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fsyncdir(path=\"%s\", datasync=%d, fi=0x%08x)\n",
	    path, datasync, fi);
    log_fi(fi);
    
    return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *bb_init(struct fuse_conn_info *conn)
{
    log_msg("\nbb_init()\n");
    
    log_conn(conn);
    log_fuse_context(fuse_get_context());
    
    return ALSFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
    log_msg("\nbb_destroy(userdata=0x%08x)\n", userdata);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
	return 0;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
// Not implemented.  I had a version that used creat() to create and
// open the file, which it turned out opened the file write-only.

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_ftruncate(path=\"%s\", offset=%lld, fi=0x%08x)\n",
	    path, offset, fi);

    return 0;
    log_fi(fi);
    
    retstat = ftruncate(fi->fh, offset);
    if (retstat < 0)
	retstat = log_error("bb_ftruncate ftruncate");
    
    return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    int retstat = 0;
    
    log_msg("\nbb_fgetattr(path=\"%s\", statbuf=0x%08x, fi=0x%08x)\n",
	    path, statbuf, fi);

    int pathCounter=countPathDepth(path);
	if (pathCounter>2 && !strncmp(path,"/adf/DF",7))
	{
		char adfPath[100];
		int trackdevice=get_trackdevice(path);
		sprintf(adfPath,"/adf/DF%d/DF%d.adf",trackdevice,trackdevice);
		if (pathCounter==3 && !strcmp(path,adfPath))
		{
			create_file_element(statbuf,2017,7,4,10,20,30,901120);
			log_msg("Created virtual file of 901120 bytes\n");
			return 0;
		}
		log_msg("### File not found ###\n");
		memset (statbuf,0,sizeof(struct stat));
		return -2;
	}

    
	/*if (countPathDepth(path)>2 && !strncmp(path,"/adf/DF",7))
	{
		create_file_element(statbuf,2017,7,4,10,20,30,901120);
		log_msg("Created virtual file of 901120 bytes\n");
		return 0;
	}*/
	    
	return curl_stat_amiga_file(path,statbuf);

	
    log_fi(fi);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (!strcmp(path, "/"))
	return bb_getattr(path, statbuf);
    
    retstat = fstat(fi->fh, statbuf);
    if (retstat < 0)
	retstat = log_error("bb_fgetattr fstat");
    
    log_stat(statbuf);
    
    return retstat;
}

struct fuse_operations bb_oper = {
  .getattr = bb_getattr,
  .readlink = bb_readlink,
  // no .getdir -- that's deprecated
  .getdir = NULL,
  .mknod = bb_mknod,
  .mkdir = bb_mkdir,
  .unlink = bb_unlink,
  .rmdir = bb_rmdir,
  .symlink = bb_symlink,
  .rename = bb_rename,
  .link = bb_link,
  .chmod = bb_chmod,
  .chown = bb_chown,
  .truncate = bb_truncate,
  .utime = bb_utime,
  .open = bb_open,
  .read = bb_read,
  .write = bb_write,
  /** Just a placeholder, don't set */ // huh???
  .statfs = bb_statfs,
  .flush = bb_flush,
  .release = bb_release,
  .fsync = bb_fsync,
  
#ifdef HAVE_SYS_XATTR_H
  .setxattr = bb_setxattr,
  .getxattr = bb_getxattr,
  .listxattr = bb_listxattr,
  .removexattr = bb_removexattr,
#endif
  
  .opendir = bb_opendir,
  .readdir = bb_readdir,
  .releasedir = bb_releasedir,
  .fsyncdir = bb_fsyncdir,
  .init = bb_init,
  .destroy = bb_destroy,
  .access = bb_access,
  .ftruncate = bb_ftruncate,
  .fgetattr = bb_fgetattr
};

void alsfs_usage()
{
    fprintf(stderr, "usage:  alsfs [FUSE and mount options] amigalinuxserialfilesystemwebserver mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct bb_state* alsfs_data;

    
    if ((getuid() == 0) || (geteuid() == 0)) {
		fprintf(stderr, "Running BBFS as root opens unnacceptable security holes\n");
		//	return 1;
    }

    // See which version of fuse we're running
    fprintf(stderr, "Fuse library version %d.%d\n", FUSE_MAJOR_VERSION, FUSE_MINOR_VERSION);
    
    // Perform some sanity checking on the command line:  make sure
    // there are enough arguments, and that neither of the last two
    // start with a hyphen (this will break if you actually have a
    // rootpoint or mountpoint whose name starts with a hyphen, but so
    // will a zillion other programs)
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-')) alsfs_usage();

	alsfs_data = malloc(sizeof(struct bb_state));
	if (alsfs_data == NULL) 
	{
		perror("main calloc");
		abort();
    }

    // Pull the rootdir out of the argument list and save it in my
    // internal data
    //bb_data->rootdir = realpath(argv[argc-2], NULL);
    //alsfs_data->rootdir = realpath("./rootdir",NULL);
    alsfs_data->alsfs_webserver = strdup(argv[argc-2]);
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    alsfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main\n");
    fuse_stat = fuse_main(argc, argv, &bb_oper, alsfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    return fuse_stat;
}
