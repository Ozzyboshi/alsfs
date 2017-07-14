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


#define LISTVOLUMES "listVolumes"
#define LISTCONTENT "listContent"
#define LISTSTAT "stat"
#define STOREBINARY "storeBinary"
#define RENAME "renameFileOrDrawer"
#define MKDIR "createEmptyDrawer"
#define TOUCH "createEmptyFile"
#define READFILE "readFile"

#define ROOTDIRELEMENTS_NUMBER 3
static const char *ROOTDIRELEMENTS[ROOTDIRELEMENTS_NUMBER]={"volumes","adf","hdf"};


