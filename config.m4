dnl $Id$
dnl config.m4 for extension profiler

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(profiler, for profiler support,
dnl Make sure that the comment is aligned:
dnl [  --with-profiler             Include profiler support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(profiler, whether to enable profiler support,
[  --enable-profiler           Enable profiler support])

if test "$PHP_PROFILER" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-profiler -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/profiler.h"  # you most likely want to change this
  dnl if test -r $PHP_PROFILER/$SEARCH_FOR; then # path given as parameter
  dnl   PROFILER_DIR=$PHP_PROFILER
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for profiler files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       PROFILER_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$PROFILER_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the profiler distribution])
  dnl fi

  dnl # --with-profiler -> add include path
  dnl PHP_ADD_INCLUDE($PROFILER_DIR/include)

  dnl # --with-profiler -> check for lib and symbol presence
  dnl LIBNAME=profiler # you may want to change this
  dnl LIBSYMBOL=profiler # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $PROFILER_DIR/lib, PROFILER_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_PROFILERLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong profiler lib version or lib not found])
  dnl ],[
  dnl   -L$PROFILER_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(PROFILER_SHARED_LIBADD)

  PHP_NEW_EXTENSION(profiler, php_profiler.c, $ext_shared)
fi
