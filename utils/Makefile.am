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
AM_CFLAGS += -Wall -Werror $(CFLAGS)

lib_LTLIBRARIES = libfwutils.la
libfwutils_la_SOURCES = rdk_fwdl_utils.c \
                        system_utils.c \
                        rdkv_cdl_log_wrapper.c \
			common_device_api.c

libfwutils_la_LDFLAGS = -shared -fPIC -lrdkloggers

libfwutils_la_include_HEADERS = rdk_fwdl_utils.h \
			        system_utils.h \
			        rdkv_cdl_log_wrapper.h \
                                common_device_api.h				 

libfwutils_la_includedir = ${includedir}
# TODO Use right AC conditional flag for gtest GTEST_ENABLE_FLAG
