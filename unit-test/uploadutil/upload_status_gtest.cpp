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
 * @file upload_status_gtest.cpp
 * @brief Google Test implementation for upload_status.c
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "upload_status.h"
#include <string.h>
#include <stdio.h>

// Function declarations for testing
extern void __uploadutil_set_status(long http_code, int curl_code);
extern void __uploadutil_get_status(long *http_code, int *curl_code);
extern void __uploadutil_set_fqdn(const char *fqdn);
extern void __uploadutil_get_fqdn(char *fqdn, size_t size);
extern void __uploadutil_set_ocsp(bool enabled);
extern bool __uploadutil_get_ocsp(void);
extern void __uploadutil_set_md5(const char *md5);
extern const char* __uploadutil_get_md5(void);

// Status structure functions
extern void initUploadStatus(UploadStatusDetail *status);
extern void setUploadResult(UploadStatusDetail *status, int result_code, long http_code, int curl_code);
extern void setUploadError(UploadStatusDetail *status, const char *error_msg);
extern void setUploadAuth(UploadStatusDetail *status, bool auth_success);
extern void setUploadCompletion(UploadStatusDetail *status, bool completed);
extern void setUploadFqdn(UploadStatusDetail *status, const char *url);
extern const char* getUploadErrorString(int result_code);
}

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::StrictMock;

class UploadStatusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test status structure
        memset(&status, 0, sizeof(status));
        initUploadStatus(&status);
    }
    
    void TearDown() override {
        // Reset any global state
        __uploadutil_set_status(0, 0);
        __uploadutil_set_fqdn(nullptr);
        __uploadutil_set_ocsp(false);
        __uploadutil_set_md5(nullptr);
    }
    
    UploadStatusDetail status;
};

// ==================== THREAD-LOCAL STATUS TESTS ====================

TEST_F(UploadStatusTest, SetAndGetStatus_Basic) {
    long http_code = 200;
    int curl_code = 0;  // CURLE_OK
    
    __uploadutil_set_status(http_code, curl_code);
    
    long retrieved_http = 0;
    int retrieved_curl = 0;
    __uploadutil_get_status(&retrieved_http, &retrieved_curl);
    
    EXPECT_EQ(200, retrieved_http);
    EXPECT_EQ(0, retrieved_curl);
}

TEST_F(UploadStatusTest, GetStatus_ResetsAfterRead) {
    long http_code = 404;
    int curl_code = 7;  // CURLE_COULDNT_CONNECT
    
    __uploadutil_set_status(http_code, curl_code);
    
    // First read
    long retrieved_http = 0;
    int retrieved_curl = 0;
    __uploadutil_get_status(&retrieved_http, &retrieved_curl);
    
    EXPECT_EQ(404, retrieved_http);
    EXPECT_EQ(7, retrieved_curl);
    
    // Second read should be reset
    __uploadutil_get_status(&retrieved_http, &retrieved_curl);
    
    EXPECT_EQ(0, retrieved_http);
    EXPECT_EQ(0, retrieved_curl);
}

TEST_F(UploadStatusTest, SetAndGetFqdn_ValidString) {
    const char* test_fqdn = "example.com";
    char retrieved_fqdn[256];
    
    __uploadutil_set_fqdn(test_fqdn);
    __uploadutil_get_fqdn(retrieved_fqdn, sizeof(retrieved_fqdn));
    
    EXPECT_STREQ(test_fqdn, retrieved_fqdn);
}

TEST_F(UploadStatusTest, SetFqdn_NullInput) {
    char retrieved_fqdn[256] = "initial";
    
    __uploadutil_set_fqdn(nullptr);
    __uploadutil_get_fqdn(retrieved_fqdn, sizeof(retrieved_fqdn));
    
    EXPECT_STREQ("", retrieved_fqdn);
}

TEST_F(UploadStatusTest, GetFqdn_ResetsAfterRead) {
    const char* test_fqdn = "test.example.com";
    char retrieved_fqdn[256];
    
    __uploadutil_set_fqdn(test_fqdn);
    
    // First read
    __uploadutil_get_fqdn(retrieved_fqdn, sizeof(retrieved_fqdn));
    EXPECT_STREQ(test_fqdn, retrieved_fqdn);
    
    // Second read should be empty
    __uploadutil_get_fqdn(retrieved_fqdn, sizeof(retrieved_fqdn));
    EXPECT_STREQ("", retrieved_fqdn);
}

