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

TEST_F(SystemUtilsTestFixture, ensure_directory_exists_InvalidPath_ReturnsFailure)
{
    EXPECT_EQ(ensure_directory_exists(NULL), RDK_API_FAILURE);
    EXPECT_EQ(ensure_directory_exists(""), RDK_API_FAILURE);
}

TEST_F(SystemUtilsTestFixture, ensure_directory_exists_NestedPath_CreatesAllDirectories)
{
    char root[] = "/tmp/system_utils_dir_XXXXXX";
    ASSERT_NE(mkdtemp(root), nullptr);
    char parent[128];
    char nested[128];
    ASSERT_LT(snprintf(parent, sizeof(parent), "%s/parent", root), sizeof(parent));
    ASSERT_LT(snprintf(nested, sizeof(nested), "%s/parent/child", root), sizeof(nested));

    EXPECT_EQ(ensure_directory_exists(nested), RDK_API_SUCCESS);
    EXPECT_EQ(filePresentCheck(parent), RDK_API_SUCCESS);
    EXPECT_EQ(filePresentCheck(nested), RDK_API_SUCCESS);

    EXPECT_EQ(rmdir(nested), 0);
    EXPECT_EQ(rmdir(parent), 0);
    EXPECT_EQ(rmdir(root), 0);
}

TEST_F(SystemUtilsTestFixture, ensure_directory_exists_FileComponent_ReturnsFailure)
{
    char root[] = "/tmp/system_utils_dir_XXXXXX";
    ASSERT_NE(mkdtemp(root), nullptr);
    char component[128];
    char nested[128];
    ASSERT_LT(snprintf(component, sizeof(component), "%s/component", root), sizeof(component));
    ASSERT_LT(snprintf(nested, sizeof(nested), "%s/component/child", root), sizeof(nested));
    FILE *file = fopen(component, "w");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(fclose(file), 0);

    EXPECT_EQ(ensure_directory_exists(nested), RDK_API_FAILURE);
    EXPECT_EQ(filePresentCheck(nested), RDK_API_FAILURE);

    EXPECT_EQ(unlink(component), 0);
    EXPECT_EQ(rmdir(root), 0);
}

TEST_F(SystemUtilsTestFixture, getFreeSpace_ValidAndInvalidPaths_ReportExpectedAvailability)
{
    char valid_path[] = "/tmp";
    char relative_path[] = "tmp";
    char missing_path[] = "/tmp/nonexistent_system_utils_mount";

    EXPECT_GT(getFreeSpace(valid_path), 0u);
    EXPECT_EQ(getFreeSpace(relative_path), 0u);
    EXPECT_EQ(getFreeSpace(missing_path), 0u);
    EXPECT_EQ(getFreeSpace(NULL), 0u);
}

TEST_F(SystemUtilsTestFixture, checkFileSystem_WritableDirectory_CreatesAndRemovesProbe)
{
    char root[] = "/tmp/system_utils_fs_XXXXXX";
    ASSERT_NE(mkdtemp(root), nullptr);
    char probe[128];
    ASSERT_LT(snprintf(probe, sizeof(probe), "%s/testfile", root), sizeof(probe));

    EXPECT_EQ(checkFileSystem(root), 1u);
    EXPECT_EQ(filePresentCheck(probe), RDK_API_FAILURE);
    EXPECT_EQ(checkFileSystem(NULL), 0u);

    EXPECT_EQ(rmdir(root), 0);
}

TEST_F(SystemUtilsTestFixture, findSize_ExistingAndMissingFiles_ReportExactSize)
{
    char file_path[] = "/tmp/system_utils_size_XXXXXX";
    int fd = mkstemp(file_path);
    ASSERT_NE(fd, -1);
    const char payload[] = "known-size";
    ASSERT_EQ(write(fd, payload, sizeof(payload) - 1), sizeof(payload) - 1);
    ASSERT_EQ(close(fd), 0);

    EXPECT_EQ(findSize(file_path), static_cast<int>(sizeof(payload) - 1));
    EXPECT_EQ(unlink(file_path), 0);
    EXPECT_EQ(findSize(file_path), 0);
    EXPECT_EQ(findSize(NULL), 0);
}

TEST_F(SystemUtilsTestFixture, isDataInList_PresentMissingAndInvalidInputs_ReportMembership)
{
    char first[] = "alpha";
    char second[] = "beta";
    char *values[] = {first, second};
    char present[] = "beta";
    char missing[] = "gamma";

    EXPECT_EQ(isDataInList(values, present, 2), 1);
    EXPECT_EQ(isDataInList(values, missing, 2), 0);
    EXPECT_EQ(isDataInList(NULL, present, 2), 0);
    EXPECT_EQ(isDataInList(values, NULL, 2), 0);
}

TEST_F(SystemUtilsTestFixture, qsStringAndStrRmDuplicate_UnsortedValues_ProduceDescendingUniqueList)
{
    char alpha[] = "alpha";
    char beta_one[] = "beta";
    char beta_two[] = "beta";
    char gamma[] = "gamma";
    char *values[] = {beta_one, alpha, gamma, beta_two};

    qsString(values, 4);
    ASSERT_STREQ(values[0], "gamma");
    ASSERT_STREQ(values[1], "beta");
    ASSERT_STREQ(values[2], "beta");
    ASSERT_STREQ(values[3], "alpha");

    int unique_count = strRmDuplicate(values, 4);
    ASSERT_EQ(unique_count, 3);
    EXPECT_STREQ(values[0], "gamma");
    EXPECT_STREQ(values[1], "beta");
    EXPECT_STREQ(values[2], "alpha");
}

