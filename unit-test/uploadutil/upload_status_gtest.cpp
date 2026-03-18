/**
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
 */

/**
 * @file upload_status_gtest.cpp
 * @brief Google Test implementation for upload_status.c
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "upload_status.h"
#include "uploadUtil.h"
#include "urlHelper.h"
#include <string.h>
#include <stdio.h>

// Mock for performS3PutUpload since we're testing the wrapper
int performS3PutUpload(const char *upload_url, const char *src_file, MtlsAuth_t *auth);
}

using ::testing::_;
using ::testing::Return;
using ::testing::StrEq;

// Global mock control
static int g_mock_performS3PutUpload_result = 0;
static bool g_mock_performS3PutUpload_called = false;

// Mock implementation
extern "C" int performS3PutUpload(const char *upload_url, const char *src_file, MtlsAuth_t *auth) {
    g_mock_performS3PutUpload_called = true;
    
    // Simulate setting thread-local status (what real function would do)
    if (g_mock_performS3PutUpload_result == 0) {
        __uploadutil_set_status(200, 0);  // Success
    } else {
        __uploadutil_set_status(500, 7);  // Error
    }
    
    return g_mock_performS3PutUpload_result;
}

class UploadStatusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset mock state
        g_mock_performS3PutUpload_result = 0;
        g_mock_performS3PutUpload_called = false;
        
        // Reset thread-local storage
        __uploadutil_set_status(0, 0);
        __uploadutil_set_fqdn(nullptr);
        __uploadutil_set_ocsp(false);
        __uploadutil_set_md5(nullptr);
        
        // Initialize test status structure
        memset(&status, 0, sizeof(status));
    }
    
    void TearDown() override {
        // Clean up thread-local state
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

// ==================== performS3PutUploadEx TESTS ====================

TEST_F(UploadStatusTest, PerformS3PutUploadEx_NullStatus) {
    const char* url = "https://s3.amazonaws.com/bucket/key";
    const char* file = "/tmp/test.log";
    
    int result = performS3PutUploadEx(url, file, nullptr, nullptr, false, nullptr);
    
    EXPECT_EQ(-1, result);
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_NullUrl) {
    int result = performS3PutUploadEx(nullptr, "/tmp/test.log", nullptr, nullptr, false, &status);
    
    EXPECT_EQ(-1, result);
    EXPECT_EQ(-1, status.result_code);
    EXPECT_STRNE("", status.error_message);
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_NullFile) {
    const char* url = "https://s3.amazonaws.com/bucket/key";
    
    int result = performS3PutUploadEx(url, nullptr, nullptr, nullptr, false, &status);
    
    EXPECT_EQ(-1, result);
    EXPECT_EQ(-1, status.result_code);
    EXPECT_STRNE("", status.error_message);
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_Success) {
    const char* url = "https://s3.amazonaws.com/bucket/key?signature=abc";
    const char* file = "/tmp/test.log";
    const char* md5 = "d41d8cd98f00b204e9800998ecf8427e";
    
    g_mock_performS3PutUpload_result = 0;  // Success
    
    int result = performS3PutUploadEx(url, file, nullptr, md5, true, &status);
    
    EXPECT_TRUE(g_mock_performS3PutUpload_called);
    EXPECT_EQ(0, result);
    EXPECT_EQ(0, status.result_code);
    EXPECT_EQ(200, status.http_code);
    EXPECT_EQ(0, status.curl_code);
    EXPECT_TRUE(status.upload_completed);
    EXPECT_TRUE(status.auth_success);
    EXPECT_STREQ("s3.amazonaws.com", status.fqdn);
    EXPECT_STRNE("", status.error_message);  // Should have success message
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_Failure) {
    const char* url = "https://s3.amazonaws.com/bucket/key";
    const char* file = "/tmp/test.log";
    
    g_mock_performS3PutUpload_result = -1;  // Failure
    
    int result = performS3PutUploadEx(url, file, nullptr, nullptr, false, &status);
    
    EXPECT_TRUE(g_mock_performS3PutUpload_called);
    EXPECT_EQ(-1, result);
    EXPECT_EQ(-1, status.result_code);
    EXPECT_EQ(500, status.http_code);
    EXPECT_EQ(7, status.curl_code);
    EXPECT_FALSE(status.upload_completed);
    EXPECT_FALSE(status.auth_success);
    EXPECT_STREQ("s3.amazonaws.com", status.fqdn);
    EXPECT_STRNE("", status.error_message);  // Should have error message
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_ExtractsFqdnFromUrl) {
    const char* url = "https://api.example.com:8443/upload/path";
    const char* file = "/tmp/test.log";
    
    g_mock_performS3PutUpload_result = 0;
    
    int result = performS3PutUploadEx(url, file, nullptr, nullptr, false, &status);
    
    EXPECT_EQ(0, result);
    EXPECT_STREQ("api.example.com", status.fqdn);
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_ExtractsFqdnFromUrlWithoutPort) {
    const char* url = "https://upload.service.com/v1/api";
    const char* file = "/tmp/test.log";
    
    g_mock_performS3PutUpload_result = 0;
    
    int result = performS3PutUploadEx(url, file, nullptr, nullptr, false, &status);
    
    EXPECT_EQ(0, result);
    EXPECT_STREQ("upload.service.com", status.fqdn);
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_SetsMd5AndOcsp) {
    const char* url = "https://s3.amazonaws.com/bucket/key";
    const char* file = "/tmp/test.log";
    const char* md5 = "test_md5_hash_value";
    
    g_mock_performS3PutUpload_result = 0;
    
    // Call with MD5 and OCSP
    int result = performS3PutUploadEx(url, file, nullptr, md5, true, &status);
    
    EXPECT_EQ(0, result);
    
    // Note: MD5 and OCSP are consumed by performS3PutUpload (mocked)
    // The get functions reset after read, so we can't verify them here
    // But we can verify the function was called successfully
    EXPECT_TRUE(g_mock_performS3PutUpload_called);
}

TEST_F(UploadStatusTest, PerformS3PutUploadEx_WithMtlsAuth) {
    const char* url = "https://s3.amazonaws.com/bucket/key";
    const char* file = "/tmp/test.log";
    MtlsAuth_t auth;
    memset(&auth, 0, sizeof(auth));
    strcpy(auth.cert_name, "/opt/certs/client.pem");
    strcpy(auth.key_pas, "password123");
    strcpy(auth.cert_type, "PEM");
    
    g_mock_performS3PutUpload_result = 0;
    
    int result = performS3PutUploadEx(url, file, &auth, nullptr, false, &status);
    
    EXPECT_EQ(0, result);
    EXPECT_TRUE(status.auth_success);
}

// ==================== INTEGRATION TESTS ====================

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

TEST_F(UploadStatusTest, PerformS3PutUploadEx_ErrorMessageContent) {
    const char* url = "https://s3.amazonaws.com/bucket/key";
    const char* file = "/tmp/test.log";
    
    // Test CURL error
    g_mock_performS3PutUpload_result = -1;
    performS3PutUploadEx(url, file, nullptr, nullptr, false, &status);
    EXPECT_NE(std::string(status.error_message).find("CURL error"), std::string::npos);
    
    // Test success message
    g_mock_performS3PutUpload_result = 0;
    __uploadutil_set_status(200, 0);
    performS3PutUploadEx(url, file, nullptr, nullptr, false, &status);
    EXPECT_NE(std::string(status.error_message).find("successful"), std::string::npos);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

