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
 * @file mtls_upload_gtest.cpp
 * @brief Google Test cases for mTLS upload with certificate rotation
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <cstring>
#include <cstdio>
#include <fstream>

extern "C" {
    #include "mtls_upload.h"
    #include "downloadUtil.h"
}

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::NotNull;
using ::testing::DoAll;
using ::testing::InSequence;

// Mock rdkcertselector functions
extern "C" {
    // Status tracking mocks
    void __uploadutil_set_status(long http_code, int curl_code);
    const char* __uploadutil_get_md5(void);
    bool __uploadutil_get_ocsp(void);
    
    // rdkcertselector mocks
    rdkcertselector_h rdkcertselector_new(const char* prefix, const char* suffix, const char* purpose);
    rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h handle, char** certUri, char** certPass);
    char* rdkcertselector_getEngine(rdkcertselector_h handle);
    rdkcertselectorStatus_t rdkcertselector_setCurlStatus(rdkcertselector_h handle, int curl_code, const char* url);
    void rdkcertselector_free(rdkcertselector_h* handle);
    
    // Download utility mocks  
    void* doCurlInit(void);
    void doStopUpload(void* curl);
    int performHttpMetadataPost(void* curl, FileUpload_t* file_upload, MtlsAuth_t* sec, long* http_code);
    int extractS3PresignedUrl(const char* result_file, char* s3_url, size_t url_size);
    int performS3PutUpload(const char* s3_url, const char* src_file, MtlsAuth_t* sec);
    
    // CURL mocks
    CURLcode curl_easy_setopt(void* curl, int option, long value);
    const char* curl_easy_strerror(CURLcode errornum);
}

// Mock class for mTLS upload dependencies
class MockMtlsUploadDeps {
public:
    MOCK_METHOD(void, uploadutil_set_status, (long http_code, int curl_code));
    MOCK_METHOD(const char*, uploadutil_get_md5, ());
    MOCK_METHOD(bool, uploadutil_get_ocsp, ());
    
    MOCK_METHOD(rdkcertselector_h, rdkcertselector_new, (const char* prefix, const char* suffix, const char* purpose));
    MOCK_METHOD(rdkcertselectorStatus_t, rdkcertselector_getCert, (rdkcertselector_h handle, char** certUri, char** certPass));
    MOCK_METHOD(char*, rdkcertselector_getEngine, (rdkcertselector_h handle));
    MOCK_METHOD(rdkcertselectorStatus_t, rdkcertselector_setCurlStatus, (rdkcertselector_h handle, int curl_code, const char* url));
    MOCK_METHOD(void, rdkcertselector_free, (rdkcertselector_h* handle));
    
    MOCK_METHOD(void*, doCurlInit, ());
    MOCK_METHOD(void, doStopUpload, (void* curl));
    MOCK_METHOD(int, performHttpMetadataPost, (void* curl, FileUpload_t* file_upload, MtlsAuth_t* sec, long* http_code));
    MOCK_METHOD(int, extractS3PresignedUrl, (const char* result_file, char* s3_url, size_t url_size));
    MOCK_METHOD(int, performS3PutUpload, (const char* s3_url, const char* src_file, MtlsAuth_t* sec));
    
    MOCK_METHOD(CURLcode, curl_easy_setopt, (void* curl, int option, long value));
    MOCK_METHOD(const char*, curl_easy_strerror, (CURLcode errornum));
};

// Global mock instance
static MockMtlsUploadDeps* g_mock = nullptr;