TEST_F(SystemUtilsTestFixture, strSplit_MoreTokensThanCapacity_StopsAtCapacity)
{
    char input[] = "one two three";
    char delimiters[] = " ";
    char *tokens[2] = {NULL, NULL};

    ASSERT_EQ(strSplit(input, delimiters, tokens, 2), 2);
    EXPECT_STREQ(tokens[0], "one");
    EXPECT_STREQ(tokens[1], "two");
}

TEST_F(SystemUtilsTestFixture, copyFiles_MultiBufferPayload_PreservesAllBytes)
{
    char source[] = "/tmp/system_utils_source_XXXXXX";
    char destination[] = "/tmp/system_utils_destination_XXXXXX";
    int source_fd = mkstemp(source);
    ASSERT_NE(source_fd, -1);
    int destination_fd = mkstemp(destination);
    ASSERT_NE(destination_fd, -1);
    ASSERT_EQ(close(destination_fd), 0);
    char payload[5000];
    for (size_t index = 0; index < sizeof(payload); ++index) {
        payload[index] = static_cast<char>(index % 251);
    }
    ASSERT_EQ(write(source_fd, payload, sizeof(payload)), static_cast<ssize_t>(sizeof(payload)));
    ASSERT_EQ(close(source_fd), 0);

    ASSERT_EQ(copyFiles(source, destination), RDK_API_SUCCESS);
    ASSERT_EQ(findSize(destination), static_cast<int>(sizeof(payload)));
    FILE *copied_file = fopen(destination, "rb");
    ASSERT_NE(copied_file, nullptr);
    char copied[sizeof(payload)];
    ASSERT_EQ(fread(copied, 1, sizeof(copied), copied_file), sizeof(copied));
    ASSERT_EQ(fclose(copied_file), 0);
    EXPECT_EQ(memcmp(payload, copied, sizeof(payload)), 0);

    EXPECT_EQ(unlink(source), 0);
    EXPECT_EQ(unlink(destination), 0);
}

TEST_F(SystemUtilsTestFixture, copyFiles_InvalidOrMissingInput_ReturnsFailure)
{
    char missing[] = "/tmp/nonexistent_system_utils_source";
    char destination[] = "/tmp/system_utils_destination_XXXXXX";
    int destination_fd = mkstemp(destination);
    ASSERT_NE(destination_fd, -1);
    ASSERT_EQ(close(destination_fd), 0);

    EXPECT_EQ(copyFiles(NULL, destination), RDK_API_FAILURE);
    EXPECT_EQ(copyFiles(missing, destination), RDK_API_FAILURE);
    EXPECT_EQ(copyFiles(destination, NULL), RDK_API_FAILURE);

    EXPECT_EQ(unlink(destination), 0);
}

TEST_F(SystemUtilsTestFixture, FileAndFolderChecks_RemoveFileReflectFilesystemState)
{
    char root[] = "/tmp/system_utils_lifecycle_XXXXXX";
    ASSERT_NE(mkdtemp(root), nullptr);
    char file_path[128];
    ASSERT_LT(snprintf(file_path, sizeof(file_path), "%s/file", root), sizeof(file_path));
    FILE *file = fopen(file_path, "w");
    ASSERT_NE(file, nullptr);
    ASSERT_EQ(fclose(file), 0);

    EXPECT_EQ(fileCheck(file_path), 1);
    EXPECT_EQ(folderCheck(file_path), 0);
    EXPECT_EQ(folderCheck(root), 1);
    EXPECT_EQ(folderCheck(NULL), 0);
    EXPECT_EQ(removeFile(file_path), RDK_API_SUCCESS);
    EXPECT_EQ(fileCheck(file_path), 0);
    EXPECT_EQ(removeFile(file_path), RDK_API_FAILURE);
    EXPECT_EQ(removeFile(NULL), RDK_API_FAILURE);

    EXPECT_EQ(rmdir(root), 0);
}

/* 6.eraseFolderExceParmFile */
TEST_F(SystemUtilsTestFixture, eraseFolderExceParamFile_folder_NULL)   
{
    char filename[30] = "file.txt";
    char pdri_filename[30] = "pdri_file.txt";
    char model[30] = "new";
    EXPECT_EQ(eraseFolderExceParamFile(NULL, filename,pdri_filename,model), -1);
}
TEST_F(SystemUtilsTestFixture, eraseFolderExceParamFile_filename_NULL)   
{
    char folder[30] = "/tmp";
    char model[30] = "abcd";
    EXPECT_EQ(eraseFolderExceParamFile(folder, NULL,NULL,model), -1);
}
TEST_F(SystemUtilsTestFixture, eraseFolderExceParamFile_model_num_NULL)   
{
    char folder[30] = "/tmp";
    char filename[30] = "file.txt";
    char pdri_filename[30] = "pdri_file.txt";
    EXPECT_EQ(eraseFolderExceParamFile(folder, filename,pdri_filename, NULL), -1);
}
TEST_F(SystemUtilsTestFixture, eraseFolderExceParamFile_valid_inputs)   
{
    int ret;
    char folder[30] = "/tmp";
    char filename[30] = "file.txt";
    char pdri_filename[30] = "pdri_filename.txt";
    char model[30] = "abcd";
    ret = system("#Before Deleteing:");
    ret = system("touch /tmp/file.txt");
    ret = system("touch /tmp/abcd_1");
    ret = system("touch /tmp/abcd_2");
    ret = system("ls -l /tmp/");
    EXPECT_EQ(eraseFolderExceParamFile(folder, filename,pdri_filename,model), 0);
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
