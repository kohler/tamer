dnl Process this file with autoconf to produce a configure script.
AC_INIT(configure.ac)
AM_INIT_AUTOMAKE(tamer, 0.1)
AM_CONFIG_HEADER(tamer/config.h)
AM_MAINTAINER_MODE

AC_PROG_MAKE_SET

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL
if test -n "$GCC" ; then CC="$CC -W -Wall" ; CXX="$CXX -W -Wall" ; fi

dnl AN_MAKEVAR([AR], [AC_PROG_AR])
dnl AN_PROGRAM([ar], [AC_PROG_AR])
dnl AC_DEFUN([AC_PROG_AR], [AC_CHECK_TOOL(AR, ar, :)])
dnl AC_PROG_AR


dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ac_cxx_template_objs.html
dnl
AC_DEFUN([AC_CXX_TEMPLATE_OBJS],
[AC_CACHE_CHECK(where template objects are stored, ac_cv_cxx_templobjs,
 [ ac_cv_cxx_templobjs='unknown'
   if test "$GXX" = yes; then
     ac_cv_cxx_templobjs='nowhere'
   else
     case $CXX in
       CC|*/CC)
        cat > conftest.cc <<EOF
template<class T> class A { public : A () {} };
template<class T> void f (const A<T>&) {}
main()
{ A<double> d;
  A<int> i;
  f (d);
  f (i);
  return 0;
}
EOF
        if test "$ac_cv_cxx_templobjs" = 'unknown' ; then
          if test -d Templates.DB ; then
            rm -fr Templates.DB
          fi
          if $CXX $CXXFLAGS -ptr. -c conftest.cc 1> /dev/null 2>&1; then
            if test -d Templates.DB ; then
#             this should be Sun CC <= 4.2
              CXXFLAGS="$CXXFLAGS -ptr."
              if test x"$LIBTOOL" = x ; then
                ac_cv_cxx_templobjs='Templates.DB/*.o'
              else
                ac_cv_cxx_templobjs='Templates.DB/*.lo'
              fi
              rm -fr Templates.DB
            fi
          fi
        fi
        if test "$ac_cv_cxx_templobjs" = 'unknown' ; then
          if test -d SunWS_cache ; then
            rm -fr SunWS_cache
          fi
          if $CXX $CXXFLAGS -c conftest.cc 1> /dev/null 2>&1; then
            if test -d SunWS_cache ; then
#             this should be Sun WorkShop C++ compiler 5.x
#             or Sun Forte C++ compiler >= 6.x
              if test x"$LIBTOOL" = x ; then
                ac_cv_cxx_templobjs='SunWS_cache/*/*.o'
              else
                ac_cv_cxx_templobjs='SunWS_cache/*/*.lo'
              fi
              rm -fr SunWS_cache
            fi
          fi
        fi
        rm -f conftest* ;;
     esac
   fi
   case "x$ac_cv_cxx_templobjs" in
     xunknown|xnowhere)
     TEMPLATE_OBJS="" ;;
     *)
     TEMPLATE_OBJS="$ac_cv_cxx_templobjs" ;;
   esac
   AC_SUBST(TEMPLATE_OBJS)])])

AC_CXX_TEMPLATE_OBJS


dnl
dnl directory searching
dnl

AC_HEADER_DIRENT


dnl
dnl <new> and/or <new.h>
dnl

AC_LANG_CPLUSPLUS
AC_CACHE_CHECK([whether <new> works], ac_cv_good_new_hdr,
[AC_TRY_LINK([#include <new>], [
  int a;
  int *b = new(&a) int;
  return 0;
], ac_cv_good_new_hdr=yes, ac_cv_good_new_hdr=no)])
if test "$ac_cv_good_new_hdr" = yes; then
   AC_DEFINE(HAVE_NEW_HDR, 1, [Define if <new> exists and works.])
fi
AC_CHECK_HEADERS(new.h)


dnl
dnl functions such as strerror, working strtod, strtoul, time
dnl headers such as unistd.h
dnl

AC_LANG_C
FIXLIBC_O=''
AC_CHECK_FUNCS(strerror, :, FIXLIBC_O='fixlibc.o')

AC_CACHE_CHECK(whether strtod works, ac_cv_working_strtod,
[AC_TRY_RUN([#include <stdio.h>
#include <stdlib.h>
int main(int c, char **v) {
  char s[] = "12 ";
  char *endp;
  double d = strtod(s, &endp);
  exit(*endp == 0 ? 1 : 0);
}], ac_cv_working_strtod=yes, ac_cv_working_strtod=no,
ac_cv_working_strtod=yes)])
if test $ac_cv_working_strtod = no; then 
    FIXLIBC_O='fixlibc.o'
    AC_DEFINE(BROKEN_STRTOD, 1, [Define if strtod is broken.])
fi

AC_CHECK_FUNCS(strtoul ctime mkstemp ftruncate sigaction waitpid)
AC_CHECK_FUNC(floor, :, [AC_CHECK_LIB(m, floor)])
AC_CHECK_FUNC(fabs, :, [AC_CHECK_LIB(m, fabs)])
AC_CHECK_HEADERS(unistd.h fcntl.h sys/time.h sys/wait.h)

AC_SUBST(FIXLIBC_O)


dnl
dnl integer types
dnl

AC_CHECK_HEADERS(inttypes.h, have_inttypes_h=yes, have_inttypes_h=no)
AC_CHECK_HEADERS(sys/types.h, have_sys_types_h=yes, have_sys_types_h=no)

