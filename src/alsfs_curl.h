struct WriteData {
  const char *readptr;
  int sizeleft;
};

long curl_post(const char*,char*,size_t,off_t);
static size_t read_callback(void *, size_t , size_t , void *);
int Base64Encode(const unsigned char* , size_t , char** );
long curl_post_create_empty_file(const char*);
long curl_post_create_empty_drawer(const char*);

