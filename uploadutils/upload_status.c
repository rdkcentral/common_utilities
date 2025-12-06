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

/**
 * @brief Extract FQDN (hostname) from URL
 * @param url Full URL (e.g., "https://example.com:443/path?query")
 * @param fqdn_out Buffer to store FQDN (e.g., "example.com")
 * @param fqdn_size Size of fqdn_out buffer
 */
static void extract_fqdn(const char *url, char *fqdn_out, size_t fqdn_size)
{
    if (!url || !fqdn_out || fqdn_size == 0) return;
    
    fqdn_out[0] = '\0';
    
    // Find start of hostname (after "://")
    const char *hostname_start = strstr(url, "://");
    if (!hostname_start) {
        hostname_start = url; // No protocol, assume raw hostname
    } else {
        hostname_start += 3; // Skip "://"
    }
    
    // Find end of hostname (before ':' port, '/' path, or '?' query)
    const char *hostname_end = hostname_start;
    while (*hostname_end && *hostname_end != ':' && *hostname_end != '/' && *hostname_end != '?') {
        hostname_end++;
    }
    
    // Copy hostname to output buffer
    size_t hostname_len = hostname_end - hostname_start;
    if (hostname_len > 0 && hostname_len < fqdn_size) {
        strncpy(fqdn_out, hostname_start, hostname_len);
        fqdn_out[hostname_len] = '\0';
    }
}

void init_upload_result(UploadResult_t* result)
{
    if (!result) return;
    
    memset(result, 0, sizeof(UploadResult_t));
    result->result_code = -1;
    result->codes.curl_code = CURLE_FAILED_INIT;
    // Other fields already zero'd by memset
}

void init_upload_status(UploadStatusDetail* status)
{
    if (!status) return;
    
    memset(status, 0, sizeof(UploadStatusDetail));
    status->result_code = -1;
    status->curl_code = CURLE_FAILED_INIT;
    // error_message, http_code, upload_completed, auth_success, fqdn already zero'd by memset
}

int performHttpMetadataPostEx(const char *upload_url, const char *filepath,
                               const char *extra_fields, MtlsAuth_t *auth, 
                               bool ocsp_enabled, UploadStatusDetail* status)
{
    if (!status) {
        return -1;
    }
    
    init_upload_status(status);
    
    if (!upload_url || !filepath) {
        snprintf(status->error_message, sizeof(status->error_message), 
                "Invalid parameters: url=%p, file=%p", upload_url, filepath);
        return -1;
    }
    
    /* Initialize curl */
    void *curl = doCurlInit();
    if (!curl) {
        snprintf(status->error_message, sizeof(status->error_message), 
                "Failed to initialize curl");
        status->curl_code = CURLE_FAILED_INIT;
        return -1;
    }
    
    /* Extract FQDN from upload URL */
    extract_fqdn(upload_url, status->fqdn, sizeof(status->fqdn));
    
    /* Prepare FileUpload_t structure */
    FileUpload_t file_upload;
    memset(&file_upload, 0, sizeof(FileUpload_t));
    
    char url_buf[512];
    char path_buf[256];
    
    strncpy(url_buf, upload_url, sizeof(url_buf) - 1);
    strncpy(path_buf, filepath, sizeof(path_buf) - 1);
    
    file_upload.url = url_buf;
    file_upload.pathname = path_buf;
    file_upload.sslverify = 1;
    file_upload.hashData = NULL;
    file_upload.pPostFields = (char*)extra_fields;  // Cast away const (API limitation)
    
    /* Apply OCSP if enabled */
    if (ocsp_enabled) {
        CURLcode ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
        if (ret != CURLE_OK) {
            snprintf(status->error_message, sizeof(status->error_message), 
                    "CURLOPT_SSL_VERIFYSTATUS failed: %s", curl_easy_strerror(ret));
        }
    }
    
    /* Perform metadata POST */
    long http_code = 0;
    int curl_ret = performHttpMetadataPost(curl, &file_upload, auth, &http_code);
    
    status->result_code = curl_ret;
    status->http_code = http_code;
    status->curl_code = curl_ret;
    
    /* Cleanup curl */
    doStopUpload(curl);
    
    if (curl_ret == 0 && http_code == 200) {
        status->upload_completed = true;
        status->auth_success = true;
        snprintf(status->error_message, sizeof(status->error_message), 
                "Metadata POST successful (HTTP %ld)", http_code);
        return 0;
    } else {
        status->upload_completed = false;
        
        /* Determine failure type */
        if (curl_ret != 0) {
            status->auth_success = false;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "CURL error: %d", curl_ret);
        } else if (http_code == 401 || http_code == 403) {
            status->auth_success = false;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "Authentication failed (HTTP %ld)", http_code);
        } else if (http_code >= 400) {
            status->auth_success = true;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "HTTP error %ld", http_code);
        } else {
            snprintf(status->error_message, sizeof(status->error_message), 
                    "Metadata POST failed");
        }
        return -1;
    }
}