TEST_F(UploadStatusTest, SetAndGetOcsp_Enabled) {
    __uploadutil_set_ocsp(true);
    EXPECT_TRUE(__uploadutil_get_ocsp());
}

TEST_F(UploadStatusTest, SetAndGetOcsp_Disabled) {
    __uploadutil_set_ocsp(false);
    EXPECT_FALSE(__uploadutil_get_ocsp());
}

TEST_F(UploadStatusTest, SetAndGetMd5_ValidHash) {
    const char* test_md5 = "abcd1234efgh5678";
    
    __uploadutil_set_md5(test_md5);
    const char* retrieved_md5 = __uploadutil_get_md5();
    
    EXPECT_STREQ(test_md5, retrieved_md5);
}

TEST_F(UploadStatusTest, SetMd5_NullInput) {
    __uploadutil_set_md5(nullptr);
    const char* retrieved_md5 = __uploadutil_get_md5();
    
    EXPECT_EQ(nullptr, retrieved_md5);
}

TEST_F(UploadStatusTest, GetMd5_EmptyString) {
    __uploadutil_set_md5("");
    const char* retrieved_md5 = __uploadutil_get_md5();
    
    EXPECT_EQ(nullptr, retrieved_md5);
}

// ==================== UPLOAD STATUS STRUCTURE TESTS ====================

TEST_F(UploadStatusTest, initUploadStatus_DefaultValues) {
    // Status should already be initialized in SetUp
    EXPECT_EQ(0, status.result_code);
    EXPECT_EQ(0, status.http_code);
    EXPECT_EQ(0, status.curl_code);  // CURLE_OK
    EXPECT_FALSE(status.upload_completed);
    EXPECT_FALSE(status.auth_success);
    EXPECT_STREQ("", status.error_message);
    EXPECT_STREQ("", status.fqdn);
}

TEST_F(UploadStatusTest, setUploadResult_Success) {
    int result_code = 0;
    long http_code = 200;
    int curl_code = 0;
    
    setUploadResult(&status, result_code, http_code, curl_code);
    
    EXPECT_EQ(0, status.result_code);
    EXPECT_EQ(200, status.http_code);
    EXPECT_EQ(0, status.curl_code);
}

TEST_F(UploadStatusTest, setUploadResult_Failure) {
    int result_code = -1;
    long http_code = 500;
    int curl_code = 7;  // CURLE_COULDNT_CONNECT
    
    setUploadResult(&status, result_code, http_code, curl_code);
    
    EXPECT_EQ(-1, status.result_code);
    EXPECT_EQ(500, status.http_code);
    EXPECT_EQ(7, status.curl_code);
}

TEST_F(UploadStatusTest, setUploadError_ValidMessage) {
    const char* error_msg = "Connection timeout";
    
    setUploadError(&status, error_msg);
    
    EXPECT_STREQ(error_msg, status.error_message);
}

TEST_F(UploadStatusTest, setUploadError_NullMessage) {
    setUploadError(&status, nullptr);
    
    EXPECT_STREQ("", status.error_message);
}

TEST_F(UploadStatusTest, setUploadError_LongMessage_Truncated) {
    char long_message[300];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';
    
    setUploadError(&status, long_message);
    
    EXPECT_EQ(255, strlen(status.error_message));  // Should be truncated
    EXPECT_EQ('A', status.error_message[254]);  // Last char before null terminator
}

TEST_F(UploadStatusTest, setUploadAuth_Success) {
    setUploadAuth(&status, true);
    EXPECT_TRUE(status.auth_success);
}

TEST_F(UploadStatusTest, setUploadAuth_Failure) {
    setUploadAuth(&status, false);
    EXPECT_FALSE(status.auth_success);
}

TEST_F(UploadStatusTest, setUploadCompletion_Completed) {
    setUploadCompletion(&status, true);
    EXPECT_TRUE(status.upload_completed);
}

