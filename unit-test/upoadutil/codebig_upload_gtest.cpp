/*
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

extern "C" {
#include "codebig_upload.h"
#include "uploadUtil.h"
#include "upload_status.h"
#include <curl/curl.h>
}

// Undefine curl macros to allow mocking
#undef curl_easy_setopt

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::SetArrayArgument;
using ::testing::StrEq;
using ::testing::NotNull;
using ::testing::Invoke;
using ::testing::StrictMock;

// ==================== Mock Classes ====================

/**
 * @brief Mock class for CodeBig signing operations
 */
class MockCodeBigOperations {
public:
    MOCK_METHOD(int, doCodeBigSigning, 
                (int server_type, const char* SignInput, 
                 char *signurl, size_t signurlsize, 
                 char *outhheader, size_t outHeaderSize));
};

/**
 * @brief Mock class for upload utility operations
 */
class MockUploadUtil {
public:
    MOCK_METHOD(int, performHttpMetadataPost,
                (void *in_curl, FileUpload_t *pfile_upload,
                 MtlsAuth_t *auth, long *out_httpCode));
    MOCK_METHOD(int, performS3PutUpload,
                (const char *s3url, const char *localfile, MtlsAuth_t *auth));
};

/**
 * @brief Mock class for CURL operations
 */
class MockCurlOperations {
public:
    MOCK_METHOD(CURLcode, curl_easy_setopt, (CURL *curl, CURLoption option, void* param));
    MOCK_METHOD(const char*, curl_easy_strerror, (CURLcode errornum));
};

// Global mock pointers
static MockCodeBigOperations* g_mock_codebig = nullptr;
static MockUploadUtil* g_mock_upload_util = nullptr;
static MockCurlOperations* g_mock_curl = nullptr;
static bool g_ocsp_enabled = false;
static char g_stored_fqdn[256] = {0};

// ==================== Mock Implementations ====================

extern "C" {
    /**
     * @brief Mock implementation of doCodeBigSigning
     */
    int doCodeBigSigning(int server_type, const char* SignInput, 
                        char *signurl, size_t signurlsize, 
                        char *outhheader, size_t outHeaderSize) {
        if (g_mock_codebig) {
            return g_mock_codebig->doCodeBigSigning(server_type, SignInput,
                                                    signurl, signurlsize,
                                                    outhheader, outHeaderSize);
        }
        return -1;
    }

    /**
     * @brief Mock implementation of performHttpMetadataPost
     */
    int performHttpMetadataPost(void *in_curl,
                                FileUpload_t *pfile_upload,
                                MtlsAuth_t *auth,
                                long *out_httpCode) {
        if (g_mock_upload_util) {
            return g_mock_upload_util->performHttpMetadataPost(in_curl, pfile_upload,
                                                               auth, out_httpCode);
        }
        return -1;
    }

    /**
     * @brief Mock implementation of performS3PutUpload
     */
    int performS3PutUpload(const char *s3url, const char *localfile, MtlsAuth_t *auth) {
        if (g_mock_upload_util) {
            return g_mock_upload_util->performS3PutUpload(s3url, localfile, auth);
        }
        return -1;
    }

    /**
     * @brief Mock implementation of __uploadutil_get_ocsp
     */
    bool __uploadutil_get_ocsp(void) {
        return g_ocsp_enabled;
    }

    /**
     * @brief Mock implementation of __uploadutil_set_status
     */
    void __uploadutil_set_status(long http_code, int curl_code) {
        // Track status for verification if needed
    }

    /**
     * @brief Mock implementation of __uploadutil_set_fqdn
     */
    void __uploadutil_set_fqdn(const char *fqdn) {
        if (fqdn) {
            strncpy(g_stored_fqdn, fqdn, sizeof(g_stored_fqdn) - 1);
            g_stored_fqdn[sizeof(g_stored_fqdn) - 1] = '\0';
        }
    }

    /**
     * @brief Mock implementation of curl_easy_setopt (for OCSP testing)
     */
    CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...) {
        if (g_mock_curl) {
            va_list args;
            va_start(args, option);
            void* param = va_arg(args, void*);
            va_end(args);
            return g_mock_curl->curl_easy_setopt(curl, option, param);
        }
        return CURLE_OK;
    }

    /**
     * @brief Mock implementation of curl_easy_strerror
     */
    const char* curl_easy_strerror(CURLcode errornum) {
        if (g_mock_curl) {
            return g_mock_curl->curl_easy_strerror(errornum);
        }
        return "Mock error";
    }
}

// ==================== Test Fixture ====================

class CodeBigUploadTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mock_codebig = &mock_codebig;
        g_mock_upload_util = &mock_upload_util;
        g_mock_curl = &mock_curl;
        g_ocsp_enabled = false;
        memset(g_stored_fqdn, 0, sizeof(g_stored_fqdn));

        // Common test data
        test_server_type_ssr = HTTP_SSR_CODEBIG;
        test_server_type_xconf = HTTP_XCONF_CODEBIG;
        test_src_file = "/tmp/test_upload.log";
        test_codebig_url = "https://codebig.comcast.com/upload?token=xyz123";
        test_auth_header = "Authorization: Bearer abc123";
        test_s3_url = "https://s3.amazonaws.com/bucket/key?signature=xyz";
        
        mock_curl_handle = (CURL*)0x12345;
    }

    void TearDown() override {
        g_mock_codebig = nullptr;
        g_mock_upload_util = nullptr;
        g_mock_curl = nullptr;
        g_ocsp_enabled = false;
    }

    StrictMock<MockCodeBigOperations> mock_codebig;
    StrictMock<MockUploadUtil> mock_upload_util;
    StrictMock<MockCurlOperations> mock_curl;

    // Test data
    int test_server_type_ssr;
    int test_server_type_xconf;
    const char* test_src_file;
    const char* test_codebig_url;
    const char* test_auth_header;
    const char* test_s3_url;
    CURL* mock_curl_handle;
};

