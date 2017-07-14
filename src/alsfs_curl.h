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

struct WriteData {
  const char *readptr;
  int sizeleft;
};

struct curl_url_data {
    size_t size;
    char* data;
};

size_t curl_write_data(void *, size_t , size_t , struct curl_url_data *);
long amiga_js_call(const char*,json_object *,const char*,char**);
static size_t read_callback(void *, size_t , size_t , void *);
int Base64Encode(const unsigned char* , size_t , char** );
int Base64Decode(char*, unsigned char**, size_t*);
long curl_post_create_empty_file(const char*);
long curl_post_create_empty_drawer(const char*);
long curl_put_rename_file_drawer(const char*,const char*);
long curl_post_create_mknode(const char*,char*,size_t,off_t);
long curl_get_read_file(const char*,size_t,off_t,char**);
int curl_stat_amiga_file(const char* path,struct stat *statbuf);
char* trans_urlToAmiga(const char*,char*);
int trans_countPathDepth(const char*);
size_t calcDecodeLength(const char*);

unsigned char *base64_decode(const char *,
                             size_t ,
                             size_t *);
void build_decoding_table();
void base64_cleanup();


char *unbase64(unsigned char *, int,int*);
