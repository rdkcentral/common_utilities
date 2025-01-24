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

AM_CFLAGS = $(TRACE_CFLAGS)

lib_LTLIBRARIES = libparsejson.la
libparsejson_la_SOURCES = json_parse.c
libparsejson_la_CFLAGS = $(cjson_CFLAGS) -I${top_srcdir}/utils
libparsejson_la_LDFLAGS = $(cjson_LIBS) -lrdkloggers
libparsejson_la_includedir = ${includedir}
libparsejson_la_include_HEADERS = json_parse.h



# enable the following block to build jsonget and jsonwrite programs
# for use in testing or if needed in the future
if build_parsejson_bins
bin_PROGRAMS = jsonget jsonwrite

jsonget_DEPENDENCIES = libparsejson.la
jsonwrite_DEPENDENCIES = libparsejson.la

jsonget_SOURCES = \
    main_get.c

jsonwrite_SOURCES = \
    main_write.c

jsonget_LDADD = ${top_builddir}/parsejson/libparsejson.la ${top_builddir}/utils/libfwutils.la
jsonwrite_LDADD = ${top_builddir}/parsejson/libparsejson.la ${top_builddir}/utils/libfwutils.la
jsonget_CFLAGS = -I${top_srcdir}/utils
jsonwrite_CFLAGS = -I${top_srcdir}/utils
endif
