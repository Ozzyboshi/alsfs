#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([fuse-tutorial], [2016-03-25], [joseph@pfeifferfamily.net])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/alsfs.c])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CC

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/statvfs.h unistd.h utime.h sys/xattr.h curl/curl.h json/json.h])

# Check for FUSE development environment
PKG_CHECK_MODULES(FUSE, fuse)
PKG_CHECK_MODULES(CURL, libcurl)
PKG_CHECK_MODULES(JSONC, json-c)
PKG_CHECK_MODULES(CRYPTO, libcrypto)
PKG_CHECK_MODULES(ZIP, libzip)


AC_CHECK_HEADERS([magic.h], [], [AC_MSG_ERROR([libmagic magic.h header file not found])])
AC_CHECK_LIB([magic], [magic_open], [AC_SUBST([LIBMAGIC], [-lmagic])], [AC_MSG_ERROR([libmagic library or magic_open function not found])], [-lz])

AC_CHECK_HEADER([magic.h], [], [
  AC_MSG_ERROR([required header magic.h not found])
])
AC_SEARCH_LIBS([magic_open], [magic], [], [
  AC_MSG_ERROR([required library libmagic not found])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([ftruncate mkdir mkfifo realpath rmdir strerror utime])

# Not all systems that support FUSE also support fdatasync (notably freebsd)
AC_CHECK_FUNCS([fdatasync])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
