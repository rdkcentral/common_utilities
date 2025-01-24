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

#include <iostream>
#include "mock_urlHelper.h"

using namespace std;

extern urlHelperMock *g_urlHelperMock;

extern "C" void urlHelperDestroyCurl(CURL *ctx) 
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return;
    }
    printf("Inside Mock Function urlHelperDestroyCurl\n");

    return g_urlHelperMock->urlHelperDestroyCurl(ctx);
}

extern "C" CURLcode setCommonCurlOpt(CURL *curl, const char *url, char *pPostFields, bool sslverify) 
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return CURLE_OK;
    }
    printf("Inside Mock Function setCommonCurlOpt\n");

    return g_urlHelperMock->setCommonCurlOpt(curl, url, pPostFields, sslverify);
}

extern "C" CURLcode setThrottleMode(CURL *curl, curl_off_t max_dwnl_speed) 
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return NULL;
    }
    printf("Inside Mock Function setThrottleMode\n");

    return g_urlHelperMock->setThrottleMode(curl, max_dwnl_speed);
}

extern "C" int urlHelperPutReuqest(CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status)  
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return 0;
    }
    printf("Inside Mock Function urlHelperPutReuqest\n");

    return g_urlHelperMock->urlHelperPutReuqest(curl, upData, httpCode_ret_status, curl_ret_status);
}

extern "C" size_t urlHelperDownloadToMem( CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status )
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return 0;
    }
    printf("Inside Mock Function urlHelperDownloadToMem\n");

    return g_urlHelperMock->urlHelperDownloadToMem(curl, pfile_dwnl, httpCode_ret_status, curl_ret_status);
}

extern "C" size_t urlHelperDownloadFile(CURL *curl, const char *file, char *dnl_start_pos, int chunk_dwnl_retry_time, int *httpCode_ret_status, CURLcode *curl_ret_status)
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return 0;
    }
    printf("Inside Mock Function urlHelperDownloadFile\n");

    return g_urlHelperMock->urlHelperDownloadFile( curl, file, dnl_start_pos, chunk_dwnl_retry_time, httpCode_ret_status, curl_ret_status);
}

extern "C" CURLcode setMtlsHeaders(CURL *curl, MtlsAuth_t *sec)
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return NULL;
    }
    printf("Inside Mock Function setMtlsHeaders\n");

    return g_urlHelperMock->setMtlsHeaders(curl, sec);
}

extern "C" struct curl_slist* SetRequestHeaders( CURL *curl, struct curl_slist *pslist, char *pHeader )
{
    if (!g_urlHelperMock)
    {
        cout << "g_urlHelperMock object is NULL" << endl;
        return NULL;
    }
    printf("Inside Mock Function SetRequestHeaders\n");

    return g_urlHelperMock->SetRequestHeaders(curl, pslist, pHeader);
}

