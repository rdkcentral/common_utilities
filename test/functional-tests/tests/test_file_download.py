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
import pytest
import subprocess

def test_dwnl_file_test():
    result = subprocess.run(['./dwnl_lib_test', '2'], stdout=subprocess.PIPE)
    assert result.returncode == 0

def test_dwnl_file_notpresent_test():
    result = subprocess.run(['./dwnl_lib_test', '3'], stdout=subprocess.PIPE)
    assert result.returncode == 0

def test_dwnl_throttle_test():
    result = subprocess.run(['./dwnl_lib_test', '4'], stdout=subprocess.PIPE)
    assert result.returncode == 0

def test_dwnl_chunk_file_test():
    result = subprocess.run(['./dwnl_lib_test', '6'], stdout=subprocess.PIPE)
    assert result.returncode == 0
