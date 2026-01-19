##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2025 RDK Management
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
##########################################################################

AM_CFLAGS = -D_ANSC_LINUX
AM_CFLAGS += -D_ANSC_USER
AM_CFLAGS += -D_ANSC_LITTLE_ENDIAN_
AM_CFLAGS += -Wall -Werror $(curl_CFLAGS) $(CFLAGS)

lib_LTLIBRARIES = libuploadutil.la
libuploadutil_la_SOURCES = uploadUtil.c \
                           mtls_upload.c \
                           codebig_upload.c \
                           upload_status.c
if USE_CPC_CODE
libuploadutil_la_SOURCES += \
    ${top_srcdir}/src/upload_util-cpc/upload_util/codebigUtils.c
else
libuploadutil_la_SOURCES += codebigUtils.c    
endif

libuploadutil_la_LDFLAGS = -shared -fPIC -lrdkloggers -lpthread $(curl_LIBS) -L$(PKG_CONFIG_SYSROOT_DIR)/usr/lib64 -lrdkconfig
libuploadutil_la_LDFLAGS += ${top_builddir}/dwnlutils/libdwnlutil.la ${top_builddir}/utils/libfwutils.la  -DLIBRDKCERTSELECTOR

libuploadutil_la_include_HEADERS = uploadUtil.h \
                                   mtls_upload.h \
                                   codebig_upload.h \
                                   upload_status.h

libuploadutil_la_CFLAGS = -I${top_srcdir}/dwnlutils -I$(top_srcdir)/utils -I${top_srcdir}/parsejson
libuploadutil_la_includedir = ${includedir}

if USE_CPC_CODE
libuploadutil_la_CFLAGS += -I${top_srcdir}/src/upload_util-cpc/upload_util
endif

if IS_LIBRDKCERTSEL_ENABLED
libuploadutil_la_CFLAGS += $(LIBRDKCERTSEL_FLAG)
AM_LDFLAGS = -lRdkCertSelector
endif
