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
#include <stdio.h>
#include "params.h"
#include "rootelements.h"
#include <json.h>
#include <curl/curl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <string.h>
#include <fuse.h>
#include "log.h"
#include "alsfs_curl.h"
#include <assert.h>
#include <unistd.h>

int CURL_READY=1;

long amiga_js_call(const char* endpoint,json_object * jobj,const char* http_method,char** http_body)
{
	while (!CURL_READY)
	{
		sleep(1);
	}
	CURL_READY=0;
	long http_code=0;
	char* url;
	struct curl_url_data data;
	
	CURL *curl = curl_easy_init();

	if (asprintf(&url,"http://%s/%s",ALSFS_DATA->alsfs_webserver,endpoint)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
	log_msg("%s\n",url);
	if (jobj) log_msg("JSON %s",json_object_get_string(jobj));
	curl_easy_setopt(curl, CURLOPT_URL, url);
	free(url);
	if (jobj!=NULL) curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jobj));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, http_method);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT,0); 
	
	if (http_body)
	{
		data.size = 0;
		data.data = malloc(4096);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
	}

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_perform(curl);
	curl_slist_free_all(headers); /* free the header list */

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(curl);
		
	if (http_code==200 && http_body)
	{
		if (asprintf(http_body,"%s",data.data)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
		free(data.data);
	}
	CURL_READY=1;
	return http_code;
}

size_t curl_write_data(void *ptr, size_t size, size_t nmemb, struct curl_url_data *data) {
	log_msg("Curl write data invoked at - %lu -\n",(unsigned long)time(NULL));
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
        log_msg("Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';
	//log_msg("body of the message : %s",data->data);
    return size * nmemb;
}


static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
  struct WriteData *pooh = (struct WriteData *)userp;

  if(size*nmemb<pooh->sizeleft) {
    *(char *)ptr = pooh->readptr[0]; /* copy one single byte */ 
    pooh->readptr++;                 /* advance pointer */ 
    pooh->sizeleft--;                /* less data left */ 
    return 1;                        /* we return 1 byte at a time! */ 
  }

  return 0;                          /* no more data left to deliver */ 
}

// Ws call to create a new node
long curl_post_create_mknode(const char* amigadestination,char* data,size_t size,off_t offset)
{
	char* base64EncodeOutput;
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	data[size]='\0';
	Base64Encode((const unsigned char*)data, size, &base64EncodeOutput);
	json_object *jstring2 = json_object_new_string(base64EncodeOutput);
	json_object *jstring3 = json_object_new_int((int)size);
	json_object *jstring4 = json_object_new_int((int)offset);
	json_object_object_add(jobj,"amigafilename", jstring);
	json_object_object_add(jobj,"data", jstring2);
	json_object_object_add(jobj,"size", jstring3);
	json_object_object_add(jobj,"offset", jstring4);
	
	return amiga_js_call(STOREBINARY,jobj,"POST",NULL);
}

long curl_post_create_empty_file(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigafilename", jstring);
	
	return amiga_js_call(TOUCH,jobj,"POST",NULL);
}

long curl_post_create_empty_drawer(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigadrawername", jstring);
	
	return amiga_js_call(MKDIR,jobj,"POST",NULL);
}

long curl_post_create_adf(const int trackDevice,const char* adfFilename)
{
	// curl -i  -H "Content-Type: application/json" -X POST -d '{"trackDevice":"2","adfFilename":"/home/ozzy/lotus2.adf","start":"0","end":"79"}' http://192.168.137.3:8081/writeAdf
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(adfFilename);
	json_object *jstring2 = json_object_new_int(trackDevice);
	json_object *jstring3 = json_object_new_int(0);
	json_object *jstring4 = json_object_new_int(79);
	json_object_object_add(jobj,"trackDevice", jstring2);
	json_object_object_add(jobj,"adfFilename", jstring);
	json_object_object_add(jobj,"start", jstring3);
	json_object_object_add(jobj,"end", jstring4);
	
	return amiga_js_call(WRITEADF,jobj,"POST",NULL);
}

long curl_put_rename_file_drawer(const char* oldname,const char* newname)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(oldname);
	json_object *jstring2 = json_object_new_string(newname);
	json_object_object_add(jobj,"amigaoldfilename", jstring);
	json_object_object_add(jobj,"amiganewfilename", jstring2);
	
	return amiga_js_call(RENAME,jobj,"PUT",NULL);
}

long curl_get_read_file(const char* amigadestination,size_t size,off_t offset,char** buf)
{
	char app[100];
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	sprintf(app,"%d",(int)size);
	json_object *jstring3 = json_object_new_string(app);
	sprintf(app,"%d",(int)offset);
	json_object *jstring4 = json_object_new_string(app);
	json_object_object_add(jobj,"amigafilename", jstring);
	json_object_object_add(jobj,"size", jstring3);
	json_object_object_add(jobj,"offset", jstring4);
		
	amiga_js_call(READFILE,jobj,"GET",buf);
	return 200;
}