// Mock function implementations
extern "C" {
    void __uploadutil_set_status(long http_code, int curl_code) {
        if (g_mock) g_mock->uploadutil_set_status(http_code, curl_code);
    }
    
    const char* __uploadutil_get_md5(void) {
        return g_mock ? g_mock->uploadutil_get_md5() : nullptr;
    }
    
    bool __uploadutil_get_ocsp(void) {
        return g_mock ? g_mock->uploadutil_get_ocsp() : false;
    }
    
    rdkcertselector_h rdkcertselector_new(const char* prefix, const char* suffix, const char* purpose) {
        return g_mock ? g_mock->rdkcertselector_new(prefix, suffix, purpose) : nullptr;
    }
    
    rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h handle, char** certUri, char** certPass) {
        return g_mock ? g_mock->rdkcertselector_getCert(handle, certUri, certPass) : certselectorFail;
    }
    
    char* rdkcertselector_getEngine(rdkcertselector_h handle) {
        return g_mock ? g_mock->rdkcertselector_getEngine(handle) : nullptr;
    }
    
    rdkcertselectorStatus_t rdkcertselector_setCurlStatus(rdkcertselector_h handle, int curl_code, const char* url) {
        return g_mock ? g_mock->rdkcertselector_setCurlStatus(handle, curl_code, url) : DONT_TRY_ANOTHER;
    }
    
    void rdkcertselector_free(rdkcertselector_h* handle) {
        if (g_mock) g_mock->rdkcertselector_free(handle);
        if (handle) *handle = nullptr;
    }
    
    void* doCurlInit(void) {
        return g_mock ? g_mock->doCurlInit() : nullptr;
    }
    
    void doStopUpload(void* curl) {
        if (g_mock) g_mock->doStopUpload(curl);
    }
    
    int performHttpMetadataPost(void* curl, FileUpload_t* file_upload, MtlsAuth_t* sec, long* http_code) {
        return g_mock ? g_mock->performHttpMetadataPost(curl, file_upload, sec, http_code) : -1;
    }
    
    int extractS3PresignedUrl(const char* result_file, char* s3_url, size_t url_size) {
        return g_mock ? g_mock->extractS3PresignedUrl(result_file, s3_url, url_size) : -1;
    }
    
    int performS3PutUpload(const char* s3_url, const char* src_file, MtlsAuth_t* sec) {
        return g_mock ? g_mock->performS3PutUpload(s3_url, src_file, sec) : -1;
    }
    
    CURLcode curl_easy_setopt(void* curl, int option, long value) {
        return g_mock ? g_mock->curl_easy_setopt(curl, option, value) : CURLE_OK;
    }
    
    const char* curl_easy_strerror(CURLcode errornum) {
        return g_mock ? g_mock->curl_easy_strerror(errornum) : "Mock error";
    }
}

// Test fixture for mTLS upload tests
class MtlsUploadTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock = &mock_deps;
    }
    
    void TearDown() override {
        g_mock = nullptr;
    }
    
    MockMtlsUploadDeps mock_deps;
    
    // Helper to create mock certificate strings
    char* CreateMockCertUri(const char* path) {
        char* uri = static_cast<char*>(malloc(strlen(path) + 10));
        if (strncmp(path, "file://", 7) == 0) {
            strcpy(uri, path);
        } else {
            sprintf(uri, "file://%s", path);
        }
        return uri;
    }
    
    char* CreateMockPassword(const char* password) {
        char* pass = static_cast<char*>(malloc(strlen(password) + 1));
        strcpy(pass, password);
        return pass;
    }
};

#ifdef LIBRDKCERTSELECTOR

// Tests for getCertificateForUpload function
TEST_F(MtlsUploadTest, GetCertificateForUpload_NullParameters) {
    rdkcertselector_h dummy_handle = reinterpret_cast<rdkcertselector_h>(0x12345);
    MtlsAuth_t sec;
    
    // Test null sec parameter
    EXPECT_EQ(MTLS_CERT_FETCH_FAILURE, getCertificateForUpload(nullptr, &dummy_handle));
    
    // Test null handle parameter
    EXPECT_EQ(MTLS_CERT_FETCH_FAILURE, getCertificateForUpload(&sec, nullptr));
}

TEST_F(MtlsUploadTest, GetCertificateForUpload_CertFetchFailure) {
    rdkcertselector_h dummy_handle = reinterpret_cast<rdkcertselector_h>(0x12345);
    MtlsAuth_t sec;
    
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(dummy_handle, _, _))
        .WillOnce(Return(certselectorFail));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&dummy_handle));
    
    EXPECT_EQ(MTLS_CERT_FETCH_FAILURE, getCertificateForUpload(&sec, &dummy_handle));
}

