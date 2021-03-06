## -*- mode: Autoconf -*-
##
## Copyright (c) 2008, 2010, 2011 The University of Utah
## All rights reserved.
##
## This file is part of `csmith', a random generator of C programs.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
##   * Redistributions of source code must retain the above copyright notice,
##     this list of conditions and the following disclaimer.
##
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in the
##     documentation and/or other materials provided with the distribution.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

###############################################################################

m4_include([version.m4])
AC_INIT(csmith, CSMITH_VERSION, CSMITH_BUGREPORT, csmith, CSMITH_URL)

AC_CONFIG_SRCDIR(src/RandomProgramGenerator.cpp)
AC_CONFIG_AUX_DIR(autoconf)
dnl AM_CONFIG_HEADER(config.h)

AC_CANONICAL_TARGET

# `-Wno-portability': avoid warnings about using `:=' in `Makefile.am' files.
AM_INIT_AUTOMAKE([-Wall -Wno-portability])

AM_MAINTAINER_MODE

# Avoid configure-time warnings about `--datarootdir' being ignored.
# This can perhaps be removed in a few years after Autoconf 2.60.
# See <http://www.gnu.org/software/libtool/manual/autoconf/Changed-Directory-Variables.html>
#
AC_DEFUN([AC_DATAROOTDIR_CHECKED])

dnl AC_PROG_CC # Automake 1.11 want us to use AM_PROG_CC_C_O instead...
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_LIBTOOL

dnl AX_BOOST_BASE()
dnl AX_BOOST_PROGRAM_OPTIONS

# If we're using g++, set the default `CXXFLAGS' to something more pedantic.
#
if test "$ac_test_CXXFLAGS" = set; then
  # The user specified `CXXFLAGS', so do nothing.
  CXXFLAGS="$CXXFLAGS"
elif test "$GXX" = yes; then 
  CXXFLAGS="-std=c++11 -g -O3 -Wall -Wextra -Wno-long-long"
fi

# Some runtime headers are generated by `m4'.
AC_CHECK_PROGS([M4], [m4 gm4], [m4])

AC_OUTPUT(
  Makefile
  doc/Makefile
  runtime/Makefile
  scripts/Makefile
  src/Makefile
)

###############################################################################

## End of file.
