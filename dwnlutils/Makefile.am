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

AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -Wall -Werror $(cjson_CFLAGS) $(curl_CFLAGS) $(CFLAGS)

lib_LTLIBRARIES = libdwnlutil.la
libdwnlutil_la_SOURCES = urlHelper.c \
                         downloadUtil.c \
                         curl_debug.c

libdwnlutil_la_LDFLAGS = -shared -fPIC -lrdkloggers -lpthread $(curl_LIBS)

libdwnlutil_la_include_HEADERS = downloadUtil.h \
				 urlHelper.h 

libdwnlutil_la_CPPFLAGS = -I${top_srcdir}/utils
libdwnlutil_la_includedir = ${includedir}
# TODO Use right AC conditional flag for gtest GTEST_ENABLE_FLAG