TEST_F(MtlsUploadTest, GetCertificateForUpload_NullCertUri) {
    rdkcertselector_h dummy_handle = reinterpret_cast<rdkcertselector_h>(0x12345);
    MtlsAuth_t sec;
    char* mock_password = CreateMockPassword("testpass123");
    
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(dummy_handle, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(static_cast<char*>(nullptr)),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&dummy_handle));
    
    EXPECT_EQ(MTLS_CERT_FETCH_FAILURE, getCertificateForUpload(&sec, &dummy_handle));
    free(mock_password);
}

TEST_F(MtlsUploadTest, GetCertificateForUpload_NullPassword) {
    rdkcertselector_h dummy_handle = reinterpret_cast<rdkcertselector_h>(0x12345);
    MtlsAuth_t sec;
    char* mock_cert_uri = CreateMockCertUri("/path/to/cert.p12");
    
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(dummy_handle, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(static_cast<char*>(nullptr)),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&dummy_handle));
    
    EXPECT_EQ(MTLS_CERT_FETCH_FAILURE, getCertificateForUpload(&sec, &dummy_handle));
    free(mock_cert_uri);
}

TEST_F(MtlsUploadTest, GetCertificateForUpload_SuccessWithFileScheme) {
    rdkcertselector_h dummy_handle = reinterpret_cast<rdkcertselector_h>(0x12345);
    MtlsAuth_t sec;
    char* mock_cert_uri = CreateMockCertUri("file:///opt/secure/client.p12");
    char* mock_password = CreateMockPassword("securepass456");
    char* mock_engine = static_cast<char*>(malloc(10));
    strcpy(mock_engine, "pkcs11");
    
    memset(&sec, 0, sizeof(sec));
    
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(dummy_handle, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_getEngine(dummy_handle))
        .WillOnce(Return(mock_engine));
    
    EXPECT_EQ(MTLS_CERT_FETCH_SUCCESS, getCertificateForUpload(&sec, &dummy_handle));
    
    // Verify file:// scheme is stripped
    EXPECT_STREQ("/opt/secure/client.p12", sec.cert_name);
    EXPECT_STREQ("securepass456", sec.key_pas);
    EXPECT_STREQ("pkcs11", sec.engine);
    EXPECT_STREQ("P12", sec.cert_type);
    
    free(mock_cert_uri);
    free(mock_password);
    free(mock_engine);
}

TEST_F(MtlsUploadTest, GetCertificateForUpload_SuccessWithoutFileScheme) {
    rdkcertselector_h dummy_handle = reinterpret_cast<rdkcertselector_h>(0x12345);
    MtlsAuth_t sec;
    char* mock_cert_uri = CreateMockCertUri("/direct/path/cert.p12");
    char* mock_password = CreateMockPassword("directpass789");
    
    memset(&sec, 0, sizeof(sec));
    
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(dummy_handle, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_getEngine(dummy_handle))
        .WillOnce(Return(static_cast<char*>(nullptr)));
    
    EXPECT_EQ(MTLS_CERT_FETCH_SUCCESS, getCertificateForUpload(&sec, &dummy_handle));
    
    // Verify path is used directly (no file:// prefix to strip)
    EXPECT_STREQ("file:///direct/path/cert.p12", sec.cert_name);
    EXPECT_STREQ("directpass789", sec.key_pas);
    EXPECT_STREQ("", sec.engine);  // Empty when getEngine returns null
    EXPECT_STREQ("P12", sec.cert_type);
    
    free(mock_cert_uri);
    free(mock_password);
}

#endif  // LIBRDKCERTSELECTOR

// Tests for uploadFileWithTwoStageFlow function
TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_NullParameters) {
    // Test null upload_url
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow(nullptr, "/path/to/file.txt"));
    
    // Test null src_file
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", nullptr));
    
    // Test both null
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow(nullptr, nullptr));
}

