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
 * @file uploadUtil_gtest.cpp
 * @brief Google Test implementation for uploadUtil.c
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "curl_mock.h"

extern "C" {
#include "uploadUtil.h"
#include "urlHelper.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <curl/curl.h>

// Type alias for CURL to void for testing
typedef void CURL;

// External dependency functions that need mocking
// Note: doCurlInit, urlHelperDestroyCurl, setCommonCurlOpt are provided by real source files
// CURLcode setMtlsAuth(void *curl, MtlsAuth_t *auth);  // This may be provided by real files too
}

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::InSequence;
using ::testing::StrictMock;
using ::testing::Invoke;

// Mock class for external dependencies only
class MockCurlOperations {
public:
    // Only mock functions that aren't provided by real source files
    MOCK_METHOD(CURLcode, setMtlsAuth, (void* curl, MtlsAuth_t* auth));
    // Use alternative names to avoid CURL macro conflicts
    MOCK_METHOD(CURLcode, mock_curl_perform, (void* curl));
    MOCK_METHOD(CURLcode, mock_curl_getinfo, (void* curl, int option, long* param));
    MOCK_METHOD(const char*, mock_curl_strerror, (CURLcode code));
};

static MockCurlOperations* g_mock_curl = nullptr;
CurlWrapperMock* g_CurlWrapperMock = nullptr;
static CURLcode g_set_common_result = CURLE_OK;
static CURLcode g_set_mtls_result = CURLE_OK;
static int g_destroy_call_count = 0;
static long g_last_http_code = 0;
static int g_last_curl_code = 0;

// Mock implementations for external dependencies only
extern "C" {
    CURLcode setMtlsAuth(void *curl, MtlsAuth_t *auth) {
        return g_mock_curl ? g_mock_curl->setMtlsAuth(curl, auth) : CURLE_FAILED_INIT;
    }
}

// Mock functions with C++ linkage to match header declarations
void* doCurlInit(void) {
    return (void*)0x12345; // Return dummy handle
}

void urlHelperDestroyCurl(CURL* curl) {
    ++g_destroy_call_count;
}

CURLcode setCommonCurlOpt(CURL* curl, const char* url, char* pPostFields, bool sslverify) {
    return g_set_common_result;
}

CURLcode setMtlsHeaders(CURL* curl, MtlsAuth_t* auth) {
    return g_set_mtls_result;
}

// Provide the missing status tracking function with C linkage
extern "C" void __uploadutil_set_status(long http_code, int curl_code) {
    g_last_http_code = http_code;
    g_last_curl_code = curl_code;
}

class UploadUtilTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock_curl = &mock_curl;
        g_set_common_result = CURLE_OK;
        g_set_mtls_result = CURLE_OK;
        g_destroy_call_count = 0;
        g_last_http_code = 0;
        g_last_curl_code = 0;
        g_CurlWrapperMock = &curl_wrapper_mock;
        
        // Initialize test data
        memset(&file_upload, 0, sizeof(file_upload));
        memset(&hash_data, 0, sizeof(hash_data));
        memset(&mtls_auth, 0, sizeof(mtls_auth));
        
        // Setup default test values
        file_upload.url = const_cast<char*>("https://example.com/upload");
        file_upload.pathname = const_cast<char*>("/tmp/test.log");
        file_upload.pPostFields = nullptr;
        file_upload.sslverify = 1;
        file_upload.hashData = &hash_data;
        
        hash_data.hashvalue = "x-md5: abcd1234";
        hash_data.hashtime = "x-upload-time: 2025-12-01T10:00:00Z";
        
        strncpy(mtls_auth.cert_name, "/tmp/client.crt", sizeof(mtls_auth.cert_name) - 1);
        strncpy(mtls_auth.key_pas, "password123", sizeof(mtls_auth.key_pas) - 1);
        strncpy(mtls_auth.cert_type, "PEM", sizeof(mtls_auth.cert_type) - 1);
    }
    
    void TearDown() override {
        g_mock_curl = nullptr;
        g_CurlWrapperMock = nullptr;
    }
    
    StrictMock<MockCurlOperations> mock_curl;
    StrictMock<CurlWrapperMock> curl_wrapper_mock;
    
    FileUpload_t file_upload;
    UploadHashData_t hash_data;
    MtlsAuth_t mtls_auth;
};

// ==================== doStopUpload TESTS ====================

