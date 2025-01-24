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

cd unit-test/

automake --add-missing
autoreconf --install

./configure

make clean
make

./system_utils_gtest
systemutils=$?
echo "*********** Return value of system_utils_gtest $systemutils"

./rdk_fwdl_utils_gtest
utils=$?
echo "*********** Return value of rdk_fwdl_utils_gtest $utils"

./common_device_api_gtest
deviceapi=$?
echo "*********** Return value of common_device_api_gtest $deviceapi"

./urlHelper_gtest
urlhelper=$?
echo "*********** Return value of urlHelper_gtest $urlhelper"

./json_parse_gtest
jsonparse=$?
echo "*********** Return value of json_parse_gtest $jsonparse"

./downloadUtil_gtest
dwnlutils=$?
echo "*********** Return value of downloadUtil_gtest $dwnlutils"

if [ "$systemutils" = "0" ] && [ "$utils" = "0" ] && [ "$deviceapi" = "0" ] && [ "$urlhelper" = "0" ] && [ "$jsonparse" = "0" ] && [ "$dwnlutils" = "0" ]; then
    cd ../

    lcov --capture --directory . --output-file coverage.info

    lcov --remove coverage.info '/usr/*' --output-file coverage.filtered.info

    genhtml coverage.filtered.info --output-directory out
else
    echo "L1 UNIT TEST FAILED. PLEASE CHECK AND FIX"
    exit 1
fi