#ifdef LIBRDKCERTSELECTOR

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_CurlInitFailure) {
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(static_cast<void*>(nullptr)));
    
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
}

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_CertSelectorInitFailure) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_deps, rdkcertselector_new(nullptr, nullptr, StrEq("MTLS")))
        .WillOnce(Return(static_cast<rdkcertselector_h>(nullptr)));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
}

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_SuccessWithMD5AndOCSP) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    rdkcertselector_h mock_cert_sel = reinterpret_cast<rdkcertselector_h>(0x22222);
    char* mock_cert_uri = CreateMockCertUri("file:///opt/certs/upload.p12");
    char* mock_password = CreateMockPassword("upload_secret");
    char* mock_engine = static_cast<char*>(malloc(8));
    strcpy(mock_engine, "openssl");
    
    // Setup expectations
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillRepeatedly(Return("d41d8cd98f00b204e9800998ecf8427e"));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(true));
    EXPECT_CALL(mock_deps, curl_easy_setopt(mock_curl, CURLOPT_SSL_VERIFYSTATUS, 1L))
        .WillOnce(Return(CURLE_OK));
    
    // Certificate selector initialization and operations
    EXPECT_CALL(mock_deps, rdkcertselector_new(nullptr, nullptr, StrEq("MTLS")))
        .WillOnce(Return(mock_cert_sel));
    
    // Certificate fetch for upload
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(mock_cert_sel, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_getEngine(mock_cert_sel))
        .WillOnce(Return(mock_engine));
    
    // Two-stage upload workflow
    long mock_http_code = 201;
    EXPECT_CALL(mock_deps, performHttpMetadataPost(mock_curl, _, _, _))
        .WillOnce(DoAll(
            SetArgPointee<3>(mock_http_code),
            Return(0)
        ));
    
    EXPECT_CALL(mock_deps, extractS3PresignedUrl(StrEq("/tmp/httpresult.txt"), _, _))
        .WillOnce(DoAll(
            testing::WithArg<1>([](char* s3_url) {
                strcpy(s3_url, "https://s3.amazonaws.com/bucket/key?signature=abc123");
            }),
            Return(0)
        ));
    
    EXPECT_CALL(mock_deps, performS3PutUpload(StrEq("https://s3.amazonaws.com/bucket/key?signature=abc123"), 
                                             StrEq("/path/to/file.txt"), _))
        .WillOnce(Return(0));
    
    // Certificate status and cleanup
    EXPECT_CALL(mock_deps, rdkcertselector_setCurlStatus(mock_cert_sel, 0, StrEq("https://example.com/upload")))
        .WillOnce(Return(DONT_TRY_ANOTHER));
    EXPECT_CALL(mock_deps, uploadutil_set_status(201, 0));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&mock_cert_sel));
    
    EXPECT_EQ(0, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
    
    free(mock_cert_uri);
    free(mock_password);
    free(mock_engine);
}

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_MetadataPostFailure) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    rdkcertselector_h mock_cert_sel = reinterpret_cast<rdkcertselector_h>(0x22222);
    char* mock_cert_uri = CreateMockCertUri("/opt/certs/upload.p12");
    char* mock_password = CreateMockPassword("upload_secret");
    
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_deps, rdkcertselector_new(nullptr, nullptr, StrEq("MTLS")))
        .WillOnce(Return(mock_cert_sel));
    
    // Certificate fetch succeeds
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(mock_cert_sel, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_getEngine(mock_cert_sel))
        .WillOnce(Return(static_cast<char*>(nullptr)));
    
    // Metadata POST fails
    long mock_http_code = 500;
    EXPECT_CALL(mock_deps, performHttpMetadataPost(mock_curl, _, _, _))
        .WillOnce(DoAll(
            SetArgPointee<3>(mock_http_code),
            Return(-1)  // Simulate curl error
        ));
    
    // Certificate retry mechanism
    EXPECT_CALL(mock_deps, rdkcertselector_setCurlStatus(mock_cert_sel, -1, StrEq("https://example.com/upload")))
        .WillOnce(Return(DONT_TRY_ANOTHER));
    EXPECT_CALL(mock_deps, uploadutil_set_status(500, -1));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&mock_cert_sel));
    
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
    
    free(mock_cert_uri);
    free(mock_password);
}

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_S3ExtractFailure) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    rdkcertselector_h mock_cert_sel = reinterpret_cast<rdkcertselector_h>(0x22222);
    char* mock_cert_uri = CreateMockCertUri("/opt/certs/upload.p12");
    char* mock_password = CreateMockPassword("upload_secret");
    
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_deps, rdkcertselector_new(nullptr, nullptr, StrEq("MTLS")))
        .WillOnce(Return(mock_cert_sel));
    
    // Certificate fetch succeeds
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(mock_cert_sel, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_getEngine(mock_cert_sel))
        .WillOnce(Return(static_cast<char*>(nullptr)));
    
    // Metadata POST succeeds
    long mock_http_code = 200;
    EXPECT_CALL(mock_deps, performHttpMetadataPost(mock_curl, _, _, _))
        .WillOnce(DoAll(
            SetArgPointee<3>(mock_http_code),
            Return(0)
        ));
    
    // S3 URL extraction fails
    EXPECT_CALL(mock_deps, extractS3PresignedUrl(StrEq("/tmp/httpresult.txt"), _, _))
        .WillOnce(Return(-1));
    
    EXPECT_CALL(mock_deps, rdkcertselector_setCurlStatus(mock_cert_sel, 0, StrEq("https://example.com/upload")))
        .WillOnce(Return(DONT_TRY_ANOTHER));
    EXPECT_CALL(mock_deps, uploadutil_set_status(200, 0));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&mock_cert_sel));
    
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
    
    free(mock_cert_uri);
    free(mock_password);
}

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_S3UploadFailure) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    rdkcertselector_h mock_cert_sel = reinterpret_cast<rdkcertselector_h>(0x22222);
    char* mock_cert_uri = CreateMockCertUri("/opt/certs/upload.p12");
    char* mock_password = CreateMockPassword("upload_secret");
    
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_deps, rdkcertselector_new(nullptr, nullptr, StrEq("MTLS")))
        .WillOnce(Return(mock_cert_sel));
    
    // Certificate fetch succeeds
    EXPECT_CALL(mock_deps, rdkcertselector_getCert(mock_cert_sel, _, _))
        .WillOnce(DoAll(
            SetArgPointee<1>(mock_cert_uri),
            SetArgPointee<2>(mock_password),
            Return(certselectorOk)
        ));
    EXPECT_CALL(mock_deps, rdkcertselector_getEngine(mock_cert_sel))
        .WillOnce(Return(static_cast<char*>(nullptr)));
    
    // Metadata POST succeeds
    long mock_http_code = 200;
    EXPECT_CALL(mock_deps, performHttpMetadataPost(mock_curl, _, _, _))
        .WillOnce(DoAll(
            SetArgPointee<3>(mock_http_code),
            Return(0)
        ));
    
    // S3 URL extraction succeeds
    EXPECT_CALL(mock_deps, extractS3PresignedUrl(StrEq("/tmp/httpresult.txt"), _, _))
        .WillOnce(DoAll(
            testing::WithArg<1>([](char* s3_url) {
                strcpy(s3_url, "https://s3.amazonaws.com/bucket/key?signature=abc123");
            }),
            Return(0)
        ));
    
    // S3 PUT upload fails
    EXPECT_CALL(mock_deps, performS3PutUpload(StrEq("https://s3.amazonaws.com/bucket/key?signature=abc123"), 
                                             StrEq("/path/to/file.txt"), _))
        .WillOnce(Return(-1));
    
    EXPECT_CALL(mock_deps, rdkcertselector_setCurlStatus(mock_cert_sel, 0, StrEq("https://example.com/upload")))
        .WillOnce(Return(DONT_TRY_ANOTHER));
    EXPECT_CALL(mock_deps, uploadutil_set_status(200, 0));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&mock_cert_sel));
    
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
    
    free(mock_cert_uri);
    free(mock_password);
}

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_CertificateRotationRetry) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    rdkcertselector_h mock_cert_sel = reinterpret_cast<rdkcertselector_h>(0x22222);
    
    // First certificate attempt
    char* mock_cert_uri_1 = CreateMockCertUri("/opt/certs/cert1.p12");
    char* mock_password_1 = CreateMockPassword("pass1");
    
    // Second certificate attempt
    char* mock_cert_uri_2 = CreateMockCertUri("/opt/certs/cert2.p12");
    char* mock_password_2 = CreateMockPassword("pass2");
    
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillRepeatedly(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_deps, rdkcertselector_new(nullptr, nullptr, StrEq("MTLS")))
        .WillOnce(Return(mock_cert_sel));
    
    // First certificate attempt fails
    {
        InSequence seq;
        
        EXPECT_CALL(mock_deps, rdkcertselector_getCert(mock_cert_sel, _, _))
            .WillOnce(DoAll(
                SetArgPointee<1>(mock_cert_uri_1),
                SetArgPointee<2>(mock_password_1),
                Return(certselectorOk)
            ));
        EXPECT_CALL(mock_deps, rdkcertselector_getEngine(mock_cert_sel))
            .WillOnce(Return(static_cast<char*>(nullptr)));
        
        long mock_http_code = 500;
        EXPECT_CALL(mock_deps, performHttpMetadataPost(mock_curl, _, _, _))
            .WillOnce(DoAll(
                SetArgPointee<3>(mock_http_code),
                Return(-1)
            ));
        
        EXPECT_CALL(mock_deps, rdkcertselector_setCurlStatus(mock_cert_sel, -1, StrEq("https://example.com/upload")))
            .WillOnce(Return(TRY_ANOTHER));  // Retry with another certificate
        
        // Second certificate attempt succeeds
        EXPECT_CALL(mock_deps, rdkcertselector_getCert(mock_cert_sel, _, _))
            .WillOnce(DoAll(
                SetArgPointee<1>(mock_cert_uri_2),
                SetArgPointee<2>(mock_password_2),
                Return(certselectorOk)
            ));
        EXPECT_CALL(mock_deps, rdkcertselector_getEngine(mock_cert_sel))
            .WillOnce(Return(static_cast<char*>(nullptr)));
        
        long success_http_code = 201;
        EXPECT_CALL(mock_deps, performHttpMetadataPost(mock_curl, _, _, _))
            .WillOnce(DoAll(
                SetArgPointee<3>(success_http_code),
                Return(0)
            ));
        
        EXPECT_CALL(mock_deps, extractS3PresignedUrl(StrEq("/tmp/httpresult.txt"), _, _))
            .WillOnce(DoAll(
                testing::WithArg<1>([](char* s3_url) {
                    strcpy(s3_url, "https://s3.amazonaws.com/bucket/key?signature=retry123");
                }),
                Return(0)
            ));
        
        EXPECT_CALL(mock_deps, performS3PutUpload(StrEq("https://s3.amazonaws.com/bucket/key?signature=retry123"), 
                                                 StrEq("/path/to/file.txt"), _))
            .WillOnce(Return(0));
        
        EXPECT_CALL(mock_deps, rdkcertselector_setCurlStatus(mock_cert_sel, 0, StrEq("https://example.com/upload")))
            .WillOnce(Return(DONT_TRY_ANOTHER));
    }
    
    EXPECT_CALL(mock_deps, uploadutil_set_status(201, 0));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    EXPECT_CALL(mock_deps, rdkcertselector_free(&mock_cert_sel));
    
    EXPECT_EQ(0, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
    
    free(mock_cert_uri_1);
    free(mock_password_1);
    free(mock_cert_uri_2);
    free(mock_password_2);
}

#else  // !LIBRDKCERTSELECTOR

TEST_F(MtlsUploadTest, UploadFileWithTwoStageFlow_NoCertSelectorSupport) {
    void* mock_curl = reinterpret_cast<void*>(0x11111);
    
    EXPECT_CALL(mock_deps, uploadutil_get_md5())
        .WillOnce(Return(nullptr));
    EXPECT_CALL(mock_deps, doCurlInit())
        .WillOnce(Return(mock_curl));
    EXPECT_CALL(mock_deps, uploadutil_get_ocsp())
        .WillOnce(Return(false));
    EXPECT_CALL(mock_deps, doStopUpload(mock_curl));
    
    EXPECT_EQ(-1, uploadFileWithTwoStageFlow("https://example.com/upload", "/path/to/file.txt"));
}

#endif  // LIBRDKCERTSELECTOR

// Main function for running the tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}