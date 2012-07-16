dnl Process this file with autoconf to produce a configure script.
AC_INIT([tamer], [1.3.0])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h tamer/autoconf.h:tamer/autoconf.h.in])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_MAKE_SET

dnl
dnl compilers
dnl

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
if test -n "$GCC" ; then CC="$CC -W -Wall" ; CXX="$CXX -W -Wall" ; fi

AC_LANG_CPLUSPLUS
AC_CACHE_CHECK([whether the C++ compiler works], [ac_cv_cxx_works], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int f(int x) { return x + 1; }]], [[]])],
	[ac_cv_cxx_works=yes], [ac_cv_cxx_works=no])])
if test "$ac_cv_cxx_works" != yes; then
    AC_MSG_ERROR([
=========================================

The C++ compiler doesn't work. Try setting 'CXX' differently.

=========================================])
fi
AC_LANG_C

dnl other programs

AM_PROG_LEX
AC_PROG_YACC
AC_PROG_LIBTOOL

dnl
dnl directory searching
dnl

AC_HEADER_DIRENT

dnl Danger, Will Robinson! dero's autoconf skills are lacking!
if test "x${bindir}" = 'x${exec_prefix}/bin'; then
  if test "x${exec_prefix}" = "xNONE"; then
    if test "x${prefix}" = "xNONE"; then
      bindir="${ac_default_prefix}/bin";
    else
      bindir="${prefix}/bin";
    fi
  else
    bindir="${exec_prefix}";
  fi
fi
if test "x${bindir}" = 'xNONE'; then
  if test "x${prefix}" = "xNONE"; then
    AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, "${ac_default_prefix}/bin", "Package installed binaries destination")
  else
    AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, "${prefix}/bin", "Package installed binaries destination")
  fi
else
  AC_DEFINE_UNQUOTED(PACKAGE_BIN_DIR, "${bindir}", "Package installed binaries destination")
fi
dnl end dero inserted code

dnl
dnl functions such as strerror, working strtod, strtoul, time
dnl headers such as unistd.h
dnl

AC_LANG_C
FIXLIBC_O=''
AC_CHECK_FUNCS(strerror, :, FIXLIBC_O='fixlibc.o')

AC_CACHE_CHECK([whether strtod works], ac_cv_working_strtod,
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

AC_CHECK_FUNCS([strtoul ctime mkstemp ftruncate sigaction waitpid])
AC_CHECK_FUNC([floor], [:], [AC_CHECK_LIB(m, floor)])
AC_CHECK_FUNC([fabs], [:], [AC_CHECK_LIB(m, fabs)])
AC_CHECK_HEADERS([unistd.h fcntl.h sys/time.h sys/wait.h])

AC_SUBST(FIXLIBC_O)


dnl
dnl integer types
dnl

AC_CHECK_HEADERS([inttypes.h], [have_inttypes_h=yes], [have_inttypes_h=no])
AC_CHECK_HEADERS([sys/types.h], [have_sys_types_h=yes], [have_sys_types_h=no])

if test $have_inttypes_h = no -a $have_sys_types_h = yes; then
    AC_CACHE_CHECK([for uintXX_t typedefs], [ac_cv_uint_t],
    [AC_EGREP_HEADER(dnl
changequote(<<,>>)<<(^|[^a-zA-Z_0-9])uint32_t[^a-zA-Z_0-9]>>changequote([,]),
    sys/types.h, ac_cv_uint_t=yes, ac_cv_uint_t=no)])
fi
if test $have_inttypes_h = no -a $have_sys_types_h = yes -a "$ac_cv_uint_t" = no; then
    AC_CACHE_CHECK([for u_intXX_t typedefs], [ac_cv_u_int_t],
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

AC_CHECK_TYPES([uintptr_t], [], [],
[#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
])

AC_CHECK_SIZEOF([void *])
AC_CHECK_SIZEOF([unsigned long])
AC_CHECK_SIZEOF([unsigned int])


dnl
dnl ntohs, ntohl (need them in C++ code)
dnl

AC_LANG_CPLUSPLUS
AC_CHECK_HEADERS([byteorder.h netinet/in.h sys/param.h])
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
dnl C++11 features
dnl

AC_CACHE_CHECK([whether the C++ compiler understands constexpr], [ac_cv_cxx_constexpr], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[constexpr int f(int x) { return x + 1; }]], [[]])],
	[ac_cv_cxx_constexpr=yes], [ac_cv_cxx_constexpr=no])])
if test "$ac_cv_cxx_constexpr" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_CONSTEXPR], [1], [Define if the C++ compiler understands constexpr.])
fi

AC_CACHE_CHECK([whether the C++ compiler understands noexcept], [ac_cv_cxx_noexcept], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int f(int x) noexcept { return x + 1; }]], [[]])],
	[ac_cv_cxx_noexcept=yes], [ac_cv_cxx_noexcept=no])])
if test "$ac_cv_cxx_noexcept" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_NOEXCEPT], [1], [Define if the C++ compiler understands noexcept.])
fi

