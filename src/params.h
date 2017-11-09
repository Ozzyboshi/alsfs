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

#ifndef _PARAMS_H_
#define _PARAMS_H_

// The FUSE API has been changed a number of times.  So, our code
// needs to define the version of the API that we assume.  As of this
// writing, the most current API version is 26
#define FUSE_USE_VERSION 26

// need this to get pwrite().  I have to use setvbuf() instead of
// setlinebuf() later in consequence.
//#define _XOPEN_SOURCE 500

// maintain alsfs state in here
#include <limits.h>
#include <stdio.h>
struct bb_state {
    FILE *logfile;
    //char *rootdir;
    char* alsfs_webserver;
};
#define ALSFS_DATA ((struct bb_state *) fuse_get_context()->private_data)

#endif

#define ALSFS_BLK_SIZE 4096

#define AMIGA_DEFAULT_YEAR 1984
#define AMIGA_DEFAULT_MONTH 1
#define AMIGA_DEFAULT_DAY 4
#define AMIGA_DEFAULT_HOUR 0
#define AMIGA_DEFAULT_MINUTE 0
#define AMIGA_DEFAULT_SECOND 0