TEST_F(UploadStatusTest, setUploadCompletion_NotCompleted) {
    setUploadCompletion(&status, false);
    EXPECT_FALSE(status.upload_completed);
}

TEST_F(UploadStatusTest, setUploadFqdn_HttpUrl) {
    const char* url = "https://example.com/upload";
    
    setUploadFqdn(&status, url);
    
    EXPECT_STREQ("example.com", status.fqdn);
}

TEST_F(UploadStatusTest, setUploadFqdn_HttpsUrlWithPort) {
    const char* url = "https://api.example.com:8443/v1/upload";
    
    setUploadFqdn(&status, url);
    
    EXPECT_STREQ("api.example.com", status.fqdn);
}

TEST_F(UploadStatusTest, setUploadFqdn_HttpUrlWithPort) {
    const char* url = "http://localhost:8080/upload";
    
    setUploadFqdn(&status, url);
    
    EXPECT_STREQ("localhost", status.fqdn);
}

TEST_F(UploadStatusTest, setUploadFqdn_InvalidUrl) {
    const char* url = "not-a-url";
    
    setUploadFqdn(&status, url);
    
    EXPECT_STREQ("", status.fqdn);  // Should remain empty for invalid URL
}

TEST_F(UploadStatusTest, setUploadFqdn_NullUrl) {
    setUploadFqdn(&status, nullptr);
    
    EXPECT_STREQ("", status.fqdn);
}

TEST_F(UploadStatusTest, getUploadErrorString_CommonCodes) {
    EXPECT_STREQ("Success", getUploadErrorString(0));
    EXPECT_STREQ("Upload failed", getUploadErrorString(-1));
    EXPECT_STREQ("Invalid parameters", getUploadErrorString(-2));
    EXPECT_STREQ("File not found", getUploadErrorString(-3));
    EXPECT_STREQ("Network error", getUploadErrorString(-4));
    EXPECT_STREQ("Authentication failed", getUploadErrorString(-5));
    EXPECT_STREQ("Server error", getUploadErrorString(-6));
}

TEST_F(UploadStatusTest, getUploadErrorString_UnknownCode) {
    const char* result = getUploadErrorString(-999);
    EXPECT_STREQ("Unknown error", result);
}

// ==================== INTEGRATION TESTS ====================

TEST_F(UploadStatusTest, FullStatusWorkflow) {
    const char* test_url = "https://upload.example.com/api/v1/logs";
    const char* test_error = "SSL handshake failed";
    
    // Set initial state
    setUploadFqdn(&status, test_url);
    setUploadAuth(&status, true);
    
    // Simulate failure
    setUploadResult(&status, -4, 0, 35);  // CURLE_SSL_CONNECT_ERROR
    setUploadError(&status, test_error);
    setUploadCompletion(&status, false);
    
    // Verify final state
    EXPECT_EQ(-4, status.result_code);
    EXPECT_EQ(0, status.http_code);
    EXPECT_EQ(35, status.curl_code);
    EXPECT_STREQ("upload.example.com", status.fqdn);
    EXPECT_STREQ(test_error, status.error_message);
    EXPECT_TRUE(status.auth_success);
    EXPECT_FALSE(status.upload_completed);
}

TEST_F(UploadStatusTest, ThreadLocalState_MultipleOperations) {
    // Simulate multiple status updates
    __uploadutil_set_status(404, 0);
    __uploadutil_set_fqdn("first.example.com");
    __uploadutil_set_md5("hash1");
    __uploadutil_set_ocsp(true);
    
    // Update with new values
    __uploadutil_set_status(200, 0);
    __uploadutil_set_fqdn("second.example.com");
    __uploadutil_set_md5("hash2");
    __uploadutil_set_ocsp(false);
    
    // Verify latest values
    long http_code;
    int curl_code;
    __uploadutil_get_status(&http_code, &curl_code);
    EXPECT_EQ(200, http_code);
    
    char fqdn[256];
    __uploadutil_get_fqdn(fqdn, sizeof(fqdn));
    EXPECT_STREQ("second.example.com", fqdn);
    
    const char* md5 = __uploadutil_get_md5();
    EXPECT_STREQ("hash2", md5);
    
    EXPECT_FALSE(__uploadutil_get_ocsp());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}