long curl_get_read_adf(int trackdevice,size_t size,off_t offset,char** buf)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_int(trackdevice);
	json_object *jstring3 = json_object_new_int(size);
	json_object *jstring4 = json_object_new_int(offset);
	json_object_object_add(jobj,"trackDevice", jstring);
	json_object_object_add(jobj,"size", jstring3);
	json_object_object_add(jobj,"offset", jstring4);
		
	amiga_js_call(READADF,jobj,"GET",buf);
	return 200;
}

long curl_delete_delete_file(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigafilename", jstring);
	return amiga_js_call(DELETEFILE,jobj,"DELETE",NULL);
}

int curl_get_test_floppy_disk(int trackDevice,char** buf)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_int(trackDevice);
	json_object_object_add(jobj,"trackDevice", jstring);
	return amiga_js_call(TESTFLOPPYDISK,jobj,"GET",buf);
}

int curl_stat_amiga_file(const char* path,struct stat *statbuf)
{
	char * out;
	CURL *curl;
	struct curl_url_data data;
	int http_code;
	char* url;
	CURLcode res;
	
	data.size=0;
	data.data=malloc(4096);
	
	out=malloc(strlen(path)+1);
	trans_urlToAmiga(path,out);
	char *urlEncoded = curl_easy_escape(curl, out, 0);
	if (urlEncoded) log_msg("urlencoded : ##%s##",urlEncoded);
	if (asprintf(&url,"http://%s/%s?path=%s",ALSFS_DATA->alsfs_webserver,LISTSTAT,urlEncoded)==-1)
		fprintf(stderr, "asprintf() failed");
	curl_free(urlEncoded);
	free(out);
	log_msg("url : ##%s##",url);

	curl = curl_easy_init();
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL,url);
		free(url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK)
			log_msg("curl_easy_perform() failed: %s %d\n",curl_easy_strerror(res),res);
					
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
		curl_easy_cleanup(curl);
	}
	if (http_code == 404)
	{
		log_msg("### File not found ###\n");
		memset (statbuf,0,sizeof(struct stat));
		return -2;
	}

	log_msg("data : ##%s##",data.data);

	json_object * jobj = json_tokener_parse(data.data);
	json_object* returnObj;
	json_object_object_get_ex(jobj, "st_size",&returnObj);
	const char *st_size = json_object_get_string(returnObj);
				
	json_object_object_get_ex(jobj, "directory",&returnObj);
	int directory = atoi(json_object_get_string(returnObj));
	log_msg("stsize : ##%s##",st_size);

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


	memset (statbuf,0,sizeof(struct stat));
	statbuf->st_size=atol(st_size);
	statbuf->st_dev = 0;
	statbuf->st_ino = 0;
	if (directory) statbuf->st_mode = 0040777;
	else statbuf->st_mode = 0100777;
	statbuf->st_nlink = 0;
	statbuf->st_uid = 0;
	statbuf->st_gid = 0;
	statbuf->st_rdev = 0;
	statbuf->st_blksize = ALSFS_BLK_SIZE;
	statbuf->st_blocks = 0;
	statbuf->st_atime = lol;
	statbuf->st_mtime = lol;
	statbuf->st_ctime = lol;
	return 0;
}

char* trans_urlToAmiga(const char* path,char* out)
{
	int depth;
	depth = trans_countPathDepth(path);
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
}
int trans_countPathDepth(const char* path)
{
	int i=0;
	int cont=0;
	for (i=0;i<strlen(path);i++)
		if (path[i]=='/')
			cont++;
	return cont;
}

char *unbase64(unsigned char *input, int length,int* length_out)
{
	BIO *b64, *bmem;

	char *buffer = (char *)malloc(length);
	memset(buffer, 0, length);

	b64 = BIO_new(BIO_f_base64());
	bmem = BIO_new_mem_buf(input, length);
	bmem = BIO_push(b64, bmem);

	*length_out=BIO_read(bmem, buffer, length);

	BIO_free_all(bmem);

	return buffer;
}

int Base64Encode(const unsigned char* buffer, size_t length, char** b64text) { //Encodes a binary safe base 64 string
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Ignore newlines - write everything in one line
	BIO_write(bio, buffer, length);
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);
	BIO_set_close(bio, BIO_NOCLOSE);
	BIO_free_all(bio);

	*b64text=(*bufferPtr).data;

	return (0); //success
}

size_t calcDecodeLength(const char* b64input) { //Calculates the length of a decoded string
	size_t len = strlen(b64input),
		padding = 0;

	if (b64input[len-1] == '=' && b64input[len-2] == '=') //last two chars are =
		padding = 2;
	else if (b64input[len-1] == '=') //last char is =
		padding = 1;

	return (len*3)/4 - padding;
}

int Base64Decode(char* b64message, unsigned char** buffer, size_t* length) { //Decodes a base64 encoded string
	BIO *bio, *b64;

	int decodeLen = calcDecodeLength(b64message);
	*buffer = (unsigned char*)malloc(decodeLen + 1);
	(*buffer)[decodeLen] = '\0';

	bio = BIO_new_mem_buf(b64message, -1);
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); //Do not use newlines to flush buffer
	*length = BIO_read(bio, *buffer, strlen(b64message));
	assert(*length == decodeLen); //length should equal decodeLen, else something went horribly wrong
	BIO_free_all(bio);

	return (0); //success
}