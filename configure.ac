dnl Process this file with autoconf to produce a configure script.
AC_INIT([tamer],[1.7.0])
AC_PREREQ([2.69])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h tamer/autoconf.h:tamer/autoconf.h.in])
AC_CONFIG_FILES([Makefile compiler/Makefile tamer/Makefile ex/Makefile knot/Makefile doc/Makefile test/Makefile bench/Makefile tamer/tamer.pc])
AM_INIT_AUTOMAKE

AC_PROG_MAKE_SET

dnl
dnl compilers
dnl

ac_user_cxx=${CXX+y}
ac_user_cxxflags=${CXXFLAGS+y}

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_CXXCPP
if test -n "$GCC"; then CC="$CC -W -Wall"; CXX="$CXX -W -Wall"; fi

AC_LANG([C++])
AC_CACHE_CHECK([whether the C++ compiler works], [ac_cv_cxx_works], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int f(int x) { return x + 1; }]], [[]])],
        [ac_cv_cxx_works=yes], [ac_cv_cxx_works=no])])
if test "$ac_cv_cxx_works" != yes; then
    AC_MSG_ERROR([
=========================================

The C++ compiler doesn't work. Try setting 'CXX' differently.

=========================================])
fi
AC_LANG([C])


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

AC_LANG([C])
FIXLIBC_O=''
AC_CHECK_FUNCS(strerror, :, FIXLIBC_O='fixlibc.o')

AC_CACHE_CHECK([whether strtod works], ac_cv_working_strtod,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[#include <stdio.h>
#include <stdlib.h>
int main(int c, char **v) {
  char s[] = "12 ";
  char *endp;
  double d = strtod(s, &endp);
  exit(*endp == 0 ? 1 : 0);
}]])],[ac_cv_working_strtod=yes],[ac_cv_working_strtod=no],[ac_cv_working_strtod=yes])])
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

AC_LANG([C++])
AC_CHECK_HEADERS([byteorder.h netinet/in.h sys/param.h sys/epoll.h])
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
AC_LANG([C])


dnl
dnl is va_list addressable?
dnl

AC_LANG([C++])
AC_CACHE_CHECK([for addressable va_list type],
    ac_cv_va_list_addr,
    [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <stdarg.h>
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
}]], [[h(2, 3, 4);]])],[ac_cv_va_list_addr=yes],[ac_cv_va_list_addr=no])])
if test "x$ac_cv_va_list_addr" = xyes; then
    AC_DEFINE([HAVE_ADDRESSABLE_VA_LIST], [1], [Define if the va_list type is addressable.])
fi


dnl
dnl C++11 features
dnl
SAVE_CXX="$CXX"
if test -n "$GCC"; then CXX="$CXX -Werror"; fi

AC_CACHE_CHECK([whether the C++ compiler understands rvalue references], [ac_cv_cxx_rvalue_references], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int f(int &) { return 1; } int f(int &&) { return 0; }]], [[return f(int());]])],
        [ac_cv_cxx_rvalue_references=yes], [ac_cv_cxx_rvalue_references=no])])
if test "$ac_cv_cxx_rvalue_references" != yes; then
    SAVE_CXX="${SAVE_CXX} -std=gnu++17"
    CXX="${CXX} -std=gnu++17"
    AC_MSG_CHECKING([whether the C++ compiler with -std=gnu++17 understands rvalue refs])
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[int f(int &) { return 1; } int f(int &&) { return 0; }]], [[return f(int());]])],
        [ac_cv_cxx_rvalue_references=yes], [ac_cv_cxx_rvalue_references=no])
    AC_MSG_RESULT([$ac_cv_cxx_rvalue_references])
fi
if test "$ac_cv_cxx_rvalue_references" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_RVALUE_REFERENCES], [1], [Define if the C++ compiler understands rvalue references.])
fi

AC_MSG_CHECKING([for std::move])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <utility>], [[long x = 0; long &&y = std::move(x); (void) y;]])], [ac_cv_std_move=yes], [ac_cv_std_move=no])
AC_MSG_RESULT([$ac_cv_std_move])

AC_MSG_CHECKING([for variadic templates])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <utility>
#include <cstddef>
template <typename... TS> struct foo { static constexpr size_t count = sizeof...(TS); };], [[long x = foo<>::count + foo<int, int, int, int, int>::count; (void) x;]])], [ac_cv_variadic_templates=yes], [ac_cv_variadic_templates=no])
AC_MSG_RESULT([$ac_cv_variadic_templates])

if test "$ac_cv_std_move" != yes -o "$ac_cv_variadic_templates" != yes; then
    AC_MSG_ERROR([

The C++ compiler does not appear to understand C++17.
To fix this problem, try supplying a "CXX" argument to ./configure,
such as "./configure CXX='c++ -std=gnu++17'".
If you are using clang on Mac, ensure the '-stdlib=libc++' option.
])
fi

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

