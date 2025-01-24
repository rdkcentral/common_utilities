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
bin_PROGRAMS = system_utils_gtest rdk_fwdl_utils_gtest common_device_api_gtest urlHelper_gtest json_parse_gtest downloadUtil_gtest

# Define the include directories
COMMON_CPPFLAGS = -std=c++11 -I/usr/include/cjson -I../utils -I../mocks -I../dwnlutils -I../parsejson -DGTEST_ENABLE

# Define the libraries to link against
COMMON_LDADD =  -lcjson -lgcov -lcurl -lgtest -lgtest_main -lgmock_main -lgmock

# Define the compiler flags
COMMON_CXXFLAGS = -frtti -fprofile-arcs -ftest-coverage -fpermissive

# Define the source files
common_device_api_gtest_SOURCES = utils/common_device_api_gtest.cpp ../utils/common_device_api.c ../utils/rdkv_cdl_log_wrapper.c
system_utils_gtest_SOURCES = utils/system_utils_gtest.cpp ../utils/system_utils.c ../utils/rdkv_cdl_log_wrapper.c

rdk_fwdl_utils_gtest_SOURCES = utils/rdk_fwdl_utils_gtest.cpp ../utils/rdk_fwdl_utils.c ../utils/rdkv_cdl_log_wrapper.c

urlHelper_gtest_SOURCES = dwnlutils/urlHelper_gtest.cpp ../dwnlutils/urlHelper.c ../utils/rdkv_cdl_log_wrapper.c ../dwnlutils/downloadUtil.c mocks/curl_mock.cpp

json_parse_gtest_SOURCES = parsejson/json_parse_gtest.cpp ../parsejson/json_parse.c ../utils/rdkv_cdl_log_wrapper.c 

downloadUtil_gtest_SOURCES = ../dwnlutils/downloadUtil.c mocks/curl_mock.cpp mocks/mock_urlHelper.cpp ../utils/rdkv_cdl_log_wrapper.c dwnlutils/downloadUtil_gtest.cpp

# Apply common properties to each program
common_device_api_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
common_device_api_gtest_LDADD = $(COMMON_LDADD)
common_device_api_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
common_device_api_gtest_CFLAGS = $(COMMON_CXXFLAGS)

system_utils_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
system_utils_gtest_LDADD = $(COMMON_LDADD)
system_utils_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
system_utils_gtest_CFLAGS = $(COMMON_CXXFLAGS)

rdk_fwdl_utils_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
rdk_fwdl_utils_gtest_LDADD = $(COMMON_LDADD)
rdk_fwdl_utils_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
rdk_fwdl_utils_gtest_CFLAGS = $(COMMON_CXXFLAGS)

urlHelper_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
urlHelper_gtest_LDADD = $(COMMON_LDADD)
urlHelper_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
urlHelper_gtest_CFLAGS = $(COMMON_CXXFLAGS)

json_parse_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
json_parse_gtest_LDADD = $(COMMON_LDADD)
json_parse_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
json_parse_gtest_CFLAGS = $(COMMON_CXXFLAGS)

downloadUtil_gtest_CPPFLAGS = $(COMMON_CPPFLAGS)
downloadUtil_gtest_LDADD = $(COMMON_LDADD)
downloadUtil_gtest_CXXFLAGS = $(COMMON_CXXFLAGS)
downloadUtil_gtest_CFLAGS = $(COMMON_CXXFLAGS)
