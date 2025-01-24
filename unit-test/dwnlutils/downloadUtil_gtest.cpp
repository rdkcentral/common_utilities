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
#include "downloadUtil.h"
}
#include "mocks/curl_mock.h"
#include "mocks/mock_urlHelper.h"

pthread_once_t initOnce = PTHREAD_ONCE_INIT;

static void urlHelperInit(void) {
    curl_global_init(CURL_GLOBAL_ALL);
}

/* Creat or initialize curl request */
CURL *urlHelperCreateCurl(void) {
    pthread_once(&initOnce, urlHelperInit);
    return curl_easy_init();
}

#define GTEST_DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define GTEST_DEFAULT_RESULT_FILENAME "CommonUtils_downloadUtil_gtest_report.json"
#define GTEST_REPORT_FILEPATH_SIZE 256

using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::StrEq;

CurlWrapperMock *g_CurlWrapperMock = NULL;
urlHelperMock *g_urlHelperMock = NULL;

class downloadUtilTestFixture : public ::testing::Test {
        protected:

        CurlWrapperMock mockCurlWrapper;
	urlHelperMock mockurlHelper;

        downloadUtilTestFixture()
        {
            g_CurlWrapperMock = &mockCurlWrapper;
	    g_urlHelperMock = &mockurlHelper;
        }
        virtual ~downloadUtilTestFixture()
        {
            g_CurlWrapperMock = NULL;
            g_urlHelperMock = NULL;
        }

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

/*1. doCurlInit*/
TEST_F(downloadUtilTestFixture, doCurlInit_curl_created)
{
    //CURL *curl = (CURL *) malloc(sizeof(CURL));
    //EXPECT_CALL(*g_urlHelperMock, urlHelperCreateCurl()).WillOnce(Return(curl));   
    EXPECT_NE(doCurlInit(), nullptr);
    //free(curl);
}

/*2. doStopDownload*/
TEST_F(downloadUtilTestFixture, doStopDownload_curl_input)
{
    void *curl = doCurlInit();
    //CURL *curl = (CURL *) malloc(sizeof(CURL));
    EXPECT_CALL(*g_urlHelperMock, urlHelperDestroyCurl(_)).WillOnce(Return());   
    doStopDownload(curl);
    //free(curl);
}
TEST_F(downloadUtilTestFixture, doStopDownload_NULL_input)
{
    doStopDownload(NULL);
}

/*3. doInteruptDwnl*/
TEST_F(downloadUtilTestFixture, doInteruptDwnl_valid_inputs)
{
    void *curl = doCurlInit();
    //CURL *curl = (CURL *) malloc(sizeof(CURL));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_pause(_,_))
	    .Times(2)
	    .WillOnce(Return(CURLE_OK))
	    .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, setThrottleMode(_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_EQ(doInteruptDwnl(curl, 20000), CURLE_OK);
    //free(curl);
}
TEST_F(downloadUtilTestFixture, doInteruptDwnl_pause_fail)
{
    void *curl = doCurlInit();
    //CURL *curl = (CURL *) malloc(sizeof(CURL));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_pause(_,_))
            .Times(1)
            .WillOnce(Return(CURLE_AGAIN));
    EXPECT_EQ(doInteruptDwnl(curl, 20000), -1);
    //free(curl);
}
TEST_F(downloadUtilTestFixture, doInteruptDwnl_continue_fail)
{
    void *curl = doCurlInit();
    //CURL *curl = (CURL *) malloc(sizeof(CURL));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_pause(_,_))
            .Times(2)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_AGAIN));
    EXPECT_CALL(*g_urlHelperMock, setThrottleMode(_,_)).WillOnce(Return(CURLE_OK)); 
    EXPECT_NE(doInteruptDwnl(curl, 20000), CURLE_OK);
    //free(curl);
}
TEST_F(downloadUtilTestFixture, doInteruptDwnl_NULL_curl)
{
    EXPECT_EQ(doInteruptDwnl(NULL, 20000), CURLE_OK);
}
TEST_F(downloadUtilTestFixture, doInteruptDwnl_max_dwlSpeed_0)
{
    void *curl = doCurlInit();
    //CURL *curl = (CURL *) malloc(sizeof(CURL));
    EXPECT_EQ(doInteruptDwnl(curl, 0), CURLE_OK);
}

/*4. doGetDwnlBytes*/
TEST_F(downloadUtilTestFixture, doGetDwnlBytes_curl_NULL)
{
    EXPECT_EQ(doGetDwnlBytes(NULL), 0);
}
TEST_F(downloadUtilTestFixture, doGetDwnlBytes_curl)
{
    void *curl = doCurlInit();
    //CURL *curl = (CURL *) malloc(sizeof(CURL));

    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_)).WillOnce(Return(CURLE_OK)); 
    EXPECT_EQ(doGetDwnlBytes(curl), 0);
}

