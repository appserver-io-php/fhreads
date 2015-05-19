dnl $Id$
dnl config.m4 for extension fhreads

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(fhreads, for fhreads support,
dnl Make sure that the comment is aligned:
dnl [  --with-fhreads             Include fhreads support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(fhreads, whether to enable fhreads support,
[  --enable-fhreads           Enable fhreads support])

if test "$PHP_FHREADS" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-fhreads -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/fhreads.h"  # you most likely want to change this
  dnl if test -r $PHP_FHREADS/$SEARCH_FOR; then # path given as parameter
  dnl   FHREADS_DIR=$PHP_FHREADS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for fhreads files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       FHREADS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$FHREADS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the fhreads distribution])
  dnl fi

  dnl # --with-fhreads -> add include path
  dnl PHP_ADD_INCLUDE($FHREADS_DIR/include)

  dnl # --with-fhreads -> check for lib and symbol presence
  dnl LIBNAME=fhreads # you may want to change this
  dnl LIBSYMBOL=fhreads # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FHREADS_DIR/$PHP_LIBDIR, FHREADS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_FHREADSLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong fhreads lib version or lib not found])
  dnl ],[
  dnl   -L$FHREADS_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(FHREADS_SHARED_LIBADD)

  PHP_NEW_EXTENSION(fhreads, fhreads.c, $ext_shared)
fi
