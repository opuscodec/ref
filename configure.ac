dnl Process this file with autoconf to produce a configure script. -*-m4-*-

AC_INIT(src/opus_encoder.c)

AM_CONFIG_HEADER([config.h])

OPUS_MAJOR_VERSION=0
OPUS_MINOR_VERSION=9
OPUS_MICRO_VERSION=4
OPUS_EXTRA_VERSION=
OPUS_VERSION=$OPUS_MAJOR_VERSION.$OPUS_MINOR_VERSION.$OPUS_MICRO_VERSION$OPUS_EXTRA_VERSION
LIBOPUS_SUFFIX=0

OPUS_LT_CURRENT=0
OPUS_LT_REVISION=0
OPUS_LT_AGE=0

AC_SUBST(OPUS_LT_CURRENT)
AC_SUBST(OPUS_LT_REVISION)
AC_SUBST(OPUS_LT_AGE)
AC_SUBST(LIBOPUS_SUFFIX)

# For automake.
VERSION=$OPUS_VERSION
PACKAGE=opus

AC_SUBST(OPUS_VERSION)

AM_INIT_AUTOMAKE($PACKAGE, $VERSION, no-define)
AM_MAINTAINER_MODE

AC_CANONICAL_HOST
AM_PROG_LIBTOOL

AC_PROG_CC_C99
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT

AC_DEFINE([OPUS_BUILD], [], [This is a build of OPUS])

AC_MSG_CHECKING(for C99 variable-size arrays)
AC_TRY_COMPILE( , [
int foo=10;
int array[foo];
],
[has_var_arrays=yes;AC_DEFINE([VAR_ARRAYS], [], [Use C99 variable-size arrays])
],
has_var_arrays=no
)
AC_MSG_RESULT($has_var_arrays)

AC_CHECK_HEADERS([alloca.h getopt.h])
AC_MSG_CHECKING(for alloca)
AC_TRY_COMPILE( [#include <alloca.h>], [
int foo=10;
int *array = alloca(foo);
],
[
has_alloca=yes;
if test x$has_var_arrays = "xno" ; then
AC_DEFINE([USE_ALLOCA], [], [Make use of alloca])
fi
],
has_alloca=no
)
AC_MSG_RESULT($has_alloca)

AC_CHECK_LIB(m, sin)

AC_DEFINE_UNQUOTED(OPUS_VERSION, "${OPUS_VERSION}", [Complete version string])
AC_DEFINE_UNQUOTED(OPUS_MAJOR_VERSION, ${OPUS_MAJOR_VERSION}, [Version major])
AC_DEFINE_UNQUOTED(OPUS_MINOR_VERSION, ${OPUS_MINOR_VERSION}, [Version minor])
AC_DEFINE_UNQUOTED(OPUS_MICRO_VERSION, ${OPUS_MICRO_VERSION}, [Version micro])
AC_DEFINE_UNQUOTED(OPUS_EXTRA_VERSION, "${OPUS_EXTRA_VERSION}", [Version extra])

has_float_approx=no
#case "$host_cpu" in
#i[[3456]]86 | x86_64 | powerpc64 | powerpc32 | ia64)
#  has_float_approx=yes
#  ;;
#esac    

ac_enable_fixed="no";
AC_ARG_ENABLE(fixed-point, [  --enable-fixed-point    compile as fixed-point],
[if test "$enableval" = yes; then
  ac_enable_fixed="yes";
  AC_DEFINE([FIXED_POINT], [1], [Compile as fixed-point])
else
  AC_DEFINE([FLOATING_POINT], , [Compile as floating-point])
fi],
AC_DEFINE([FLOATING_POINT], , [Compile as floating-point]))

ac_enable_fixed_debug="no"
AC_ARG_ENABLE(fixed-point-debug, [  --enable-fixed-point-debug  debug fixed-point implementation],
[if test "$enableval" = yes; then
  ac_enable_fixed_debug="yes"
  AC_DEFINE([FIXED_DEBUG], , [Debug fixed-point implementation])
fi])

