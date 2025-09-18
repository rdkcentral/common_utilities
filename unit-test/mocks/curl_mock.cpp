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
#include "curl_mock.h"

using namespace std;

extern CurlWrapperMock *g_CurlWrapperMock;

extern "C" CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...)
{
    if (!g_CurlWrapperMock)
    {
        cout << "g_CurlWrapperMock object is NULL" << endl;
        return CURLE_OK;
    }
    printf("Inside Mock Function curl_easy_setopt\n");
    return g_CurlWrapperMock->curl_easy_setopt(curl, option, NULL);
}

extern "C" CURLcode curl_easy_perform(CURL *curl)
{
    if (!g_CurlWrapperMock)
    {
	cout << "g_CurlWrapperMock object is NULL" << endl;
        return CURLE_OK;
    }
    printf("Inside Mock Function curl_easy_perform\n");

    return g_CurlWrapperMock->curl_easy_perform(curl);
}

extern "C" CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, ...)
{
    if (!g_CurlWrapperMock)
    {
        cout << "g_CurlWrapperMock object is NULL" << endl;
        return CURLE_OK;
    }
    va_list args;
    va_start(args, info);
    // Forward the argument as void*
    void* param = va_arg(args, void*);
    va_end(args);
    printf("Inside Mock Function curl_easy_getinfo\n");
    return g_CurlWrapperMock->curl_easy_getinfo(curl, info, NULL);
}
extern "C" const char* curl_easy_strerror(CURLcode errornum)
{
    if (!g_CurlWrapperMock)
    {
        cout << "g_CurlWrapperMock object is NULL" << endl;
        return "g_CurlWrapperMock object is NULL";
    }
    printf("Inside Mock Function curl_easy_strerror\n");

    return g_CurlWrapperMock->curl_easy_strerror(errornum);
}

extern "C" CURLcode curl_easy_pause(CURL *handle, int bitmask)
{
    if (!g_CurlWrapperMock)
    {
        cout << "g_CurlWrapperMock object is NULL" << endl;
        return CURLE_OK;
    }
    printf("Inside Mock Function curl_easy_pause\n");

    return g_CurlWrapperMock->curl_easy_pause(handle, bitmask );
}