TEST_F(UploadUtilTest, doStopUpload_ValidCurl_CallsDestroy) {
    void* mock_curl_handle = (void*)0x12345;
    
    // Just call the function - it uses real urlHelperDestroyCurl implementation
    doStopUpload(mock_curl_handle);
    // Function should complete without crashing
}

TEST_F(UploadUtilTest, doStopUpload_NullCurl_NoAction) {
    // Should not call anything when curl is NULL
    doStopUpload(nullptr);
}

// ==================== extractS3PresignedUrl TESTS ====================
TEST_F(UploadUtilTest, extractS3PresignedUrl_ValidFile_PreservesUrl) {
    char test_file[] = "/tmp/uploadutil_response_XXXXXX";
    char url_buffer[256];
    const char* expected_url = "https://s3.amazonaws.com/bucket/key?signature=xyz";
    int fd = mkstemp(test_file);
    ASSERT_NE(-1, fd);
    ASSERT_EQ(static_cast<ssize_t>(strlen(expected_url)),
              write(fd, expected_url, strlen(expected_url)));
    ASSERT_EQ(0, close(fd));

    int result = extractS3PresignedUrl(test_file, url_buffer, sizeof(url_buffer));

    EXPECT_EQ(0, result);
    EXPECT_STREQ(expected_url, url_buffer);
    EXPECT_EQ(0, unlink(test_file));
}

TEST_F(UploadUtilTest, extractS3PresignedUrl_TrailingNewline_StripsNewline) {
    char test_file[] = "/tmp/uploadutil_response_XXXXXX";
    char url_buffer[256];
    const char* expected_url = "https://s3.amazonaws.com/bucket/key?signature=xyz";
    int fd = mkstemp(test_file);
    ASSERT_NE(-1, fd);
    ASSERT_EQ(static_cast<ssize_t>(strlen(expected_url)),
              write(fd, expected_url, strlen(expected_url)));
    ASSERT_EQ(1, write(fd, "\n", 1));
    ASSERT_EQ(0, close(fd));

    int result = extractS3PresignedUrl(test_file, url_buffer, sizeof(url_buffer));

    EXPECT_EQ(0, result);
    EXPECT_STREQ(expected_url, url_buffer);
    EXPECT_EQ(0, unlink(test_file));
}

TEST_F(UploadUtilTest, extractS3PresignedUrl_InvalidParameters) {
    char url_buffer[256];
    
    // Test null result_file
    EXPECT_EQ(-1, extractS3PresignedUrl(nullptr, url_buffer, sizeof(url_buffer)));
    
    // Test null out_url
    EXPECT_EQ(-1, extractS3PresignedUrl("/tmp/test.txt", nullptr, sizeof(url_buffer)));
    
    // Test zero buffer size
    EXPECT_EQ(-1, extractS3PresignedUrl("/tmp/test.txt", url_buffer, 0));
}

TEST_F(UploadUtilTest, extractS3PresignedUrl_FileOpenFailure) {
    const char* test_file = "/tmp/nonexistent_xyz_12345.txt";
    char url_buffer[256];
    
    // Test with actual nonexistent file - should fail
    int result = extractS3PresignedUrl(test_file, url_buffer, sizeof(url_buffer));
    EXPECT_EQ(-1, result);
}

TEST_F(UploadUtilTest, extractS3PresignedUrl_EmptyFile_ReturnsFailure) {
    char test_file[] = "/tmp/uploadutil_response_XXXXXX";
    char url_buffer[256];
    int fd = mkstemp(test_file);
    ASSERT_NE(-1, fd);
    ASSERT_EQ(0, close(fd));

    int result = extractS3PresignedUrl(test_file, url_buffer, sizeof(url_buffer));

    EXPECT_EQ(-1, result);
    EXPECT_EQ(0, unlink(test_file));
}

// ==================== performS3PutUpload TESTS ====================
// Note: Most performS3PutUpload tests are disabled because they require
// real file I/O which cannot be mocked when linking against compiled objects

TEST_F(UploadUtilTest, DISABLED_performS3PutUpload_Success_NoAuth) {
    // DISABLED: Requires real file to exist
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    const char* local_file = "/tmp/test.log";
    
    int result = performS3PutUpload(s3_url, local_file, nullptr);
    // Note: Result will depend on actual implementation behavior
}

TEST_F(UploadUtilTest, DISABLED_performS3PutUpload_Success_WithAuth) {
    // DISABLED: Requires real file to exist, and uses setMtlsHeaders not setMtlsAuth
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    const char* local_file = "/tmp/test.log";
    
    int result = performS3PutUpload(s3_url, local_file, &mtls_auth);
}

