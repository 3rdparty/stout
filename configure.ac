# Generated with autoscan, then modified appropriately.
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([stout], [0.1.1])

AC_LANG([C++])

# Initialize automake.
AM_INIT_AUTOMAKE([1.10])

AC_CHECK_LIB([z], [deflate, gzread, gzwrite, inflate], [],
               [AC_MSG_ERROR([cannot find libz])])
               
AC_CHECK_LIB([curl], [curl_easy_init], [],
               [AC_MSG_ERROR([cannot find libcurl])])

AC_CONFIG_FILES([Makefile include/Makefile stout.pc])

AC_OUTPUT
