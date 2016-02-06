#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([qotd], [0.3.1])
AC_CONFIG_SRCDIR([src/qotdd.c])
AC_OUTPUT(Makefile src/Makefile man/Makefile)
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC([gcc clang cc])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([netinet/in.h stdlib.h string.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([socket strerror])

AC_OUTPUT
