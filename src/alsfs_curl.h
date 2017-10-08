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
long curl_post_create_empty_file(const char*);
long curl_post_create_empty_drawer(const char*);
long curl_put_rename_file_drawer(const char*,const char*);
long curl_put_relabel(const char* oldname,const char* newname);
long curl_post_create_adf(const int,const char*);
long curl_post_create_adf_b64(const int,const char*);
long curl_post_create_mknode(const char*,char*,size_t,off_t);
long curl_get_read_file(const char*,size_t,off_t,char**);
long curl_get_stat(const char*,char** );
long curl_get_read_adf(int ,size_t ,off_t ,char**);
int curl_stat_amiga_file(const char* path,struct stat *statbuf);
int curl_get_test_floppy_disk(int,char**);
long curl_get_list_floppies(char**);
char* trans_urlToAmiga(const char*,char*);
int trans_countPathDepth(const char*);
size_t calcDecodeLength(const char*);
long curl_delete_delete_file(const char*);
void build_decoding_table();
void base64_cleanup();


char *unbase64(unsigned char *, int,int*);

int Base64Decode(char* , unsigned char** , size_t* );
size_t calcDecodeLength(const char* );

#ifndef b64_malloc
#  define b64_malloc(ptr) malloc(ptr)
#endif
#ifndef b64_realloc
#  define b64_realloc(ptr, size) realloc(ptr, size)
#endif
extern void* b64_malloc(size_t);
extern void* b64_realloc(void*, size_t);

char * b64_encode (const unsigned char *, size_t );

static const char b64_table[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '+', '/'
};