/*5. doCurlPutRequest*/
TEST_F(downloadUtilTestFixture, doCurlPutRequest)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxawer";
    char token_header[300];
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char url[128] = "http://127.0.0.1:9998/Service/Controller/Activate/org.rdk.FactoryProtect.1";
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", url);
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);
    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_)).WillOnce(Return(CURLE_OK)); 
    EXPECT_CALL(*g_urlHelperMock, urlHelperPutReuqest(_,_,_,_))
	    .WillOnce(Invoke([](CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status) {
	    *curl_ret_status = CURLE_OK;
	    return CURLE_OK;
            }));

    EXPECT_NE(doCurlPutRequest(Curl_req, &req_data, token_header, &httpCode), -1);
}

TEST_F(downloadUtilTestFixture, doCurlPutRequest_token_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char url[128] = "http://127.0.0.1:9998/Service/Controller/Activate/org.rdk.FactoryProtect.1";
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", url);

    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, urlHelperPutReuqest(_,_,_,_))
            .WillOnce(Invoke([](CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status) {   
            *curl_ret_status = CURLE_OK;
            return CURLE_OK;
            }));

    EXPECT_NE(doCurlPutRequest(Curl_req, &req_data, NULL, &httpCode), -1);
}

TEST_F(downloadUtilTestFixture, doCurlPutRequest_SetCommonCurlOpt_fail)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char url[128] = "http://127.0.0.1:9998/Service/Controller/Activate/org.rdk.FactoryProtect.1";
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", url);

    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_AGAIN));

    EXPECT_EQ(doCurlPutRequest(Curl_req, &req_data, NULL, &httpCode), -1);
}

TEST_F(downloadUtilTestFixture, doCurlPutRequest_curl_NULL)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxaweraaaaaaaaaaaaaaacc";
    char token_header[300];
    FileDwnl_t req_data;
    int httpCode = 0;
    char url[128] = "http://127.0.0.1:9998/Service/Controller/Activate/org.rdk.FactoryProtect.1";
    char header[64]  = "Content-Type: application/json";

    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", url);
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);

    EXPECT_EQ(doCurlPutRequest(NULL, &req_data, token_header, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doCurlPutRequest_httpcode_NULL)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxaweraaaaaaaaaaaaaaacmn";
    char token_header[300];
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char url[128] = "http://127.0.0.1:9998/Service/Controller/Activate/org.rdk.FactoryProtect.1";
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", url);
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);

    EXPECT_EQ(doCurlPutRequest(Curl_req, &req_data, token_header, NULL), -1);
}
TEST_F(downloadUtilTestFixture, doCurlPutRequest_file_dwnl_NULL)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxaweraaaaaaaaaa123456";
    char token_header[300];
    void *Curl_req = NULL;
    int httpCode = 0;

    Curl_req = doCurlInit();
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);

    EXPECT_EQ(doCurlPutRequest(Curl_req, NULL, token_header, &httpCode), -1);
}

/*6. getJsonRpcData*/
TEST_F(downloadUtilTestFixture, getJsonRpcData_curl_NULL)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxaweraaaaaaaaartyujkl";
    char token_header[300];
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);

    EXPECT_EQ(getJsonRpcData(NULL, &req_data, token_header, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, getJsonRpcData_req_data_NULL)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxaweraaaa";
    char token_header[300];
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);

    EXPECT_EQ(getJsonRpcData(Curl_req, NULL, token_header, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, getJsonRpcData_httpcode_NULL)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxawera1234567890";
    char token_header[300];
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);

    EXPECT_EQ(getJsonRpcData(Curl_req, &req_data, token_header, NULL), -1);
}

// Positive case
TEST_F(downloadUtilTestFixture, getJsonRpcData_download_succeeds)
{
    char token[256] = "abcdefghijklmnopqrstuvwxyz1234567890abcdefghijklmnopqrstuvwxyz123456ujklmnbvxaweraaaaaaaaaaaaaaac";
    char token_header[300];
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";
    DownloadData dData;

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = &dData;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");
    snprintf(token_header, sizeof(token_header), "Authorization: Bearer %s", token);
    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, urlHelperDownloadToMem(_,_,_,_))
            .WillOnce(Invoke([](CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status) {   
	    char pvOut[10] ="20";
	    pfile_dwnl->pDlData->pvOut = malloc(sizeof(pvOut));
	    *((char *)pfile_dwnl->pDlData->pvOut) = pvOut;
            *curl_ret_status = CURLE_OK;
            return 2000000;
            }));

    EXPECT_NE(getJsonRpcData(Curl_req, &req_data, token_header, &httpCode), -1);

    if (req_data.pDlData->pvOut){
    free (req_data.pDlData->pvOut);
    }
}