AC_CACHE_CHECK([whether the C++ compiler understands template alias], [ac_cv_cxx_template_alias], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[template <typename T> struct X { typedef T type; }; template <typename T> using Y = X<T>; int f(int x) { return x; }]], [[return f(Y<int>::type());]])],
        [ac_cv_cxx_template_alias=yes], [ac_cv_cxx_template_alias=no])])
if test "$ac_cv_cxx_template_alias" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_TEMPLATE_ALIAS], [1], [Define if the C++ compiler understands template alias.])
fi

AC_CACHE_CHECK([whether the C++ compiler understands variadic templates], [ac_cv_cxx_variadic_templates], [
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[template <typename... Ts> struct X; template <> struct X<> { static constexpr int n = 0; }; template <typename T, typename... Ts> struct X<T, Ts...> { static constexpr int n = X<Ts...>::n + 1; };]], [[switch (0) { case 0: case X<>::n == 0 && X<int>::n == 1 && X<int, int>::n == 2: break; }]])],
        [ac_cv_cxx_variadic_templates=yes], [ac_cv_cxx_variadic_templates=no])])
if test "$ac_cv_cxx_variadic_templates" = yes; then
    AC_DEFINE([TAMER_HAVE_CXX_VARIADIC_TEMPLATES], [1], [Define if the C++ compiler understands variadic templates.])
fi

CXX="$SAVE_CXX"


dnl
dnl select information
dnl

AC_MSG_CHECKING([size of fd_set element])
AC_COMPUTE_INT([ac_fd_set_element_size], [[sizeof(fds.fds_bits[0])]],
               [AC_INCLUDES_DEFAULT
[static fd_set fds;]], [ac_fd_set_element_size=-1])
if test "$ac_fd_set_element_size" = -1; then
    AC_COMPUTE_INT([ac_fd_set_element_size], [[sizeof(fds.__fds_bits[0])]],
                   [AC_INCLUDES_DEFAULT
[static fd_set fds;]], [ac_fd_set_element_size=-1])
fi
AC_MSG_RESULT([$ac_fd_set_element_size])
AC_DEFINE_UNQUOTED([SIZEOF_FD_SET_ELEMENT], [$ac_fd_set_element_size], [The size of an `fd_set` element, as computed by sizeof.])


dnl
dnl libevent support
dnl

DRIVER_LIBS=
AC_ARG_WITH([libevent], 
    [AS_HELP_STRING([--with-libevent], [include libevent driver])],
    [:], [with_libevent=maybe])
