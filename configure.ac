# Copyright 2023 Comcast Cable Communications Management, LLC
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
#

AC_PREREQ([2.65])
AC_INIT([common_utilities], [1.0], [BUG-REPORT-ADDRESS])
AM_INIT_AUTOMAKE([subdir-objects])
#AM_INIT_AUTOMAKE([foreign])
LT_INIT

AC_PREFIX_DEFAULT(`pwd`)
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_MAKE_SET

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AM_PROG_LIBTOOL(libtool)

TRACE_CFLAGS=" "
TRACE_LIBS==" "

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([sys/ioctl.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([termio.h])
AC_FUNC_MKTIME
AC_PROG_RANLIB
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T


PKG_CHECK_MODULES([cjson], [libcjson >= 1.7.12])
PKG_CHECK_MODULES([curl], [libcurl >= 7.60.0])


# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

AC_SUBST(TRACE_CFLAGS)
AC_SUBST(TRACE_LIBS)
AC_CONFIG_FILES([Makefile
    dwnlutils/Makefile
    utils/Makefile
    parsejson/Makefile
    ])

BUILD_BINS="false"
AM_CONDITIONAL([build_parsejson_bins], [test "x$BUILD_BINS" = xtrue])

AC_OUTPUT


