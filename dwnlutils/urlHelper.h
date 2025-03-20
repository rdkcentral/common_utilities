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

#ifndef  _RDK_DWNLUTIL_H_
#define  _RDK_DWNLUTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <ctype.h>
#include <curl/curl.h>
#include <ctype.h>

#define CURL_TLS_TIMEOUT 7200L
#define CURL_PROGRESS_FILE "/opt/curl_progress"

#define MAX_BUFF_SIZE 512
#define MAX_BUFF_SIZE1 256
#define MIN_BUFF_SIZE 64
#define MIN_BUFF_SIZE1 32
#define SMALL_SIZE_BUFF 8
#define URL_MAX_LEN 512
#define DWNL_PATH_FILE_LEN 128
#define BIG_BUF_LEN 1024

//TODO
typedef struct credential {
        char cert_name[64];
        char cert_type[16];
        char key_pas[65];
#ifdef LIBRDKCERTSELECTOR
        char engine[32];
#endif
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

#ifdef CURL_DEBUG
typedef struct debugdata {
        char trace_ascii; /* 1 or 0 */
        FILE *verboslog;
} DbgData_t;

int my_trace(CURL *handle, curl_infotype type, char *data, size_t size, void *userp);
CURLcode setCurlDebugOpt(CURL *curl, DbgData_t *debug);
#endif

/*Below structure use for store curl progress */
struct curlprogress {
    FILE *prog_store;
    curl_off_t lastruntime; /* type depends on version, see above */
    CURL *curl;
};

/*#define SWUPDATELOG(level, ...) do { \
                        RDK_LOG(level, "LOG.RDK.FWUPG", __VA_ARGS__); \
                        } while (0);*/


/* urlHelperGetHeaderInfo(): Used for get curl request header data
 * url: Request server url
 * httpCode: Use for return http status to called function.
 * sec: This is structure which is used for receive certificate, key, password etc
 * pathname : Header data Save file name with path
 * curl_ret_status : Use for return curl status to called function.
 * */
int urlHelperGetHeaderInfo(const char* url,
                             MtlsAuth_t *sec,
                             const char* pathname,
                             int *httpCode_ret_status,
                             int *curl_ret_status);

/* urlHelperDownloadFile(): Use for download a file
 * curl : Curl Object
 * file : path with file name to download image
 * dnl_start_pos : Use for chunk Download if it is NULL in that case request is Full Downlaod
 * httpCode_ret_status : Send back http status.
 * curl_ret_status : Send back curl status
 * Return Type size_t : Return no of bytes downloaded.
 * */
size_t urlHelperDownloadFile(CURL *curl, const char *file, char *dnl_start_pos, int chunk_dwnl_retry_time, int* httpCode_ret_status, CURLcode *curl_ret_status);
size_t urlHelperDownloadToMem( CURL *curl, FileDwnl_t *pFileData, int *httpCode_ret_status, CURLcode *curl_ret_status );

CURL *urlHelperCreateCurl(void);
void urlHelperDestroyCurl(CURL *ctx);
CURLcode setMtlsHeaders(CURL *curl, MtlsAuth_t *sec);
CURLcode setCommonCurlOpt(CURL *curl, const char *url, char *pPostFields, bool sslverify);
CURLcode setCurlProgress(CURL *curl, struct curlprogress *curl_progress);
CURLcode setThrottleMode(CURL *curl, curl_off_t max_dwnl_speed);
char *printCurlError(int curl_ret_code);
struct curl_slist* SetRequestHeaders(CURL *curl, struct curl_slist *pslist, char *pHeader);
CURLcode SetPostFields(CURL *curl, char *pPostFields);
int urlHelperPutReuqest(CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status);
int allocDowndLoadDataMem( DownloadData *pDwnData, int szDataSize );
bool checkDeviceInternetConnection(long);
size_t writeFunction(void *contents, size_t size, size_t nmemb, void *userp);
void closeFile(DownloadData *data, struct curlprogress *prog, FILE *fp);
#if CURL_DEBUG
CURLcode setCurlDebugOpt(CURL *curl, DbgData_t *debug);
#endif
long long current_time();

#endif
