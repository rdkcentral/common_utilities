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

AUTOMAKE_OPTIONS = subdir-objects

# Define the program name and the source files
bin_PROGRAMS = uploadUtil_gtest upload_status_gtest codebig_upload_gtest mtls_upload_gtest

# Define the include directories
COMMON_CPPFLAGS = -std=c++11 -I/usr/include/cjson -I../../utils -I../../mocks -I../../uploadutil -I../../dwnlutils -I../../parsejson -DGTEST_ENABLE

# Define the libraries to link against
COMMON_LDADD = -lcjson -lgcov -lcurl -lgtest -lgtest_main -lgmock_main -lgmock

# Define the compiler flags
COMMON_CXXFLAGS = -frtti -fprofile-arcs -ftest-coverage -fpermissive

# Define the source files
uploadUtil_gtest_SOURCES = uploadUtil_gtest.cpp ../../uploadutil/uploadUtil.c ../../utils/rdkv_cdl_log_wrapper.c ../../parsejson/json_parse.c

upload_status_gtest_SOURCES = upload_status_gtest.cpp ../../uploadutil/upload_status.c ../../utils/rdkv_cdl_log_wrapper.c

codebig_upload_gtest_SOURCES = codebig_upload_gtest.cpp ../../uploadutil/codebig_upload.c ../../utils/rdkv_cdl_log_wrapper.c

mtls_upload_gtest_SOURCES = mtls_upload_gtest.cpp ../../uploadutil/mtls_upload.c ../../uploadutil/uploadUtil.c ../../utils/rdkv_cdl_log_wrapper.c ../../parsejson/json_parse.c ../../dwnlutils/downloadUtil.c ../../dwnlutils/urlHelper.c

# Apply common properties to each program
uploadUtil_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
uploadUtil_gtest_LDADD = $(COMMON_LDADD)
uploadUtil_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
uploadUtil_gtest_CFLAGS = $(COMMON_CXXFLAGS)

upload_status_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
upload_status_gtest_LDADD = $(COMMON_LDADD)
upload_status_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
upload_status_gtest_CFLAGS = $(COMMON_CXXFLAGS)

codebig_upload_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
codebig_upload_gtest_LDADD = $(COMMON_LDADD)
codebig_upload_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
codebig_upload_gtest_CFLAGS = $(COMMON_CXXFLAGS)

mtls_upload_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
mtls_upload_gtest_LDADD = $(COMMON_LDADD)
mtls_upload_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
mtls_upload_gtest_CFLAGS = $(COMMON_CXXFLAGS)