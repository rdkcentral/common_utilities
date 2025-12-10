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
 * @file upload_status.c
 * @brief Enhanced upload status implementation
 */

#include "upload_status.h"
#include "mtls_upload.h"
#include "codebig_upload.h"
#include "uploadUtil.h"
#include "downloadUtil.h"
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

/* Thread-local status tracking for capturing real codes from underlying functions */
static __thread long g_last_http_code = 0;
static __thread int g_last_curl_code = CURLE_OK;
static __thread char g_last_fqdn[256] = {0};
static __thread bool g_ocsp_enabled = false;
static __thread char g_md5_base64[64] = {0};

void __uploadutil_set_status(long http_code, int curl_code)
{
    g_last_http_code = http_code;
    g_last_curl_code = curl_code;
}

void __uploadutil_set_fqdn(const char *fqdn)
{
    if (fqdn) {
        strncpy(g_last_fqdn, fqdn, sizeof(g_last_fqdn) - 1);
        g_last_fqdn[sizeof(g_last_fqdn) - 1] = '\0';
    } else {
        g_last_fqdn[0] = '\0';
    }
}

void __uploadutil_get_fqdn(char *fqdn, size_t size)
{
    if (fqdn && size > 0) {
        strncpy(fqdn, g_last_fqdn, size - 1);
        fqdn[size - 1] = '\0';
    }
    /* Reset after reading */
    g_last_fqdn[0] = '\0';
}

void __uploadutil_set_ocsp(bool enabled)
{
    g_ocsp_enabled = enabled;
}

bool __uploadutil_get_ocsp(void)
{
    return g_ocsp_enabled;
}

void __uploadutil_set_md5(const char *md5)
{
    if (md5) {
        strncpy(g_md5_base64, md5, sizeof(g_md5_base64) - 1);
        g_md5_base64[sizeof(g_md5_base64) - 1] = '\0';
    } else {
        g_md5_base64[0] = '\0';
    }
}

const char* __uploadutil_get_md5(void)
{
    return g_md5_base64[0] != '\0' ? g_md5_base64 : NULL;
}

void __uploadutil_get_status(long *http_code, int *curl_code)
{
    if (http_code) *http_code = g_last_http_code;
    if (curl_code) *curl_code = g_last_curl_code;
    /* Reset after reading */
    g_last_http_code = 0;
    g_last_curl_code = CURLE_OK;
}

int performS3PutUploadEx(const char *upload_url, const char *src_file, 
                        MtlsAuth_t *auth, const char *md5_base64, bool ocsp_enabled, UploadStatusDetail* status)
{
    if (!status) {
        return -1; // Can't report status
    }
    
    memset(status, 0, sizeof(UploadStatusDetail));
    status->result_code = -1;
    status->curl_code = CURLE_FAILED_INIT;
    
    if (!upload_url || !src_file) {
        snprintf(status->error_message, sizeof(status->error_message), 
                "Invalid parameters: upload_url=%p, src_file=%p", upload_url, src_file);
        return -1;
    }
    
    /* Extract FQDN from upload URL (simple extraction) */
    status->fqdn[0] = '\0';
    const char *hostname_start = strstr(upload_url, "://");
    if (hostname_start) {
        hostname_start += 3;
        const char *hostname_end = hostname_start;
        while (*hostname_end && *hostname_end != ':' && *hostname_end != '/' && *hostname_end != '?') {
            hostname_end++;
        }
        size_t len = hostname_end - hostname_start;
        if (len > 0 && len < sizeof(status->fqdn)) {
            strncpy(status->fqdn, hostname_start, len);
            status->fqdn[len] = '\0';
        }
    }
    
    /* Set MD5 and OCSP flags for underlying functions */
    __uploadutil_set_md5(md5_base64);
    __uploadutil_set_ocsp(ocsp_enabled);
    
    /* Call underlying function - it will set thread-local status */
    int result = performS3PutUpload(upload_url, src_file, auth);
    
    /* Capture real status codes from thread-local storage */
    long http_code = 0;
    int curl_code = CURLE_OK;
    __uploadutil_get_status(&http_code, &curl_code);
    
    /* Capture FQDN if underlying function set it (may override URL-based extraction) */
    char thread_fqdn[256];
    __uploadutil_get_fqdn(thread_fqdn, sizeof(thread_fqdn));
    if (thread_fqdn[0] != '\0') {
        strncpy(status->fqdn, thread_fqdn, sizeof(status->fqdn) - 1);
        status->fqdn[sizeof(status->fqdn) - 1] = '\0';
    }
    
    status->result_code = result;
    status->http_code = http_code;
    status->curl_code = curl_code;
    
    if (result == 0) {
        status->upload_completed = true;
        status->auth_success = true;
        snprintf(status->error_message, sizeof(status->error_message), 
                "S3 upload successful (HTTP %ld)", http_code);
    } else {
        status->upload_completed = false;
        
        /* Determine failure type from HTTP/CURL codes */
        if (curl_code != CURLE_OK) {
            status->auth_success = false;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "S3 CURL error: %d", curl_code);
        } else if (http_code == 403) {
            status->auth_success = false;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "S3 access denied (HTTP %ld)", http_code);
        } else if (http_code == 404) {
            status->auth_success = true;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "S3 bucket/key not found (HTTP %ld)", http_code);
        } else if (http_code >= 400) {
            status->auth_success = true;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "S3 HTTP error %ld", http_code);
        } else {
            snprintf(status->error_message, sizeof(status->error_message), 
                    "S3 upload failed");
        }
    }
    
    return result;
}