int uploadFileWithCodeBigFlowEx(const char *src_file, int server_type, 
                                const char *md5_base64, bool ocsp_enabled, UploadStatusDetail* status)
{
    if (!status) {
        return -1; // Can't report status
    }
    
    init_upload_status(status);
    
    if (!src_file) {
        snprintf(status->error_message, sizeof(status->error_message), 
                "Invalid parameter: src_file=%p", src_file);
        return -1;
    }
    
    /* Set MD5 and OCSP flags for underlying functions */
    __uploadutil_set_md5(md5_base64);
    __uploadutil_set_ocsp(ocsp_enabled);
    
    /* Call underlying function - it will set thread-local status */
    int result = uploadFileWithCodeBigFlow(src_file, server_type);
    
    /* Capture real status codes from thread-local storage */
    long http_code = 0;
    int curl_code = CURLE_OK;
    __uploadutil_get_status(&http_code, &curl_code);
    
    /* Capture FQDN from thread-local storage (CodeBig sets this) */
    __uploadutil_get_fqdn(status->fqdn, sizeof(status->fqdn));
    
    status->result_code = result;
    status->http_code = http_code;
    status->curl_code = curl_code;
    
    if (result == 0) {
        status->upload_completed = true;
        status->auth_success = true;
        snprintf(status->error_message, sizeof(status->error_message), 
                "CodeBig upload successful (HTTP %ld)", http_code);
    } else {
        status->upload_completed = false;
        
        /* Determine failure type from HTTP/CURL codes */
        if (curl_code != CURLE_OK) {
            status->auth_success = false;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "CodeBig CURL error: %d", curl_code);
        } else if (http_code == 401 || http_code == 403) {
            status->auth_success = false;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "CodeBig authorization failed (HTTP %ld)", http_code);
        } else if (http_code >= 400) {
            status->auth_success = true;
            snprintf(status->error_message, sizeof(status->error_message), 
                    "CodeBig HTTP error %ld", http_code);
        } else {
            snprintf(status->error_message, sizeof(status->error_message), 
                    "CodeBig upload failed");
        }
    }
    
    return result;
}

int performS3PutUploadEx(const char *upload_url, const char *src_file, 
                        MtlsAuth_t *auth, const char *md5_base64, bool ocsp_enabled, UploadStatusDetail* status)
{
    if (!status) {
        return -1; // Can't report status
    }
    
    init_upload_status(status);
    
    if (!upload_url || !src_file) {
        snprintf(status->error_message, sizeof(status->error_message), 
                "Invalid parameters: upload_url=%p, src_file=%p", upload_url, src_file);
        return -1;
    }
    
    /* Extract FQDN from upload URL */
    extract_fqdn(upload_url, status->fqdn, sizeof(status->fqdn));
    
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