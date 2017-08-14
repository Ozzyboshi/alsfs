bin_PROGRAMS = alsfs
alsfs_SOURCES = alsfs.c log.c log.h  params.h rootelements.h virtual_stat.c virtual_stat.h alsfs_curl.c alsfs_curl.h magic.h
AM_CFLAGS = @FUSE_CFLAGS@ @JSONC_CFLAGS@
LDADD = @FUSE_LIBS@ @CURL_LIBS@ @JSONC_LIBS@ @CRYPTO_LIBS@ @ZIP_LIBS@

