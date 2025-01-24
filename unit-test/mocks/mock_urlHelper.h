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

#ifndef MOCK_URLHELPER
#define MOCK_URLHELPER
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <curl/curl.h>
#include "rdkv_cdl_log_wrapper.h"
#define RDK_API_SUCCESS 0

#undef urlHelperDestroyCurl
#undef setCommonCurlOpt
#undef setThrottleMode
#undef urlHelperPutReuqest
#undef urlHelperDownloadToMem
#undef urlHelperDownloadFile
#undef setMtlsHeaders 
#undef SetRequestHeaders

/*############################################################*/
#ifndef _RDK_DWNLUTIL_H_
#define DWNL_PATH_FILE_LEN 128
#define BIG_BUF_LEN 1024

typedef struct credential {
        char cert_name[64];
        char cert_type[16];
        char key_pas[32];
}MtlsAuth_t;

/* Below structure use for download file data */
typedef struct CommonDownloadData {
    void* pvOut;
    size_t datasize;        // data size
    size_t memsize;         // allocated memory size (if applicable)
} DownloadData;

/* Structure Use for Hash Value and Time*/
typedef struct hashParam {
    char *hashvalue;
    char *hashtime;
}hashParam_t;

typedef struct filedwnl {
        char *pPostFields;
        char *pHeaderData;
        DownloadData *pDlData;
        DownloadData *pDlHeaderData;
        int chunk_dwnl_retry_time;
        char url[BIG_BUF_LEN];
        char pathname[DWNL_PATH_FILE_LEN];
        bool sslverify;
        hashParam_t *hashData;
}FileDwnl_t;
#endif


class urlHelperInterface {
public:
    virtual ~urlHelperInterface() {}
    virtual void urlHelperDestroyCurl(CURL *ctx) = 0;
    virtual CURLcode setCommonCurlOpt(CURL *curl, const char *url, char *pPostFields, bool sslverify) = 0;
    virtual CURLcode setThrottleMode(CURL *curl, curl_off_t max_dwnl_speed) = 0;
    virtual int urlHelperPutReuqest(CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status) = 0;
    virtual size_t urlHelperDownloadToMem( CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status ) = 0;
    virtual size_t urlHelperDownloadFile(CURL *curl, const char *file, char *dnl_start_pos, int chunk_dwnl_retry_time, int *httpCode_ret_status, CURLcode *curl_ret_status) = 0;
    virtual CURLcode setMtlsHeaders(CURL *curl, MtlsAuth_t *sec) = 0;
    virtual struct curl_slist* SetRequestHeaders( CURL *curl, struct curl_slist *pslist, char *pHeader ) = 0;
};

class urlHelperMock : public urlHelperInterface {
public:
    virtual ~urlHelperMock() {}
    MOCK_METHOD1(urlHelperDestroyCurl, void (CURL *ctx));
    MOCK_METHOD4(setCommonCurlOpt, CURLcode (CURL *curl, const char *url, char *pPostFields, bool sslverify));
    MOCK_METHOD2(setThrottleMode, CURLcode (CURL *curl, curl_off_t max_dwnl_speed));
    MOCK_METHOD4(urlHelperPutReuqest, int (CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status));
    MOCK_METHOD6(urlHelperDownloadFile, size_t (CURL *curl, const char *file, char *dnl_start_pos, int chunk_dwnl_retry_time, int *httpCode_ret_status, CURLcode *curl_ret_status));
    MOCK_METHOD4(urlHelperDownloadToMem, size_t ( CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status ));
    MOCK_METHOD2(setMtlsHeaders, CURLcode (CURL *curl, MtlsAuth_t *sec));
    MOCK_METHOD3(SetRequestHeaders, struct curl_slist* ( CURL *curl, struct curl_slist *pslist, char *pHeader ));
};
#endif