// ==================== doCodeBigSigningForUpload Tests ====================

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_Success_SSRCodeBig) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
                signurl[signurlsize - 1] = '\0';
                strncpy(outhheader, test_auth_header, outHeaderSize - 1);
                outhheader[outHeaderSize - 1] = '\0';
            }),
            Return(0)
        ));

    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(0, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_Success_XconfCodeBig) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_xconf, StrEq(test_src_file), 
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
                signurl[signurlsize - 1] = '\0';
                strncpy(outhheader, test_auth_header, outHeaderSize - 1);
                outhheader[outHeaderSize - 1] = '\0';
            }),
            Return(0)
        ));

    int result = doCodeBigSigningForUpload(test_server_type_xconf, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(0, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidParameters_NullSrcFile) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    int result = doCodeBigSigningForUpload(test_server_type_ssr, nullptr, 
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidParameters_NullSignUrl) {
    char auth_header[MAX_HEADER_LEN];

    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                           nullptr, MAX_CODEBIG_URL,
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidParameters_NullAuthHeader) {
    char signurl[MAX_CODEBIG_URL];

    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file, 
                                           signurl, sizeof(signurl),
                                           nullptr, MAX_HEADER_LEN);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidServerType_Direct) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    // HTTP_SSR_DIRECT is not a valid CodeBig server type
    int result = doCodeBigSigningForUpload(HTTP_SSR_DIRECT, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidServerType_XconfDirect) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    // HTTP_XCONF_DIRECT is not a valid CodeBig server type
    int result = doCodeBigSigningForUpload(HTTP_XCONF_DIRECT, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_InvalidServerType_Unknown) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    int result = doCodeBigSigningForUpload(HTTP_UNKNOWN, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_SigningFailure) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(Return(-1));

    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, doCodeBigSigningForUpload_SigningNonZeroError) {
    char signurl[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(test_src_file), 
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(Return(5)); // Non-zero error code

    int result = doCodeBigSigningForUpload(test_server_type_ssr, test_src_file,
                                           signurl, sizeof(signurl),
                                           auth_header, sizeof(auth_header));

    EXPECT_EQ(-1, result);
}

// ==================== performCodeBigMetadataPost Tests ====================

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_Success_NoExtraFields) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    // Setup CodeBig signing mock
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
                signurl[signurlsize - 1] = '\0';
                strncpy(outhheader, test_auth_header, outHeaderSize - 1);
                outhheader[outHeaderSize - 1] = '\0';
            }),
            Return(0)
        ));

    // Setup performHttpMetadataPost mock
    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(0, result);
    EXPECT_EQ(200L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_Success_WithExtraFields) {
    const char* filepath = "/tmp/test.log";
    const char* extra_fields = "md5=abc123&size=1024";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_xconf, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
                signurl[signurlsize - 1] = '\0';
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(201L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, extra_fields,
                                            test_server_type_xconf, &http_code_out);

    EXPECT_EQ(0, result);
    EXPECT_EQ(201L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_InvalidParameters_NullCurl) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    int result = performCodeBigMetadataPost(nullptr, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_InvalidParameters_NullFilepath) {
    long http_code_out = 0;

    int result = performCodeBigMetadataPost(mock_curl_handle, nullptr, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_InvalidParameters_NullHttpCodeOut) {
    const char* filepath = "/tmp/test.log";

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, nullptr);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_InvalidServerType) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    // Invalid server type - should fail before calling any mocks
    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            HTTP_SSR_DIRECT, &http_code_out);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_SigningFailure) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(Return(-1));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_HttpPostFailure_CurlError) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(0L),
            Return(CURLE_COULDNT_CONNECT)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(-1, result);
    EXPECT_EQ(0L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_HttpPostFailure_HttpError) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(404L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(-1, result);
    EXPECT_EQ(404L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_HttpPostFailure_ServerError) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(500L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(-1, result);
    EXPECT_EQ(500L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_SuccessWithOCSP) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;
    g_ocsp_enabled = true;

    // Expect OCSP setopt call
    EXPECT_CALL(mock_curl, curl_easy_setopt(mock_curl_handle, CURLOPT_SSL_VERIFYSTATUS, _))
        .WillOnce(Return(CURLE_OK));

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(0, result);
    EXPECT_EQ(200L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_OCSPSetoptFailure) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;
    g_ocsp_enabled = true;

    // Expect OCSP setopt call to fail (should still continue)
    EXPECT_CALL(mock_curl, curl_easy_setopt(mock_curl_handle, CURLOPT_SSL_VERIFYSTATUS, _))
        .WillOnce(Return(CURLE_UNSUPPORTED_PROTOCOL));
    EXPECT_CALL(mock_curl, curl_easy_strerror(CURLE_UNSUPPORTED_PROTOCOL))
        .WillOnce(Return("Unsupported protocol"));

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    // Should still succeed despite OCSP failure
    EXPECT_EQ(0, result);
    EXPECT_EQ(200L, http_code_out);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_FQDNExtraction_HTTPS) {
    const char* filepath = "/tmp/test.log";
    const char* codebig_url_https = "https://codebig.xfinity.com:443/upload?token=xyz";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([codebig_url_https](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, codebig_url_https, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(0, result);
    EXPECT_STREQ("codebig.xfinity.com", g_stored_fqdn);
}

TEST_F(CodeBigUploadTest, performCodeBigMetadataPost_FQDNExtraction_HTTP) {
    const char* filepath = "/tmp/test.log";
    const char* codebig_url_http = "http://test.example.com/path";
    long http_code_out = 0;

    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([codebig_url_http](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, codebig_url_http, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                            test_server_type_ssr, &http_code_out);

    EXPECT_EQ(0, result);
    EXPECT_STREQ("test.example.com", g_stored_fqdn);
}

// ==================== performCodeBigS3Put Tests ====================

TEST_F(CodeBigUploadTest, performCodeBigS3Put_Success) {
    EXPECT_CALL(mock_upload_util, performS3PutUpload(StrEq(test_s3_url),
                                                      StrEq(test_src_file),
                                                      nullptr))
        .WillOnce(Return(0));

    int result = performCodeBigS3Put(test_s3_url, test_src_file);

    EXPECT_EQ(0, result);
}

TEST_F(CodeBigUploadTest, performCodeBigS3Put_Failure) {
    EXPECT_CALL(mock_upload_util, performS3PutUpload(StrEq(test_s3_url),
                                                      StrEq(test_src_file),
                                                      nullptr))
        .WillOnce(Return(-1));

    int result = performCodeBigS3Put(test_s3_url, test_src_file);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigS3Put_InvalidParameters_NullS3Url) {
    int result = performCodeBigS3Put(nullptr, test_src_file);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigS3Put_InvalidParameters_NullSrcFile) {
    int result = performCodeBigS3Put(test_s3_url, nullptr);

    EXPECT_EQ(-1, result);
}

TEST_F(CodeBigUploadTest, performCodeBigS3Put_InvalidParameters_BothNull) {
    int result = performCodeBigS3Put(nullptr, nullptr);

    EXPECT_EQ(-1, result);
}

// ==================== Integration/Workflow Tests ====================

TEST_F(CodeBigUploadTest, TwoStageUpload_CompleteWorkflow) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    // Stage 1: Metadata POST
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result_stage1 = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                                    test_server_type_ssr, &http_code_out);

    EXPECT_EQ(0, result_stage1);
    EXPECT_EQ(200L, http_code_out);

    // Stage 2: S3 PUT
    EXPECT_CALL(mock_upload_util, performS3PutUpload(StrEq(test_s3_url),
                                                      StrEq(filepath),
                                                      nullptr))
        .WillOnce(Return(0));

    int result_stage2 = performCodeBigS3Put(test_s3_url, filepath);

    EXPECT_EQ(0, result_stage2);
}

TEST_F(CodeBigUploadTest, TwoStageUpload_Stage1Success_Stage2Failure) {
    const char* filepath = "/tmp/test.log";
    long http_code_out = 0;

    // Stage 1: Success
    EXPECT_CALL(mock_codebig, doCodeBigSigning(test_server_type_ssr, StrEq(filepath),
                                                NotNull(), MAX_CODEBIG_URL,
                                                NotNull(), MAX_HEADER_LEN))
        .WillOnce(DoAll(
            Invoke([this](int server_type, const char* SignInput, 
                         char *signurl, size_t signurlsize, 
                         char *outhheader, size_t outHeaderSize) {
                strncpy(signurl, test_codebig_url, signurlsize - 1);
            }),
            Return(0)
        ));

    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           nullptr, NotNull()))
        .WillOnce(DoAll(
            SetArgPointee<3>(200L),
            Return(0)
        ));

    int result_stage1 = performCodeBigMetadataPost(mock_curl_handle, filepath, nullptr,
                                                    test_server_type_ssr, &http_code_out);

    EXPECT_EQ(0, result_stage1);

    // Stage 2: Failure
    EXPECT_CALL(mock_upload_util, performS3PutUpload(StrEq(test_s3_url),
                                                      StrEq(filepath),
                                                      nullptr))
        .WillOnce(Return(-1));

    int result_stage2 = performCodeBigS3Put(test_s3_url, filepath);

    EXPECT_EQ(-1, result_stage2);
}

// ==================== Main ====================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

