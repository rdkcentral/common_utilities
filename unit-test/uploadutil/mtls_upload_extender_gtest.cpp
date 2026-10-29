/**
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

/**
 * @file mtls_upload_extender_gtest.cpp
 * L1 for DEVICE_EXTENDER P12 + rdkconfig_get path in mtls_upload.c
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

extern "C" {
#include "mtls_upload.h"
#include "uploadUtil.h"
#include "rdkconfig.h"
#include <curl/curl.h>
}

#undef curl_easy_setopt

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgPointee;

class MockUploadUtil {
public:
    MOCK_METHOD(int, performHttpMetadataPost,
                (void *in_curl, FileUpload_t *pfile_upload,
                 MtlsAuth_t *auth, long *out_httpCode));
    MOCK_METHOD(void*, doCurlInit, ());
    MOCK_METHOD(void, doStopUpload, (void* curl));
};

class MockCurlOperations {
public:
    MOCK_METHOD(CURLcode, curl_easy_setopt, (CURL *curl, CURLoption option, void* param));
    MOCK_METHOD(const char*, curl_easy_strerror, (CURLcode errornum));
};

static MockUploadUtil* g_mock_upload_util = nullptr;
static MockCurlOperations* g_mock_curl = nullptr;
static bool g_ocsp_enabled = false;
static int g_access_dynamic = 0;
static int g_access_static = 0;
static int g_rdkconfig_fail = 0;
static const char *g_last_rdkconfig_ref = nullptr;

extern "C" {

int mock_extender_access(const char *pathname, int mode)
{
    (void)mode;
    if (!pathname)
    {
        return -1;
    }
    if (strstr(pathname, "devicecert_1.pk12") != NULL)
    {
        return g_access_dynamic ? 0 : -1;
    }
    if (strstr(pathname, "opensync/certs/cert.p12") != NULL)
    {
        return g_access_static ? 0 : -1;
    }
    return -1;
}

int rdkconfig_get(uint8_t **sbuff, size_t *sbuffsz, const char *refname)
{
    g_last_rdkconfig_ref = refname;
    if (g_rdkconfig_fail || !sbuff || !sbuffsz)
    {
        return RDKCONFIG_FAIL;
    }
    const char *phrase = "test-phrase\n";
    size_t n = strlen(phrase);
    uint8_t *buf = (uint8_t *)malloc(n);
    if (!buf)
    {
        return RDKCONFIG_FAIL;
    }
    memcpy(buf, phrase, n);
    *sbuff = buf;
    *sbuffsz = n;
    return RDKCONFIG_OK;
}

int rdkconfig_free(uint8_t **sbuff, size_t sbuffsz)
{
    (void)sbuffsz;
    if (sbuff && *sbuff)
    {
        free(*sbuff);
        *sbuff = NULL;
    }
    return RDKCONFIG_OK;
}

#ifdef LIBRDKCERTSELECTOR
rdkcertselector_h rdkcertselector_new(const char *file_path, const char *file_content,
                                      const char *service_name)
{
    (void)file_path;
    (void)file_content;
    (void)service_name;
    return NULL;
}

rdkcertselectorStatus_t rdkcertselector_getCert(rdkcertselector_h selector,
                                                char **cert_uri, char **cert_pass)
{
    (void)selector;
    (void)cert_uri;
    (void)cert_pass;
    return static_cast<rdkcertselectorStatus_t>(-1);
}

char *rdkcertselector_getEngine(rdkcertselector_h selector)
{
    (void)selector;
    return NULL;
}

rdkcertselectorRetry_t rdkcertselector_setCurlStatus(rdkcertselector_h selector,
                                                     unsigned int curl_status, const char *url)
{
    (void)selector;
    (void)curl_status;
    (void)url;
    return static_cast<rdkcertselectorRetry_t>(0);
}

void rdkcertselector_free(rdkcertselector_h *selector)
{
    (void)selector;
}
#endif

int performHttpMetadataPost(void *in_curl, FileUpload_t *pfile_upload,
                            MtlsAuth_t *auth, long *out_httpCode)
{
    if (g_mock_upload_util)
    {
        return g_mock_upload_util->performHttpMetadataPost(in_curl, pfile_upload,
                                                           auth, out_httpCode);
    }
    return -1;
}

int performS3PutUpload(const char *s3url, const char *localfile, MtlsAuth_t *auth)
{
    (void)s3url;
    (void)localfile;
    (void)auth;
    return -1;
}

void *doCurlInit(void)
{
    if (g_mock_upload_util)
    {
        return g_mock_upload_util->doCurlInit();
    }
    return (void *)0x12345;
}

void doStopUpload(void *curl)
{
    if (g_mock_upload_util)
    {
        g_mock_upload_util->doStopUpload(curl);
    }
}

bool __uploadutil_get_ocsp(void)
{
    return g_ocsp_enabled;
}

const char *__uploadutil_get_md5(void)
{
    return NULL;
}

void __uploadutil_set_status(long http_code, int curl_code)
{
    (void)http_code;
    (void)curl_code;
}

CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...)
{
    if (g_mock_curl)
    {
        va_list args;
        va_start(args, option);
        void *param = va_arg(args, void *);
        va_end(args);
        return g_mock_curl->curl_easy_setopt(curl, option, param);
    }
    return CURLE_OK;
}

const char *curl_easy_strerror(CURLcode errornum)
{
    if (g_mock_curl)
    {
        return g_mock_curl->curl_easy_strerror(errornum);
    }
    return "Mock error";
}

}

class MtlsUploadExtenderTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        g_mock_upload_util = &mock_upload_util;
        g_mock_curl = &mock_curl;
        g_ocsp_enabled = false;
        g_access_dynamic = 1;
        g_access_static = 0;
        g_rdkconfig_fail = 0;
        g_last_rdkconfig_ref = nullptr;
        memset(&test_sec_out, 0, sizeof(test_sec_out));
        mock_curl_handle = (void *)0x12345;
    }

    void TearDown() override
    {
        g_mock_upload_util = nullptr;
        g_mock_curl = nullptr;
    }

    NiceMock<MockUploadUtil> mock_upload_util;
    NiceMock<MockCurlOperations> mock_curl;
    MtlsAuth_t test_sec_out;
    void *mock_curl_handle;
};

TEST_F(MtlsUploadExtenderTest, NullUrl_ReturnsFail)
{
    long http_code = 0;
    EXPECT_EQ(-1, performMetadataPostWithCertRotationEx(nullptr, "/tmp/out",
                                                        NULL, &test_sec_out, &http_code));
}

TEST_F(MtlsUploadExtenderTest, NoCerts_ReturnsFail)
{
    long http_code = 0;
    g_access_dynamic = 0;
    g_access_static = 0;
    EXPECT_EQ(-1, performMetadataPostWithCertRotationEx("https://ssr.example/sign",
                                                        "/tmp/signed_url", NULL,
                                                        &test_sec_out, &http_code));
}

TEST_F(MtlsUploadExtenderTest, RdkconfigFail_ReturnsFail)
{
    long http_code = 0;
    g_rdkconfig_fail = 1;
    EXPECT_EQ(-1, performMetadataPostWithCertRotationEx("https://ssr.example/sign",
                                                        "/tmp/signed_url", NULL,
                                                        &test_sec_out, &http_code));
}

TEST_F(MtlsUploadExtenderTest, DynamicCert_PostSuccess)
{
    long http_code = 0;
    g_access_dynamic = 1;

    EXPECT_CALL(mock_upload_util, doCurlInit())
        .WillOnce(Return(mock_curl_handle));
    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           NotNull(), NotNull()))
        .WillOnce(DoAll(SetArgPointee<3>(200L), Return(0)));
    EXPECT_CALL(mock_upload_util, doStopUpload(mock_curl_handle))
        .Times(1);

    int result = performMetadataPostWithCertRotationEx("https://ssr.example/sign",
                                                       "/tmp/signed_url", "type=minidump",
                                                       &test_sec_out, &http_code);
    EXPECT_EQ(0, result);
    EXPECT_EQ(200L, http_code);
    EXPECT_STREQ(test_sec_out.cert_type, "P12");
    EXPECT_TRUE(strstr(test_sec_out.cert_name, "devicecert_1.pk12") != nullptr);
    EXPECT_STREQ(test_sec_out.key_pas, "test-phrase");
    ASSERT_NE(g_last_rdkconfig_ref, nullptr);
    EXPECT_STREQ(g_last_rdkconfig_ref, "/tmp/.cfgDynamicxpki");
}

TEST_F(MtlsUploadExtenderTest, StaticCert_WhenDynamicMissing)
{
    long http_code = 0;
    g_access_dynamic = 0;
    g_access_static = 1;

    EXPECT_CALL(mock_upload_util, doCurlInit())
        .WillOnce(Return(mock_curl_handle));
    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(mock_curl_handle, NotNull(),
                                                           NotNull(), NotNull()))
        .WillOnce(DoAll(SetArgPointee<3>(200L), Return(0)));
    EXPECT_CALL(mock_upload_util, doStopUpload(mock_curl_handle))
        .Times(1);

    int result = performMetadataPostWithCertRotationEx("https://ssr.example/sign",
                                                       "/tmp/signed_url", NULL,
                                                       &test_sec_out, &http_code);
    EXPECT_EQ(0, result);
    EXPECT_TRUE(strstr(test_sec_out.cert_name, "opensync/certs/cert.p12") != nullptr);
    ASSERT_NE(g_last_rdkconfig_ref, nullptr);
    EXPECT_STREQ(g_last_rdkconfig_ref, "/tmp/.cfgStaticxpki");
}

TEST_F(MtlsUploadExtenderTest, CurlInitFail_ReturnsFail)
{
    long http_code = 0;
    EXPECT_CALL(mock_upload_util, doCurlInit())
        .WillOnce(Return(nullptr));

    EXPECT_EQ(-1, performMetadataPostWithCertRotationEx("https://ssr.example/sign",
                                                        "/tmp/signed_url", NULL,
                                                        &test_sec_out, &http_code));
}

TEST_F(MtlsUploadExtenderTest, HttpFail_ReturnsFail)
{
    long http_code = 0;
    EXPECT_CALL(mock_upload_util, doCurlInit())
        .WillOnce(Return(mock_curl_handle));
    EXPECT_CALL(mock_upload_util, performHttpMetadataPost(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<3>(403L), Return(-1)));
    EXPECT_CALL(mock_upload_util, doStopUpload(mock_curl_handle))
        .Times(1);

    EXPECT_EQ(-1, performMetadataPostWithCertRotationEx("https://ssr.example/sign",
                                                        "/tmp/signed_url", NULL,
                                                        &test_sec_out, &http_code));
    EXPECT_EQ(403L, http_code);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
