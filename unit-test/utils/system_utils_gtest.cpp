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
#include "system_utils.h"
#include "rdkv_cdl_log_wrapper.h"
}

#define GTEST_DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define GTEST_DEFAULT_RESULT_FILENAME "CommonUtils_SystemUtils_gtest_report.json"
#define GTEST_REPORT_FILEPATH_SIZE 256

using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::StrEq;

/*TO DO - Write Class
CLASS name : SystemUtilsTestFixture */
class SystemUtilsTestFixture : public ::testing::Test {
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

/* 1. filePresentCheck */
TEST_F(SystemUtilsTestFixture, filePresentCheck_Input_NULL)   
{
    EXPECT_EQ(filePresentCheck(NULL), -1);
}
TEST_F(SystemUtilsTestFixture, filePresentCheck_file_not_present)   
{
    char filename[30] = "file.txt";
    EXPECT_EQ(filePresentCheck(filename), -1);
}
TEST_F(SystemUtilsTestFixture, filePresentCheck_file_present)
{
    int ret;
    char filename[30] = "/tmp/file.txt";
    ret = system("echo \"filler string\" > /tmp/file.txt");
    EXPECT_EQ(filePresentCheck(filename), 0);
    ret = system("rm -f /tmp/file.txt");
}

/* 2. cmdExec */
TEST_F(SystemUtilsTestFixture, cmdExec_Command_NULL)   
{
    char Output[30];
    EXPECT_EQ(cmdExec(NULL, Output, sizeof(Output)), -1);
}
TEST_F(SystemUtilsTestFixture, cmdExec_Output_NULL)   
{
    char command[30] = "Execute something";
    EXPECT_EQ(cmdExec(command, NULL, sizeof(command)), -1);
}
TEST_F(SystemUtilsTestFixture, cmdExec_Size_Zero)   
{
    char Output[30];
    char command[30] = "Execute something";
    EXPECT_EQ(cmdExec(command, Output, 0), -1);
}
TEST_F(SystemUtilsTestFixture, cmdExec_Larger_Size)   
{
    char Output[30];
    char command[30] = "Execute something";
    EXPECT_EQ(cmdExec(command, Output, 6124), -1);
}
TEST_F(SystemUtilsTestFixture, cmdExec_Valid_Inputs)   
{
    char Output[30];
    char command[200] = "ls";
    EXPECT_EQ(cmdExec(command, Output, sizeof(command)), 0);
    printf("output: %s", Output);
}

/* 3.getFileSize */
TEST_F(SystemUtilsTestFixture, getFileSize_filename_NULL)   
{
    EXPECT_EQ(getFileSize(NULL), -1);
}
TEST_F(SystemUtilsTestFixture, getFileSize_file_not_present)   
{
    char filename[30] = "/tmp/file.txt";
    EXPECT_EQ(getFileSize(filename), -1);
}
TEST_F(SystemUtilsTestFixture, getFileSize_filename_not_null)
{
    int ret;
    char filename[30] = "/tmp/file.txt";
    ret = system("echo \"filler string\" > /tmp/file.txt");
    EXPECT_NE(getFileSize(filename), -1);
    ret = system("rm -f /tmp/file.txt");
}

/* 4.logFileData */
TEST_F(SystemUtilsTestFixture, logFileData_filepath_NULL)   
{
    EXPECT_EQ(logFileData(NULL), -1);
}
TEST_F(SystemUtilsTestFixture, logFileData_file_not_found)   
{
    char filepath[30] = "/tmp/file.txt";
    EXPECT_EQ(logFileData(filepath), -1);
}
TEST_F(SystemUtilsTestFixture, logFileData_file_found)
{
    int ret;
    char filepath[30] = "/tmp/file.txt";
    ret = system("echo \"filler string\" > /tmp/file.txt");
    EXPECT_EQ(logFileData(filepath), 1);
    ret = system("rm -f /tmp/file.txt");
}

/* 5.createDir */
TEST_F(SystemUtilsTestFixture, createDir_directory_NULL)   
{
    EXPECT_EQ(createDir(NULL), -1);
}
TEST_F(SystemUtilsTestFixture, createDir_directory_not_null)   
{
    int ret;
    char directory[30] = "/tmp/newdir";
    EXPECT_EQ(createDir(directory), 0);
    ret = system("rm -f /tmp/newdir");
}
TEST_F(SystemUtilsTestFixture, createDir_directory_already_exists)
{
    int ret;
    char directory[30] = "/tmp/newdir";
    ret = system("mkdir /tmp/newdir");
    EXPECT_EQ(createDir(directory), 0);
    ret = system("rm -f /tmp/newdir");
}

/* 6.eraseFolderExcePramaFile */
TEST_F(SystemUtilsTestFixture, eraseFolderExcePramaFile_folder_NULL)   
{
    char filename[30] = "file.txt";
    char model[30] = "new";
    EXPECT_EQ(eraseFolderExcePramaFile(NULL, filename,model), -1);
}
TEST_F(SystemUtilsTestFixture, eraseFolderExcePramaFile_filename_NULL)   
{
    char folder[30] = "/tmp";
    char model[30] = "abcd";
    EXPECT_EQ(eraseFolderExcePramaFile(folder, NULL,model), -1);
}
TEST_F(SystemUtilsTestFixture, eraseFolderExcePramaFile_model_num_NULL)   
{
    char folder[30] = "/tmp";
    char filename[30] = "file.txt";
    EXPECT_EQ(eraseFolderExcePramaFile(folder, filename, NULL), -1);
}
TEST_F(SystemUtilsTestFixture, eraseFolderExcePramaFile_valid_inputs)   
{
    int ret;
    char folder[30] = "/tmp";
    char filename[30] = "file.txt";
    char model[30] = "abcd";
    ret = system("#Before Deleteing:");
    ret = system("touch /tmp/file.txt");
    ret = system("touch /tmp/abcd_1");
    ret = system("touch /tmp/abcd_2");
    ret = system("ls -l /tmp/");
    EXPECT_EQ(eraseFolderExcePramaFile(folder, filename,model), 0);
    ret = system("#After Deleteing:");
    ret = system("ls -l /tmp/");
    ret = system("rm -rf /tmp/file.txt /tmp/abcd_1 /tmp/abcd_2");
}

/* 7. createFile */
TEST_F(SystemUtilsTestFixture, createFile_filename_NULL)   /*void function*/
{
    EXPECT_EQ(createFile(NULL), -1);
}
TEST_F(SystemUtilsTestFixture, createFile_filename_not_null)   
{
    int ret;
    char filename[30] = "/tmp/file.txt";
    EXPECT_EQ(createFile(filename), 0);
    ret = system("rm -rf /tmp/file.txt");
}
TEST_F(SystemUtilsTestFixture, createFile_file_read_only)
{
    int ret;
    char filename[30] = "/tmp/file.txt";
    ret = system("touch /tmp/file.txt");
    ret = system("chmod 444 /tmp/file.txt");
    EXPECT_EQ(createFile(filename), 0);
    ret = system("rm -rf /tmp/file.txt");
}

/* 8. eraseTGZItemsMatching */
TEST_F(SystemUtilsTestFixture, eraseTGZItemsMatching_folder_NULL)   /*success=0, failure=1*/
{
    char filename[30] = "file.txt";
    EXPECT_EQ(eraseTGZItemsMatching(NULL, filename), 1);
}
TEST_F(SystemUtilsTestFixture, eraseTGZItemsMatching_filename_NULL)
{
    char folder[30] = "/tmp";
    EXPECT_EQ(eraseTGZItemsMatching(folder, NULL), 1);
}
TEST_F(SystemUtilsTestFixture,  eraseTGZItemsMatching_file_not_deleted)   
{
    char folder[30] = "/tmp";
    char filename[30] = "file";
    EXPECT_EQ(eraseTGZItemsMatching(folder, filename), 1);
}
TEST_F(SystemUtilsTestFixture,  eraseTGZItemsMatching_file_deleted)
{
    int ret;
    char folder[30] = "/tmp";
    char filename[30] = "file";
    ret = system ("touch /tmp/file.tgz\n");
    EXPECT_EQ(eraseTGZItemsMatching(folder, filename), 0);
}
TEST_F(SystemUtilsTestFixture, emptyFolder_ValidDirectory) {
    system("mkdir -p /tmp/testdir && touch /tmp/testdir/file1");
    EXPECT_EQ(emptyFolder("/tmp/testdir"), RDK_API_SUCCESS);
    system("rmdir /tmp/testdir");
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
