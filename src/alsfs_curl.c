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
#include "alsfs_curl.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <fuse.h>
#include "log.h"


long amiga_js_call(const char* endpoint,json_object * jobj,const char* http_method)
{
	long http_code=0;
	char* url;
	
	CURL *curl = curl_easy_init();

	if (asprintf(&url,"http://%s/%s",ALSFS_DATA->alsfs_webserver,endpoint)==-1)
			log_msg("asprintf() failed at file alfs_curl.c:%d",__LINE__);
	curl_easy_setopt(curl, CURLOPT_URL, url);
	free(url);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jobj));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, http_method);

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_perform(curl);
	curl_slist_free_all(headers); /* free the header list */

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(curl);
	
	return http_code;
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
	
	return amiga_js_call(STOREBINARY,jobj,"POST");
}

long curl_post_create_empty_file(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigafilename", jstring);
	
	return amiga_js_call(TOUCH,jobj,"POST");;
}

long curl_post_create_empty_drawer(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigadrawername", jstring);
	
	return amiga_js_call(MKDIR,jobj,"POST");
}

long curl_put_rename_file_drawer(const char* oldname,const char* newname)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(oldname);
	json_object *jstring2 = json_object_new_string(newname);
	json_object_object_add(jobj,"amigaoldfilename", jstring);
	json_object_object_add(jobj,"amiganewfilename", jstring2);
	
	return amiga_js_call(RENAME,jobj,"PUT");
}