if test $have_inttypes_h = no -a $have_sys_types_h = yes; then
    AC_CACHE_CHECK([for uintXX_t typedefs], ac_cv_uint_t,
    [AC_EGREP_HEADER(dnl
changequote(<<,>>)<<(^|[^a-zA-Z_0-9])uint32_t[^a-zA-Z_0-9]>>changequote([,]),
    sys/types.h, ac_cv_uint_t=yes, ac_cv_uint_t=no)])
fi
if test $have_inttypes_h = no -a $have_sys_types_h = yes -a "$ac_cv_uint_t" = no; then
    AC_CACHE_CHECK([for u_intXX_t typedefs], ac_cv_u_int_t,
    [AC_EGREP_HEADER(dnl
changequote(<<,>>)<<(^|[^a-zA-Z_0-9])u_int32_t[^a-zA-Z_0-9]>>changequote([,]),
    sys/types.h, ac_cv_u_int_t=yes, ac_cv_u_int_t=no)])
fi
if test $have_inttypes_h = yes -o "$ac_cv_uint_t" = yes; then
    :
elif test "$ac_cv_u_int_t" = yes; then
    AC_DEFINE(HAVE_U_INT_TYPES, 1, [Define if you have u_intXX_t types but not uintXX_t types.])
else
    AC_MSG_WARN([
=========================================

Neither uint32_t nor u_int32_t defined by <inttypes.h> or <sys/types.h>!
Assuming "short" has 16 bits and "int" has 32 bits.

=========================================])
    AC_DEFINE(HAVE_FAKE_INT_TYPES, 1, [Define if intXX_t types are not available.])
fi

AC_CHECK_TYPES(uintptr_t, [], [],
[#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
])

AC_CHECK_SIZEOF(void *)
AC_CHECK_SIZEOF(unsigned long)
AC_CHECK_SIZEOF(unsigned int)


dnl
dnl ntohs, ntohl (need them in C++ code)
dnl

AC_LANG_CPLUSPLUS
AC_CHECK_HEADERS(byteorder.h netinet/in.h sys/param.h)
AC_MSG_CHECKING([whether ntohs and ntohl are defined])
ac_ntoh_defined=no
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM([[#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_BYTEORDER_H
# include <byteorder.h>
#elif HAVE_NETINET_IN_H
# include <netinet/in.h>
#elif HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
]], [[(void) ntohs(0x0020), (void) ntohl(0x03040020);]])],
	[AC_MSG_RESULT(yes)
ac_ntoh_defined=yes],
	[AC_MSG_RESULT(no)])
if test $ac_ntoh_defined = no; then
    AC_CHECK_HEADERS([arpa/inet.h], [have_arpa_inet_h=yes], [have_arpa_inet_h=no])
    if test $have_arpa_inet_h = yes; then
	AC_MSG_CHECKING([whether ntohs and ntohl are defined in <arpa/inet.h>])
	AC_COMPILE_IFELSE(
		[AC_LANG_PROGRAM([[#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include <arpa/inet.h>
]], [[(void) ntohs(0x0020), (void) ntohl(0x03040020);]])],
		[AC_MSG_RESULT(yes)
AC_DEFINE(NEED_ARPA_INET_H, 1, [Define to 1 if you must include <arpa/inet.h> to get `ntohl'.])
ac_ntoh_defined=yes],
		[AC_MSG_RESULT(no)])
    fi
fi
if test $ac_ntoh_defined = no; then
    AC_MSG_ERROR([
=========================================

Cannot find a definition for ntohs and/or ntohl!

=========================================])
fi
AC_LANG_C


dnl
dnl is va_list addressable?
dnl

AC_LANG_CPLUSPLUS
AC_CACHE_CHECK([for addressable va_list type], 
    ac_cv_va_list_addr,
    [AC_TRY_COMPILE([#include <stdarg.h>
void f(va_list *) {
}
void g(va_list val) {
    f(&val);
}
void h(int a, ...) {
    va_list val;
    va_start(val, a);
    g(val);
    va_end(val);
}], [h(2, 3, 4);], ac_cv_va_list_addr=yes, ac_cv_va_list_addr=no)])
if test "x$ac_cv_va_list_addr" = xyes; then
    AC_DEFINE(HAVE_ADDRESSABLE_VA_LIST, 1, [Define if the va_list type is addressable.])
fi


dnl
dnl verbatim portions of the header
dnl

AH_TOP([#ifndef TAMER_CONFIG_H
#define TAMER_CONFIG_H])

AH_BOTTOM([/* Allow compilation on Windows (thanks, Fabrice Popineau). */
#ifdef WIN32
# include <win32lib.h>
#else
# define CDECL /* nothing */
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Prototype strerror if we don't have it. */
#if !HAVE_STRERROR
char *strerror(int errno);
#endif

/* Prototype good_strtod if we need it. */
#if BROKEN_STRTOD
double good_strtod(const char *nptr, char **endptr);
#endif

#ifdef __cplusplus
}
/* Get rid of a possible inline macro under C++. */
# define inline inline
#endif

#endif /* TAMER_CONFIG_H */])


dnl
dnl set path variables
dnl

shell_expand () {
  val=`eval echo '$'"$1"`
  while echo "x$val" | fgrep '$' >/dev/null 2>&1; do val=`eval echo "$val"`; done
  eval "$1='$val'"
}

dnl Preset $prefix and $exec_prefix.
test "x$prefix" = xNONE && prefix=$ac_default_prefix
test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'

shell_expand datadir


dnl
dnl Output
dnl

AC_OUTPUT(Makefile compiler/Makefile tamer/Makefile)