/*7. doHttpFileDownload*/
TEST_F(downloadUtilTestFixture, doHttpFileDownload_curl_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char range[16] = "99999999";
    char header[64]  = "Content-Type: application/json";
    MtlsAuth_t *sec = NULL;
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(doHttpFileDownload(NULL, &req_data, sec, 200000, range, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doHttpFileDownload_reqdata_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char range[16] = "99999999";
    char header[64]  = "Content-Type: application/json";
    MtlsAuth_t *sec = NULL;
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(doHttpFileDownload(Curl_req, NULL, sec, 200000, range, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doHttpFileDownload_httpcode_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char range[16] = "99999999";
    char header[64]  = "Content-Type: application/json";
    MtlsAuth_t *sec = NULL;
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(doHttpFileDownload(Curl_req, &req_data, sec, 200000, range, NULL), -1);
}
TEST_F(downloadUtilTestFixture, doHttpFileDownload_downloadToMem)
{
    FileDwnl_t req_data;
    hashParam_t hashData;
    void *Curl_req = NULL;
    int httpCode = 0;
    char range[16] = "99999999";
    char header[64]  = "Content-Type: application/json";
    MtlsAuth_t *sec = NULL;
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    DownloadData dData;

    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    hashData.hashvalue = "235";
    hashData.hashtime = "22";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = &dData;
    req_data.hashData = &hashData;
    req_data.pathname[0] = '\0';
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, setMtlsHeaders(_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, setThrottleMode(_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, urlHelperDownloadToMem(_,_,_,_))
            .WillOnce(Invoke([](CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status) {   
	    char pvOut[10] ="20";
	    pfile_dwnl->pDlData->pvOut = malloc(sizeof(pvOut));
	    *((char *)pfile_dwnl->pDlData->pvOut) = pvOut;
            *curl_ret_status = CURLE_OK;
            return 2000000;
            }));

    EXPECT_NE(doHttpFileDownload(Curl_req, &req_data, sec, 200000, range, &httpCode), -1);
}

TEST_F(downloadUtilTestFixture, doHttpFileDownload_downloadToFile)
{
    FileDwnl_t req_data;
    hashParam_t hashData;
    void *Curl_req = NULL;
    int httpCode = 0;
    char range[16] = "99999999";
    char header[64]  = "Content-Type: application/json";
    MtlsAuth_t *sec = NULL;
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    DownloadData dData;

    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    hashData.hashvalue = "235";
    hashData.hashtime = "22";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = &dData;
    req_data.hashData = &hashData;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, setMtlsHeaders(_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, setThrottleMode(_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, urlHelperDownloadFile(_,_,_,_,_,_))
            .WillOnce(Invoke([](CURL *curl, const char *file, char *dnl_start_pos, int chunk_dwnl_retry_time, int *httpCode_ret_status, CURLcode *curl_ret_status) {
            *curl_ret_status = CURLE_OK;
            return 2000000;
            }));

    EXPECT_NE(doHttpFileDownload(Curl_req, &req_data, sec, 200000, range, &httpCode), -1);
    free(sec);
}

/*8. doAuthHttpFileDownload*/
TEST_F(downloadUtilTestFixture, doAuthHttpFileDownload_SetRequestHeaders_fails)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, SetRequestHeaders(_,_,_)).WillOnce(Return(NULL));

    EXPECT_EQ(doAuthHttpFileDownload(Curl_req, &req_data, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doAuthHttpFileDownload_download_success)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";
    struct curl_slist *slist = curl_slist_append(slist, header); //Just appending a random string

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    req_data.pathname[0] = '\0';
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_CALL(*g_urlHelperMock, setCommonCurlOpt(_,_,_,_)).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_urlHelperMock, SetRequestHeaders(_,_,_)).WillOnce(Return(slist));
    EXPECT_CALL(*g_urlHelperMock, urlHelperDownloadToMem(_,_,_,_))
            .WillOnce(Invoke([](CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status) {
            *curl_ret_status = CURLE_OK;
            return 2000000;
            }));

    EXPECT_NE(doAuthHttpFileDownload(Curl_req, &req_data, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doAuthHttpFileDownload_curl_NUL)
{
    FileDwnl_t req_data;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(doAuthHttpFileDownload(NULL, &req_data, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doAuthHttpFileDownload_reqdata_NULL)
{
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();

    EXPECT_EQ(doAuthHttpFileDownload(Curl_req, NULL, &httpCode), -1);
}
TEST_F(downloadUtilTestFixture, doAuthHttpFileDownload_httpcode_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(doAuthHttpFileDownload(Curl_req, &req_data, NULL), -1);
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