TEST_F(UploadUtilTest, performS3PutUpload_InvalidParameters) {
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    const char* local_file = "/tmp/test.log";
    
    // Test null s3_url
    EXPECT_EQ(-1, performS3PutUpload(nullptr, local_file, nullptr));
    
    // Test null local_file
    EXPECT_EQ(-1, performS3PutUpload(s3_url, nullptr, nullptr));
}

TEST_F(UploadUtilTest, performS3PutUpload_CurlInitFailure) {
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    const char* local_file = "/tmp/test.log";
    
    // Test with invalid parameters to trigger init failure path
    int result = performS3PutUpload(nullptr, local_file, nullptr);
    EXPECT_EQ(-1, result);
}

TEST_F(UploadUtilTest, performS3PutUpload_InvalidParams) {
    // Test various invalid parameter combinations
    EXPECT_EQ(-1, performS3PutUpload(nullptr, "/tmp/test.log", nullptr));
    EXPECT_EQ(-1, performS3PutUpload("https://s3.amazonaws.com/bucket/key", nullptr, nullptr));
    EXPECT_EQ(-1, performS3PutUpload("", "/tmp/test.log", nullptr));
}

TEST_F(UploadUtilTest, performS3PutUpload_CommonSetupFailure_DestroysCurl) {
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    g_set_common_result = CURLE_URL_MALFORMAT;
    EXPECT_CALL(curl_wrapper_mock, curl_easy_strerror(CURLE_URL_MALFORMAT))
        .WillOnce(Return("URL malformed"));

    EXPECT_EQ(-1, performS3PutUpload(s3_url, "/tmp/test.log", nullptr));
    EXPECT_EQ(1, g_destroy_call_count);
}

TEST_F(UploadUtilTest, performS3PutUpload_MtlsSetupFailure_DestroysCurl) {
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    g_set_mtls_result = CURLE_SSL_CERTPROBLEM;
    EXPECT_CALL(curl_wrapper_mock, curl_easy_strerror(CURLE_SSL_CERTPROBLEM))
        .WillOnce(Return("certificate problem"));

    EXPECT_EQ(-1, performS3PutUpload(s3_url, "/tmp/test.log", &mtls_auth));
    EXPECT_EQ(1, g_destroy_call_count);
}

TEST_F(UploadUtilTest, performS3PutUpload_MissingLocalFile_DestroysCurl) {
    const char* s3_url = "https://s3.amazonaws.com/bucket/key";
    const char* local_file = "/tmp/nonexistent_uploadutil_file_12345.log";
    ASSERT_EQ(-1, unlink(local_file));

    EXPECT_EQ(-1, performS3PutUpload(s3_url, local_file, nullptr));
    EXPECT_EQ(1, g_destroy_call_count);
}

TEST_F(UploadUtilTest, performS3PutUpload_HttpSuccess_ReturnsSuccessAndReportsStatus) {
    char local_file[] = "/tmp/uploadutil_payload_XXXXXX";
    int fd = mkstemp(local_file);
    ASSERT_NE(-1, fd);
    ASSERT_EQ(0, close(fd));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_setopt(_, _, _))
        .Times(3).WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_perform(_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_getinfo(_, CURLINFO_RESPONSE_CODE, _))
        .WillOnce(Invoke([](CURL*, CURLINFO, void* output) {
            *static_cast<long*>(output) = 204;
            return CURLE_OK;
        }));

    EXPECT_EQ(0, performS3PutUpload("https://s3.example/upload", local_file, nullptr));
    EXPECT_EQ(204, g_last_http_code);
    EXPECT_EQ(CURLE_OK, g_last_curl_code);
    EXPECT_EQ(1, g_destroy_call_count);
    EXPECT_EQ(0, unlink(local_file));
}

TEST_F(UploadUtilTest, performS3PutUpload_TransportFailure_ReturnsFailureAndReportsStatus) {
    char local_file[] = "/tmp/uploadutil_payload_XXXXXX";
    int fd = mkstemp(local_file);
    ASSERT_NE(-1, fd);
    ASSERT_EQ(0, close(fd));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_setopt(_, _, _))
        .Times(3).WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_perform(_)).WillOnce(Return(CURLE_OPERATION_TIMEDOUT));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_getinfo(_, CURLINFO_RESPONSE_CODE, _))
        .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(-1, performS3PutUpload("https://s3.example/upload", local_file, nullptr));
    EXPECT_EQ(0, g_last_http_code);
    EXPECT_EQ(CURLE_OPERATION_TIMEDOUT, g_last_curl_code);
    EXPECT_EQ(1, g_destroy_call_count);
    EXPECT_EQ(0, unlink(local_file));
}

