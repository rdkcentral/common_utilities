/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <unistd.h>

extern "C" {
#include "rdk_fwdl_utils.h"

BUILDTYPE getbuild( char *pBldStr );
}

#define GTEST_DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define GTEST_DEFAULT_RESULT_FILENAME "CommonUtils_RdkFwdlUtils_gtest_report.json"
#define GTEST_REPORT_FILEPATH_SIZE 256

using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::StrEq;

/*TO DO - Write Class
CLASS name : RdkFwDwnldUtilsTestFixture */

class RdkFwDwnldUtilsTestFixture : public ::testing::Test {
        protected:
                    // Member variables and functions here
        virtual void SetUp()
        {
            printf("%s\n", __func__);
        }

        virtual void TearDown()
        {
            printf("%s\n", __func__);
        }

        static void SetUpTestCase()
        {
            printf("%s\n", __func__);
        }

        static void TearDownTestCase()
        {
            printf("%s\n", __func__);
        }
    };

/* 1. getbuild*/
TEST_F(RdkFwDwnldUtilsTestFixture, filename_valid)   /*REVISIT FOR INVALID FILENAME*/
{
    int ret;
    char file[10] = "vbn";
    ret = getbuild(file);
    printf("Build type = %d\n", ret);
    EXPECT_GE(ret, 0);
    EXPECT_LE(ret, 4);
}

/* 2. getDeviceProperties */
TEST_F(RdkFwDwnldUtilsTestFixture, getDeviceProperties_Input_NULL)   
{
    EXPECT_EQ(getDeviceProperties(NULL), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, getDeviceProperties_Input_Not_NULL)   
{
    int ret;
    DeviceProperty_t pDevice_info;
    ret = system("echo \"MODEL_NUM=12062024\" > /tmp/device.properties");
    ret = system("echo \"MODEL_NUM=12062024\" > /tmp/include.properties");
    EXPECT_EQ(getDeviceProperties(&pDevice_info), 1);
    ret = system("rm -f /tmp/device.properties");
    ret = system("rm -f /tmp/include.properties");
}

/* 3. getDevicePropertyData */
TEST_F(RdkFwDwnldUtilsTestFixture, getDevicePropertyData_Property_Name_NULL)   
{
    char dev_type[16];
    EXPECT_EQ(getDevicePropertyData(NULL, dev_type, 6), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, getDevicePropertyData_Property_NULL)   
{
    char dev_prop_name[20] = "DEVICE_TYPE";
    EXPECT_EQ(getDevicePropertyData(dev_prop_name, NULL, 6), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, getDevicePropertyData_property_size_ZERO)   
{
    char dev_prop_name[20] = "DEVICE_TYPE";
    char dev_type[16];
    EXPECT_EQ(getDevicePropertyData(dev_prop_name, dev_type, 0), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, getDevicePropertyData_property_size_LARGER)   
{
    char dev_prop_name[20] = "DEVICE_TYPE";
    char dev_type[16];
    EXPECT_EQ(getDevicePropertyData(dev_prop_name, dev_type, 82), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, getDevicePropertyData_Valid_Inputs)   
{
    int ret;
    char dev_prop_name[20] = "MODEL_NUM";
    char dev_type[16];
    ret = system("echo \"MODEL_NUM=12062024\" > /tmp/device.properties");
    EXPECT_EQ(getDevicePropertyData(dev_prop_name, dev_type, 6), 1);
    ret = system("rm -f /tmp/device.properties");
}

/* 4. getIncludePropertyData */
TEST_F(RdkFwDwnldUtilsTestFixture, Property_Name_NULL)   
{
    char dev_type[16];
    EXPECT_EQ(getIncludePropertyData(NULL, dev_type, 6), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, Property_NULL)   
{
    char dev_prop_name[20] = "DEVICE_TYPE";
    EXPECT_EQ(getIncludePropertyData(dev_prop_name, NULL, 6), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, property_size_ZERO)   
{
    char dev_prop_name[20] = "DEVICE_TYPE";
    char dev_type[16];
    EXPECT_EQ(getIncludePropertyData(dev_prop_name, dev_type, 0), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, property_size_LARGER)   
{
    char dev_prop_name[20] = "DEVICE_TYPE";
    char dev_type[16];
    EXPECT_EQ(getIncludePropertyData(dev_prop_name, dev_type, 82), -1);
}
TEST_F(RdkFwDwnldUtilsTestFixture, Valid_Inputs)   
{
    int ret;
    char dev_prop_name[20] = "UTILITY_PATH";
    char utility_path[16];
    ret = system("echo \"UTILITY_PATH=/lib/rdk\" > /tmp/include.properties");
    EXPECT_EQ(getIncludePropertyData(dev_prop_name, utility_path, 20), 1);
    ret = system("rm -f /tmp/include.properties");
}

/* 5. isMediaClientDevice */
TEST_F(RdkFwDwnldUtilsTestFixture, isMediaClientDevice_Valid_Inputs)
{
    int ret;
    char dev_prop_name[20] = "DEVICE_TYPE";
    char dev_type[16];
    ret = system("echo \"DEVICE_TYPE=mediaclient\" > /tmp/device.properties");
    EXPECT_EQ(isMediaClientDevice(), true);
    ret = system("rm -f /tmp/device.properties");
}

/* 6. getImageDetails */
TEST_F(RdkFwDwnldUtilsTestFixture, Input_Not_NULL)   
{
    int ret;
    ImageDetails_t cur_img_detail;
    ret = system("echo \"imagename:Image_fortesting\" > /tmp/version.txt");
    EXPECT_EQ(getImageDetails(&cur_img_detail), 1);
    ret = system("rm -f /tmp/version.txt");
    printf("Current image name: %s\n", cur_img_detail.cur_img_name);
}
TEST_F(RdkFwDwnldUtilsTestFixture, Input_NULL)   
{
    EXPECT_EQ(getImageDetails(NULL), -1);
}

GTEST_API_ int main(int argc, char *argv[]){
    char testresults_fullfilepath[GTEST_REPORT_FILEPATH_SIZE];
    char buffer[GTEST_REPORT_FILEPATH_SIZE];

    memset( testresults_fullfilepath, 0, GTEST_REPORT_FILEPATH_SIZE );
    memset( buffer, 0, GTEST_REPORT_FILEPATH_SIZE );

    snprintf( testresults_fullfilepath, GTEST_REPORT_FILEPATH_SIZE, "json:%s%s" , GTEST_DEFAULT_RESULT_FILEPATH , GTEST_DEFAULT_RESULT_FILENAME);
    ::testing::GTEST_FLAG(output) = testresults_fullfilepath;
            ::testing::InitGoogleTest(&argc, argv);
                //testing::Mock::AllowLeak(mock);
                return RUN_ALL_TESTS();
        } 
