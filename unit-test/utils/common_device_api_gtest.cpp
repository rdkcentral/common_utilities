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
#include "common_device_api.h"
extern size_t GetEstbMac(char *pEstbMac, size_t szBufSize );
}

#define GTEST_DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define GTEST_DEFAULT_RESULT_FILENAME "CommonUtils_DeviceApi_gtest_report.json"
#define GTEST_REPORT_FILEPATH_SIZE 256

using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::StrEq;

/*TO DO - Write Class
CLASS name : CommonDeviceApiTestFixture */

class CommonDeviceApiTestFixture : public ::testing::Test {
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

/*1. stripinvalidchar*/
TEST_F(CommonDeviceApiTestFixture, TestName_stripinvalidchar_Null)
{
    EXPECT_EQ(stripinvalidchar(NULL, 0), 0);
}
TEST_F(CommonDeviceApiTestFixture, TestName_stripinvalidchar_Size_0)
{
    EXPECT_EQ(stripinvalidchar(NULL, 0), 0);
}
TEST_F(CommonDeviceApiTestFixture, TestName_stripinvalidchar_notNull)
{
    char data[32];
    snprintf(data, sizeof(data), "%s", "Name@#123456");
    EXPECT_NE(stripinvalidchar(data, sizeof(data)), 0);
}

/* 2. GetEstbMac*/
TEST_F(CommonDeviceApiTestFixture, TestName_GetEstbMac_NULL_chracter)
{
    EXPECT_EQ(GetEstbMac(NULL, 2),0);
}
TEST_F(CommonDeviceApiTestFixture, TestName_GetEstbMac_size_0)
{
    char data[32];
    EXPECT_EQ(GetEstbMac(data, 0),0);
}
TEST_F(CommonDeviceApiTestFixture, TestName_GetEstbMac_valid_inputs)
{
    int ret;
    char data[32];
    ret = system("echo \"F0:46:3B:9B:9A:5D\" > /tmp/estbmacfile");
    EXPECT_NE(GetEstbMac(data, 7),0);
    printf("MAC = %s\n", data);
    ret = system("rm -f /tmp/estbmacfile");
}

/* 3. GetPartnerId*/
TEST_F(CommonDeviceApiTestFixture, TestName_GetPartnerId_NULL_chracter)
{
    EXPECT_EQ(GetPartnerId(NULL, 2),0);
}
TEST_F(CommonDeviceApiTestFixture, TestName_GetPartnerId_size_0)
{
    char data[32];
    EXPECT_EQ(GetPartnerId(data, 0),0);
}
TEST_F(CommonDeviceApiTestFixture, TestName_GetPartnerId_partner_id_file)
{
    int ret;
    char data[32];
    ret = system("echo \"123456\" > /tmp/partnerId3.dat");
    EXPECT_NE(GetPartnerId(data, 7),0);
    printf("Pertner ID = %s\n", data);
    ret = system("rm -f /tmp/partnerId3.dat");
}
TEST_F(CommonDeviceApiTestFixture, TestName_GetPartnerId_bootstrap_file)
{
    int ret;
    char data[32];
    ret = system("echo \"X_RDKCENTRAL-COM_Syndication.PartnerId=789\" > /tmp/bootstrap.ini");
    EXPECT_NE(GetPartnerId(data, 7),0);
    printf("Pertner ID = %s\n", data);
    ret = system("rm -f /tmp/bootstrap.ini");
}
TEST_F(CommonDeviceApiTestFixture, TestName_GetPartnerId_no_source_file)
{
    int ret;
    char data[32];
    EXPECT_NE(GetPartnerId(data, 7),0);
    printf("Pertner ID = %s\n", data);
}

/* 4. GetModelNum*/
TEST_F(CommonDeviceApiTestFixture, GetModelNum_NULL_chracter)
{
    EXPECT_EQ(GetModelNum(NULL, 2),0);
}
TEST_F(CommonDeviceApiTestFixture, GetModelNum_size_0)
{
    char data[32];
    EXPECT_EQ(GetModelNum(data, 0),0);
}
TEST_F(CommonDeviceApiTestFixture, GetModelNum_file_not_found)
{
    int ret;
    char data[32];
    EXPECT_EQ(GetModelNum(data, 7),0);
}
TEST_F(CommonDeviceApiTestFixture, GetModelNum_file_found)
{
    int ret;
    char data[32];
    ret = system("echo \"03182025\" > /tmp/.model_number");
    EXPECT_NE(GetModelNum(data, 7),0);
    ret = system("rm -f /tmp/.model_number");
}

/* 5. GetMFRName*/
TEST_F(CommonDeviceApiTestFixture, GetMFRName_NULL_chracter)
{
    EXPECT_EQ(GetMFRName(NULL, 2),0);
}
TEST_F(CommonDeviceApiTestFixture, GetMFRName_size_0)
{
    char data[32];
    EXPECT_EQ(GetMFRName(data, 0),0);
}
TEST_F(CommonDeviceApiTestFixture, GetMFRName_file_not_found)
{
    int ret;
    char data[32];
    EXPECT_EQ(GetMFRName(data, 7),0);
}
TEST_F(CommonDeviceApiTestFixture, GetMFRName_file_found)
{
    int ret;
    char data[32];
    ret = system("echo \"03182025\" > /tmp/.manufacturer");
    EXPECT_NE(GetMFRName(data, 7),0);
    ret = system("rm -f /tmp/.manufacturer");
}

/* 6. GetBuildType*/
TEST_F(CommonDeviceApiTestFixture, GetBuildType_NULL_chracter)
{
    BUILDTYPE eBuildType;
    EXPECT_EQ(GetBuildType(NULL, 2, &eBuildType), 0);
}
TEST_F(CommonDeviceApiTestFixture, GetBuildType_size_0)
{
    char data[32];
    BUILDTYPE eBuildType;
    EXPECT_EQ(GetBuildType(data, 0, &eBuildType),0);
}
TEST_F(CommonDeviceApiTestFixture, GetBuildType_file_not_found)
{
    int ret;
    char data[200];
    BUILDTYPE eBuildType;
    ret = system("echo \"imagename:abcdtesting\" > /tmp/version.txt");
    EXPECT_EQ(GetBuildType(data, 200, &eBuildType),0);
    ret = system("rm -f /tmp/version.txt");
}
TEST_F(CommonDeviceApiTestFixture, GetBuildType_file_found)
{
    int ret;
    char data[32];
    BUILDTYPE eBuildType;
    ret = system("echo \"BUILD_TYPE=VBN\" > /tmp/device.properties");
    EXPECT_NE(GetBuildType(data, 7, &eBuildType),0);
    ret = system("rm -f /tmp/device.properties");
}

/* 7. GetFirmwareVersion*/
TEST_F(CommonDeviceApiTestFixture, GetFirmwareVersion_NULL_chracter)         /*3rd parameter revisit*/
{
    EXPECT_EQ(GetFirmwareVersion(NULL, 2),0);
}
TEST_F(CommonDeviceApiTestFixture, GetFirmwareVersion_size_0)
{
    char data[32];
    EXPECT_EQ(GetFirmwareVersion(data, 0),0);
}
TEST_F(CommonDeviceApiTestFixture, GetFirmwareVersion_file_not_found)
{
    char data[32];
    EXPECT_EQ(GetFirmwareVersion(data, 7),0);
}
TEST_F(CommonDeviceApiTestFixture, GetFirmwareVersion_file_found)
{
    int ret;
    char data[200];
    ret = system("echo \"imagename:Image.bin\" > /tmp/version.txt");
    EXPECT_NE(GetFirmwareVersion(data, 200),0);
    printf("FW Version = %s\n", data);
    ret = system("rm -f /tmp/version.txt");
}

/* 8. CurrentRunningInst */
TEST_F(CommonDeviceApiTestFixture, CurrentRunningInst_Filename_NULL)   
{
    EXPECT_NE(CurrentRunningInst(NULL), true);
}
TEST_F(CommonDeviceApiTestFixture, CurrentRunningInst_File_not_found)  
{
    EXPECT_NE(CurrentRunningInst("/tmp/.rfcServiceLock"), true);
}
TEST_F(CommonDeviceApiTestFixture, CurrentRunningInst_File_found)  
{
    int ret;
    ret = system("echo \"filler string\" > /tmp/.rfcServiceLock");
    EXPECT_EQ(CurrentRunningInst("/tmp/.rfcServiceLock"), true);
    ret = system("rm -rf /tmp/.rfcServiceLock");
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
