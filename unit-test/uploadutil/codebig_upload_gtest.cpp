/*
 * Copyright 2025 RDK Management
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

/**
 * @file codebig_upload_gtest.cpp
 * @brief Google Test implementation for codebig_upload.c
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "codebig_upload.h"
#include "uploadUtil.h"
#include <string.h>
#include <stdio.h>

// Function declarations for testing
extern int doCodeBigSigningForUpload(int server_type, const char* src_file, 
                                     char *signurl, size_t signurlsize, 
                                     char *outhheader, size_t outHeaderSize);
extern int doCodeBigSigning(int server_type, const char* SignInput, 
                           char *signurl, size_t signurlsize, 
                           char *outhheader, size_t outHeaderSize);
extern int uploadWithCodeBig(const char* src_file, int server_type);

// Mock functions for dependencies
extern bool __uploadutil_get_ocsp(void);
extern void __uploadutil_set_status(long http_code, int curl_code);
}

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::InSequence;
using ::testing::StrictMock;

// Mock class for CodeBig operations
class MockCodeBigOperations {
public:
    MOCK_METHOD(int, doCodeBigSigning, (int server_type, const char* SignInput, 
                                       char *signurl, size_t signurlsize, 
                                       char *outhheader, size_t outHeaderSize));
    MOCK_METHOD(bool, __uploadutil_get_ocsp, ());
    MOCK_METHOD(void, __uploadutil_set_status, (long http_code, int curl_code));
};

static MockCodeBigOperations* g_mock_codebig = nullptr;

// Mock implementations
extern "C" {
    int doCodeBigSigning(int server_type, const char* SignInput, 
                        char *signurl, size_t signurlsize, 
                        char *outhheader, size_t outHeaderSize) {
        return g_mock_codebig ? g_mock_codebig->doCodeBigSigning(server_type, SignInput, signurl, signurlsize, outhheader, outHeaderSize) : -1;
    }
    
    bool __uploadutil_get_ocsp(void) {
        return g_mock_codebig ? g_mock_codebig->__uploadutil_get_ocsp() : false;
    }
    
    void __uploadutil_set_status(long http_code, int curl_code) {
        if (g_mock_codebig) g_mock_codebig->__uploadutil_set_status(http_code, curl_code);
    }
}

class CodeBigUploadTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock_codebig = &mock_codebig;
        
        // Setup test data
        test_src_file = "/tmp/test.log";
        test_server_type_ssr = HTTP_SSR_CODEBIG;
        test_server_type_xconf = HTTP_XCONF_CODEBIG;
        test_invalid_server_type = HTTP_UNKNOWN;
        
        memset(signurl_buffer, 0, sizeof(signurl_buffer));
        memset(header_buffer, 0, sizeof(header_buffer));
    }
    
    void TearDown() override {
        g_mock_codebig = nullptr;
    }
    
    StrictMock<MockCodeBigOperations> mock_codebig;
    
    const char* test_src_file;
    int test_server_type_ssr;
    int test_server_type_xconf;
    int test_invalid_server_type;
    char signurl_buffer[MAX_CODEBIG_URL];
    char header_buffer[MAX_HEADER_LEN];
};

// ==================== doCodeBigSigningForUpload TESTS ====================

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_Success_SSRCodeBig) {
    const char* expected_url = "https://codebig.example.com/upload?signature=abc123";
    const char* expected_header = "Authorization: Bearer token123";
    
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(DoAll(
            [expected_url, expected_header](int server_type, const char* SignInput,
                                          char *signurl, size_t signurlsize,
                                          char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, expected_url, signurlsize - 1);
                strncpy(outhheader, expected_header, outHeaderSize - 1);
                return 0;
            }));
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(0, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_Success_XconfCodeBig) {
    const char* expected_url = "https://xconf-codebig.example.com/upload";
    const char* expected_header = "X-Auth: secret456";
    
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_xconf, StrEq(test_src_file), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(DoAll(
            [expected_url, expected_header](int server_type, const char* SignInput,
                                          char *signurl, size_t signurlsize,
                                          char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, expected_url, signurlsize - 1);
                strncpy(outhheader, expected_header, outHeaderSize - 1);
                return 0;
            }));
    
    int result = doCodeBigSigningForUpload(test_server_type_xconf, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(0, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidParameters) {
    // Test null src_file
    EXPECT_EQ(-1, doCodeBigSigningForUpload(test_server_type_ssr, nullptr, 
                                           signurl_buffer, sizeof(signurl_buffer),
                                           header_buffer, sizeof(header_buffer)));
    
    // Test null signurl
    EXPECT_EQ(-1, doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                           nullptr, sizeof(signurl_buffer),
                                           header_buffer, sizeof(header_buffer)));
    
    // Test null outhheader
    EXPECT_EQ(-1, doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                           signurl_buffer, sizeof(signurl_buffer),
                                           nullptr, sizeof(header_buffer)));
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidServerType) {
    // Test with HTTP_UNKNOWN server type
    int result = doCodeBigSigningForUpload(test_invalid_server_type, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_DirectServerTypes_Invalid) {
    // Test with HTTP_SSR_DIRECT (should be invalid for CodeBig)
    int result = doCodeBigSigningForUpload(HTTP_SSR_DIRECT, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(-1, result);
    
    // Test with HTTP_XCONF_DIRECT (should be invalid for CodeBig)
    result = doCodeBigSigningForUpload(HTTP_XCONF_DIRECT, test_src_file, 
                                      signurl_buffer, sizeof(signurl_buffer),
                                      header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_SigningFailure) {
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(Return(-1));  // Simulate signing failure
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(-1, result);
}

// ==================== UPLOAD WORKFLOW TESTS ====================

TEST_F(CodeBigUploadTest, CodeBigSigningWorkflow_BufferSizes) {
    const char* long_url = "https://very-long-codebig-url.example.com/upload/with/very/long/path/and/many/query/parameters?signature=verylongsignaturevaluethatmightexceedbuffers&timestamp=123456789&additional=data";
    const char* long_header = "Authorization: Bearer verylongbearertokenvaluethatmightexceedbufferverylongbearertokenvaluethatmightexceedbufferverylongbearertokenvaluethatmightexceedbufferverylongbearertokenvaluethatmightexceedbuffer";
    
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(DoAll(
            [long_url, long_header](int server_type, const char* SignInput,
                                   char *signurl, size_t signurlsize,
                                   char *outhheader, size_t outHeaderSize) {
                // Test that buffers can handle long values
                strncpy(signurl, long_url, signurlsize - 1);
                signurl[signurlsize - 1] = '\0';
                strncpy(outhheader, long_header, outHeaderSize - 1);
                outhheader[outHeaderSize - 1] = '\0';
                return 0;
            }));
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(0, result);
    // Verify strings are properly terminated
    EXPECT_EQ('\0', signurl_buffer[sizeof(signurl_buffer) - 1]);
    EXPECT_EQ('\0', header_buffer[sizeof(header_buffer) - 1]);
}

TEST_F(CodeBigUploadTest, CodeBigSigningWorkflow_EmptyResponses) {
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(DoAll(
            [](int server_type, const char* SignInput,
               char *signurl, size_t signurlsize,
               char *outhheader, size_t outHeaderSize) {
                // Return empty strings
                signurl[0] = '\0';
                outhheader[0] = '\0';
                return 0;
            }));
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(0, result);
    EXPECT_STREQ("", signurl_buffer);
    EXPECT_STREQ("", header_buffer);
}

// ==================== SERVER TYPE VALIDATION TESTS ====================

TEST_F(CodeBigUploadTest, ValidateServerTypes_CodeBigConstants) {
    // Verify CodeBig server type constants are properly defined
    EXPECT_EQ(1, HTTP_SSR_CODEBIG);
    EXPECT_EQ(3, HTTP_XCONF_CODEBIG);
    EXPECT_EQ(0, HTTP_SSR_DIRECT);
    EXPECT_EQ(2, HTTP_XCONF_DIRECT);
    EXPECT_EQ(5, HTTP_UNKNOWN);
}

TEST_F(CodeBigUploadTest, ValidateBufferSizes_Constants) {
    // Verify buffer size constants are appropriate
    EXPECT_EQ(512, MAX_HEADER_LEN);
    EXPECT_EQ(1024, MAX_CODEBIG_URL);
    
    // Verify our test buffers match the constants
    EXPECT_EQ(MAX_CODEBIG_URL, sizeof(signurl_buffer));
    EXPECT_EQ(MAX_HEADER_LEN, sizeof(header_buffer));
}

// ==================== ERROR HANDLING TESTS ====================

TEST_F(CodeBigUploadTest, ErrorHandling_ZeroBufferSizes) {
    // Test with zero buffer sizes
    char small_buffer[1];
    
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                              _, 1, _, 1))
        .WillOnce(DoAll(
            [](int server_type, const char* SignInput,
               char *signurl, size_t signurlsize,
               char *outhheader, size_t outHeaderSize) {
                // Even with size 1, should null-terminate
                if (signurlsize > 0) signurl[0] = '\0';
                if (outHeaderSize > 0) outhheader[0] = '\0';
                return 0;
            }));
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                          small_buffer, 1,
                                          small_buffer, 1);
    
    EXPECT_EQ(0, result);
}

TEST_F(CodeBigUploadTest, ErrorHandling_SpecialCharactersInFilename) {
    const char* special_filename = "/tmp/test file with spaces & symbols!@#$.log";
    
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(special_filename), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(Return(0));
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, special_filename, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(0, result);
}

// ==================== INTEGRATION TESTS ====================

TEST_F(CodeBigUploadTest, Integration_FullCodeBigWorkflow) {
    // This test simulates a complete CodeBig upload workflow
    const char* codebig_url = "https://codebig-upload.example.com/api/v1/upload?token=abc123";
    const char* auth_header = "Authorization: CodeBig signature=def456";
    
    InSequence workflow;
    
    // 1. CodeBig signing
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                              _, MAX_CODEBIG_URL, _, MAX_HEADER_LEN))
        .WillOnce(DoAll(
            [codebig_url, auth_header](int server_type, const char* SignInput,
                                      char *signurl, size_t signurlsize,
                                      char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, codebig_url, signurlsize - 1);
                strncpy(outhheader, auth_header, outHeaderSize - 1);
                return 0;
            }));
    
    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                          signurl_buffer, sizeof(signurl_buffer),
                                          header_buffer, sizeof(header_buffer));
    
    EXPECT_EQ(0, result);
    EXPECT_STREQ(codebig_url, signurl_buffer);
    EXPECT_STREQ(auth_header, header_buffer);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}