#include <json.h>
#include <curl/curl.h>
#include "alsfs_curl.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

long curl_post(const char* amigadestination,char* data,size_t size,off_t offset)
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

	long http_code=0;
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.137.3:8081/storeBinary");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jobj));
	//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"amigafilename\" : \"ram:setup2\",\"pcfilename\":\"/tmp/lol\"}");
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

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

long curl_post_create_empty_file(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigafilename", jstring);

	long http_code=0;
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.137.3:8081/createEmptyFile");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jobj));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_perform(curl);
	curl_slist_free_all(headers); /* free the header list */

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(curl);
	
	return http_code;
}

long curl_post_create_empty_drawer(const char* amigadestination)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(amigadestination);
	json_object_object_add(jobj,"amigadrawername", jstring);

	long http_code=0;
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.137.3:8081/createEmptyDrawer");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jobj));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_perform(curl);
	curl_slist_free_all(headers); /* free the header list */

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(curl);
	
	return http_code;
}

long curl_put_rename_file_drawer(const char* oldname,const char* newname)
{
	json_object * jobj = json_object_new_object();
	json_object *jstring = json_object_new_string(oldname);
	json_object *jstring2 = json_object_new_string(newname);
	json_object_object_add(jobj,"amigaoldfilename", jstring);
	json_object_object_add(jobj,"amiganewfilename", jstring2);

	long http_code=0;
	CURL *curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.137.3:8081/renameFileOrDrawer");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(jobj));
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_perform(curl);
	curl_slist_free_all(headers); /* free the header list */

    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
	curl_easy_cleanup(curl);
	
	return http_code;
}