ac_enable_experimental_postfilter="no"
AC_ARG_ENABLE(experimental-postfilter, [  --enable-experimental-postfilter     Enable this for testing only if you know what you're doing ],
[if test "$enableval" = yes; then
  ac_enable_experimental_postfilter="yes"
  AC_DEFINE([ENABLE_POSTFILTER], , [Postfilter])
fi])

ac_enable_custom_modes="no"
AC_ARG_ENABLE(custom-modes, [  --enable-custom-modes        Enable non-Opus modes, like 44.1 kHz and powers of two ],
[if test "$enableval" = yes; then
  ac_enable_custom_modes="yes"
  AC_DEFINE([CUSTOM_MODES], , [Custom modes])
fi])

float_approx=$has_float_approx
AC_ARG_ENABLE(float-approx, [  --enable-float-approx   enable fast approximations for floating point],
    [ if test "$enableval" = yes; then
    AC_WARN([Floating point approximations are not supported on all platforms.])
    float_approx=yes
    else
    float_approx=no
    fi], [ float_approx=$has_float_approx ])

if test "x${float_approx}" = "xyes"; then
    AC_DEFINE([FLOAT_APPROX], , [Float approximations])
fi

ac_enable_assertions="no"
AC_ARG_ENABLE(assertions, [  --enable-assertions     enable additional software error checking],
[if test "$enableval" = yes; then
  ac_enable_assertions="yes"
  AC_DEFINE([ENABLE_ASSERTIONS], , [Assertions])
fi])

if test "$OPUS_BUILD" != "true" ; then
saved_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -fvisibility=hidden"
AC_MSG_CHECKING([if ${CXX} supports -fvisibility=hidden])
AC_COMPILE_IFELSE([char foo;],
      [ AC_MSG_RESULT([yes])
        SYMBOL_VISIBILITY="-fvisibility=hidden" ],
        AC_MSG_RESULT([no]))
CFLAGS="$saved_CFLAGS $SYMBOL_VISIBILITY"
AC_SUBST(SYMBOL_VISIBILITY)
fi

if test $ac_cv_c_compiler_gnu = yes ; then
        CFLAGS="$CFLAGS -W -Wstrict-prototypes -Wall -Wextra -Wcast-align -Wnested-externs -Wshadow -Wno-parentheses -Wno-unused-parameter -Wno-sign-compare"
fi

AC_CHECK_FUNCS([lrintf])
AC_CHECK_FUNCS([lrint])

AC_CHECK_SIZEOF(short)
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)

if test x$has_char16 = "xyes" ; then
        case 1 in
                $ac_cv_sizeof_short) SIZE16="short";;
                $ac_cv_sizeof_int) SIZE16="int";;
        esac
else
        case 2 in
                $ac_cv_sizeof_short) SIZE16="short";;
                $ac_cv_sizeof_int) SIZE16="int";;
        esac
fi

if test x$has_char16 = "xyes" ; then
        case 2 in
                $ac_cv_sizeof_int) SIZE32="int";;
                $ac_cv_sizeof_long) SIZE32="long";;
                $ac_cv_sizeof_short) SIZE32="short";;
        esac
else
        case 4 in
                $ac_cv_sizeof_int) SIZE32="int";;
                $ac_cv_sizeof_long) SIZE32="long";;
                $ac_cv_sizeof_short) SIZE32="short";;
        esac
fi

AC_SUBST(SIZE16)
AC_SUBST(SIZE32)

if test "$OPUS_BUILD" = "true" ; then
AC_DEFINE(OPUS_BUILD, [], [We're part of Opus])
fi

AM_CONDITIONAL([FIXED_POINT], [test x$ac_enable_fixed = xyes])

AC_OUTPUT([Makefile])

AC_MSG_RESULT([
------------------------------------------------------------------------
  $PACKAGE $VERSION:  Automatic configuration OK.
  
    Compiler support:

      C99 var arrays: ................ ${has_var_arrays}
      C99 lrintf: .................... ${ac_cv_func_lrintf}
      Alloca: ........................ ${has_alloca}
    
    General configuration:
    
      Fast float approximations: ..... ${float_approx}
      Fixed point support: ........... ${ac_enable_fixed}
      Fixed point debugging: ......... ${ac_enable_fixed_debug}
      Custom modes: .................. ${ac_enable_custom_modes}
      Assertion checking: ............ ${ac_enable_assertions}
------------------------------------------------------------------------
])

echo "Type \"make; make install\" to compile and install";
echo "Type \"make check\" to run the test suite";