if test "$with_libevent" != no; then
    AC_CHECK_LIB([event], [event_init], [have_libevent=yes])
    AC_CHECK_HEADERS([event.h], [have_event_h=yes])
    if test "$have_libevent" = yes -a "$have_event_h" = yes; then
        AC_DEFINE([HAVE_LIBEVENT], [1], [Define if tamed programs may use libevent.])
        DRIVER_LIBS="$DRIVER_LIBS -levent"
        AC_CHECK_DECLS([event_get_struct_event_size], [], [], [#include <event.h>])
    elif test "$with_libevent" = yes; then
        AC_MSG_ERROR([
=========================================

You explicitly requested libevent support, but <event.h> and/or -levent
were not found.

=========================================])
    fi
fi


dnl
dnl libev support
dnl

AC_ARG_WITH([libev],
        [AS_HELP_STRING([--with-libev], [include libev driver])],
        [:], [with_libev=maybe])
if test "$with_libev" != no; then
    AC_CHECK_LIB([ev], [ev_run], [have_libev=yes])
    AC_CHECK_HEADERS([ev.h], [have_ev_h=yes])
    if test "$have_libev" = yes -a "$have_ev_h" = yes; then
        AC_DEFINE([HAVE_LIBEV], [1], [Define if tamed programs may use libev.])
        DRIVER_LIBS="$DRIVER_LIBS -lev"
    elif test "$with_libev" = yes; then
        AC_MSG_ERROR([
=========================================

You explicitly requested libev support, but <ev.h> and/or -lev
were not found.

=========================================])
    fi
fi


dnl
dnl mbedtls support
dnl

AC_ARG_WITH([mbedtls],
    [AS_HELP_STRING([--with-mbedtls], [include mbedtls crypto/SSL support])],
    [:], [with_mbedtls=maybe])
if test "$with_mbedtls" != no; then
    AC_CHECK_LIB([mbedcrypto], [mbedtls_entropy_init], [have_libmbedtls=yes])
    AC_CHECK_HEADERS([mbedtls/entropy.h], [have_mbedtls_entropy_h=yes])
    if test "$have_libmbedtls" = yes -a "$have_mbedtls_entropy_h" = yes; then
        AC_DEFINE([HAVE_LIBMBEDTLS], [1], [Define if tamed programs may use libmbedtls.])
        DRIVER_LIBS="$DRIVER_LIBS -lmbedcrypto"
        with_mbedtls=yes
    elif test "$with_mbedtls" = yes; then
        AC_MSG_ERROR([
=========================================

You explicitly requested libmbedtls support, but <mbedtls/*.h> and/or
-lmbedcrypto were not found.

=========================================])
    fi
fi
AM_CONDITIONAL([MBEDTLS], [test x$with_mbedtls = xyes])

AC_SUBST([DRIVER_LIBS])
AC_DEFINE_UNQUOTED([TAMER_LIBS], ["]$DRIVER_LIBS["], [Define to a C string with the libraries required to link a tamed program.])


dnl
dnl http-parser support
dnl

AC_ARG_WITH([http-parser],
        [AS_HELP_STRING([--without-http-parser], [don't include http-parser library])],
        [:], [with_http_parser=yes])
if test "$with_http_parser" = yes; then
    AC_DEFINE([TAMER_HTTP_PARSER], [1], [Define if http-parser is available.])
fi
AM_CONDITIONAL([HTTP_PARSER], [test x$with_http_parser = xyes])


dnl
dnl fast malloc support (for tests)
dnl

AC_CHECK_LIB([jemalloc], [malloc], [have_jemalloc=true], [have_jemalloc=])
AC_CHECK_LIB([tcmalloc_minimal], [malloc], [have_tcmalloc_minimal=true], [have_tcmalloc_minimal=])

AC_ARG_WITH([malloc],
    [AS_HELP_STRING([--with-malloc=TYPE],
                    [Memory allocator for tests (malloc|jemalloc|tcmalloc)])],
    [tamer_malloc=$withval], [tamer_malloc=yes])

if test \( "$tamer_malloc" = tcmalloc -a -z "$have_tcmalloc_minimal" \) \
        -o \( "$tamer_malloc" = jemalloc -a -z "$have_jemalloc" \); then
    AC_MSG_ERROR([$tamer_malloc not found])
elif test "$tamer_malloc" = yes -o "$tamer_malloc" = default \
        -o "$tamer_malloc" = no -o "$tamer_malloc" = malloc \
        -o -z "$tamer_malloc"; then
    tamer_malloc=malloc
elif test "$tamer_malloc" = jemalloc -o "$tamer_malloc" = tcmalloc; then
    :
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


AC_ARG_ENABLE([epoll], [AS_HELP_STRING([--disable-epoll], [do not use epoll])], [], [enable_epoll=yes])
if test "$enable_epoll" = no; then
    AC_DEFINE([TAMER_NOEPOLL], [1], [Define to disable epoll.])
fi


dnl
dnl file descriptor helper support
dnl

AC_ARG_ENABLE([fd-helper], [  --enable-fd-helper      enable experimental fd helper support
                          (for nonblocking disk I/O)])
if test "$enable_fd_helper" = yes; then
    AC_DEFINE([HAVE_TAMER_FDHELPER], [1], [Define if Tamer programs should use fdhelper for disk I/O.])
    AC_SUBST([TAMER_FDHELPER_PROGRAM], ['tamerfdh${EXEEXT}'])
fi
AM_CONDITIONAL([FDHELPER], [test x$enable_fd_helper = xyes])


dnl
dnl debugging and event tracing
dnl

AC_ARG_ENABLE([tracing], [AS_HELP_STRING([--disable-tracing], [do not trace events])], [], [enable_tracing=yes])
if test "$enable_tracing" = no; then
    AC_DEFINE([TAMER_NOTRACE], [1], [Define to disable event tracing.])
fi

AC_ARG_ENABLE([debug], [AS_HELP_STRING([--enable-debug], [include more debugging assertions])])
if test "$enable_debug" = yes; then
    AC_DEFINE([TAMER_DEBUG], [1], [Define to include more debugging assertions.])
fi
AM_CONDITIONAL([TAMER_DEBUG], [test x$enable_debug = xyes])

AC_ARG_ENABLE([sanitizers], [AS_HELP_STRING([--enable-sanitizers], [enable sanitizers in library and test])], [], [enable_sanitizers=no])
SANITIZER_FLAGS=
if test x"$enable_sanitizers" = xyes; then
    SANITIZER_FLAGS="-fsanitize=undefined -fsanitize=address"
elif test -n "$enable_sanitizers" -a x"$enable_sanitizers" != xno; then
    SANITIZER_FLAGS="$enable_sanitizers"
    enable_sanitizers=yes
fi
AC_SUBST([SANITIZER_FLAGS])
AM_CONDITIONAL([TAMER_SANITIZERS], [test x$enable_sanitizers = xyes])



dnl
dnl lex, yacc, libtool
dnl

AC_PROG_LEX(noyywrap)
AC_PROG_YACC
LT_INIT


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

AC_OUTPUT