AC_CACHE_CHECK([whether the C++ compiler understands static_assert], [ac_cv_cxx_static_assert], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[const int f = 2;]], [[static_assert(f == 2, "f should be 2");]])],
	[ac_cv_cxx_static_assert=yes], [ac_cv_cxx_static_assert=no])])
if test "$ac_cv_cxx_static_assert" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_STATIC_ASSERT], [1], [Define if the C++ compiler understands static_assert.])
fi

AC_CACHE_CHECK([whether the C++ compiler understands rvalue references], [ac_cv_cxx_rvalue_references], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int f(int &) { return 1; } int f(int &&) { return 0; }]], [[return f(int());]])],
	[ac_cv_cxx_rvalue_references=yes], [ac_cv_cxx_rvalue_references=no])])
if test "$ac_cv_cxx_rvalue_references" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_RVALUE_REFERENCES], [1], [Define if the C++ compiler understands rvalue references.])
fi

AC_CACHE_CHECK([whether the C++ compiler understands template alias], [ac_cv_cxx_template_alias], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[template <typename T> struct X { typedef T type; }; template <typename T> using Y = X<T>; int f(int x) { return x; }]], [[return f(Y<int>::type());]])],
	[ac_cv_cxx_template_alias=yes], [ac_cv_cxx_template_alias=no])])
if test "$ac_cv_cxx_template_alias" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_TEMPLATE_ALIAS], [1], [Define if the C++ compiler understands template alias.])
fi


dnl
dnl libevent support
dnl

LIBEVENT_LIBS=
AC_ARG_WITH([libevent], [  --with-libevent         include libevent driver], [:], [with_libevent=maybe])
if test "$with_libevent" != no; then
    AC_CHECK_LIB([event], [event_init], [have_libevent=yes])
    AC_CHECK_HEADERS([event.h], [have_event_h=yes])
    if test "$have_libevent" = yes -a "$have_event_h" = yes; then
	AC_DEFINE([HAVE_LIBEVENT], [1], [Define if Tame programs should use libevent.])
	LIBEVENT_LIBS=-levent
    elif test "$with_libevent" = yes; then
	AC_MSG_ERROR([
=========================================

You explicitly requested libevent support, but <event.h> and/or -levent
were not found.

=========================================])
    fi
fi
AC_SUBST([LIBEVENT_LIBS])


dnl
dnl fast malloc support (for tests)
dnl

AC_CHECK_LIB([jemalloc], [malloc], [have_jemalloc=true], [have_jemalloc=])
AC_CHECK_LIB([tcmalloc_minimal], [malloc], [have_tcmalloc_minimal=true], [have_tcmalloc_minimal=])

AC_ARG_ENABLE([malloc],
    [AS_HELP_STRING([--enable-malloc=TYPE],
                    [Memory allocator for tests (malloc|jemalloc|tcmalloc)])],
    [tamer_malloc=$enableval], [tamer_malloc=yes])

if test \( "$tamer_malloc" = tcmalloc -a -z "$have_tcmalloc_minimal" \) \
	-o \( "$tamer_malloc" = jemalloc -a -z "$have_jemalloc" \); then
    AC_MSG_ERROR([$tamer_malloc not found])
elif test "$tamer_malloc" = yes -o "$tamer_malloc" = default; then
    AC_MSG_CHECKING([for malloc library])
    if test -n "$have_jemalloc"; then tamer_malloc=jemalloc;
    elif test -n "$have_tcmalloc_minimal"; then tamer_malloc=tcmalloc;
    else tamer_malloc=malloc; fi
    AC_MSG_RESULT([$tamer_malloc])
elif test "$tamer_malloc" = no -o "$tamer_malloc" = malloc -o -z "$tamer_malloc"; then
    tamer_malloc=malloc
else
    AC_MSG_ERROR([Unknown malloc type $tamer_malloc])
fi

if test "$tamer_malloc" = tcmalloc; then
    MALLOC_LIBS="-ltcmalloc_minimal"
elif test "$tamer_malloc" = jemalloc; then
    MALLOC_LIBS="-ljemalloc"
else
    MALLOC_LIBS=
fi
AC_SUBST([MALLOC_LIBS])


dnl
dnl file descriptor helper support
dnl

AC_ARG_ENABLE([fd-helper], [  --enable-fd-helper      enable experimental fd helper support
                          (for nonblocking disk I/O)])
if test "$enable_fd_helper" = yes; then
    AC_DEFINE([HAVE_TAMER_FDHELPER], [1], [Define if Tame programs should use fdhelper for disk I/O.])
    AC_SUBST([TAMER_FDHELPER_OBJS], ['fdhmsg.$(OBJEXT) fdh.$(OBJEXT)'])
    AC_SUBST([TAMER_FDHELPER_PROGRAM], ['tamerfdh${EXEEXT}'])
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

AC_OUTPUT(Makefile compiler/Makefile tamer/Makefile ex/Makefile knot/Makefile doc/Makefile test/Makefile tamer/tamer.pc)