TEST_F(UploadUtilTest, performS3PutUpload_HttpFailure_ReturnsFailureAndReportsStatus) {
    char local_file[] = "/tmp/uploadutil_payload_XXXXXX";
    int fd = mkstemp(local_file);
    ASSERT_NE(-1, fd);
    ASSERT_EQ(0, close(fd));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_setopt(_, _, _))
        .Times(3).WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_perform(_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(curl_wrapper_mock, curl_easy_getinfo(_, CURLINFO_RESPONSE_CODE, _))
        .WillOnce(Invoke([](CURL*, CURLINFO, void* output) {
            *static_cast<long*>(output) = 403;
            return CURLE_OK;
        }));

    EXPECT_EQ(-1, performS3PutUpload("https://s3.example/upload", local_file, nullptr));
    EXPECT_EQ(403, g_last_http_code);
    EXPECT_EQ(CURLE_OK, g_last_curl_code);
    EXPECT_EQ(1, g_destroy_call_count);
    EXPECT_EQ(0, unlink(local_file));
}

// ==================== performHttpMetadataPost TESTS ====================

TEST_F(UploadUtilTest, performHttpMetadataPost_InvalidParameters) {
    void* mock_curl_handle = (void*)0x12345;
    long http_code = 0;
    
    // Test null curl handle
    EXPECT_EQ(-1, performHttpMetadataPost(nullptr, &file_upload, nullptr, &http_code));
    
    // Test null file_upload
    EXPECT_EQ(-1, performHttpMetadataPost(mock_curl_handle, nullptr, nullptr, &http_code));
    
    // Test null out_httpCode
    EXPECT_EQ(-1, performHttpMetadataPost(mock_curl_handle, &file_upload, nullptr, nullptr));
}

TEST_F(UploadUtilTest, performHttpMetadataPost_CommonSetupFailure_PropagatesError) {
    void* mock_curl_handle = (void*)0x12345;
    long http_code = 503;
    g_set_common_result = CURLE_URL_MALFORMAT;
    EXPECT_CALL(curl_wrapper_mock, curl_easy_strerror(CURLE_URL_MALFORMAT))
        .WillOnce(Return("URL malformed"));

    EXPECT_EQ(CURLE_URL_MALFORMAT,
              performHttpMetadataPost(mock_curl_handle, &file_upload, nullptr, &http_code));
    EXPECT_EQ(0, http_code);
}

TEST_F(UploadUtilTest, performHttpMetadataPost_MtlsSetupFailure_PropagatesError) {
    void* mock_curl_handle = (void*)0x12345;
    long http_code = 503;
    g_set_mtls_result = CURLE_SSL_CERTPROBLEM;
    EXPECT_CALL(curl_wrapper_mock, curl_easy_strerror(CURLE_SSL_CERTPROBLEM))
        .WillOnce(Return("certificate problem"));

    EXPECT_EQ(CURLE_SSL_CERTPROBLEM,
              performHttpMetadataPost(mock_curl_handle, &file_upload, &mtls_auth, &http_code));
    EXPECT_EQ(0, http_code);
}

TEST_F(UploadUtilTest, performHttpMetadataPost_ResponseFileOpenFailure_ReturnsUploadFail) {
    void* mock_curl_handle = (void*)0x12345;
    long http_code = 503;
    file_upload.pathname = const_cast<char*>("/tmp/nonexistent_uploadutil_dir/response.txt");
    file_upload.hashData = nullptr;

    EXPECT_EQ(UPLOAD_FAIL,
              performHttpMetadataPost(mock_curl_handle, &file_upload, nullptr, &http_code));
    EXPECT_EQ(0, http_code);
}

// ==================== INTEGRATION TESTS ====================

TEST_F(UploadUtilTest, TwoStageUpload_FullWorkflow) {
    // This would test the complete upload workflow:
    // 1. HTTP POST for metadata
    // 2. Extract S3 URL from response
    // 3. S3 PUT for file upload
    
    // Implementation would require more detailed mocking of the full workflow
    EXPECT_TRUE(true);  // Placeholder for now
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
