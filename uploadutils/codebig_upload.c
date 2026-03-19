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
 */

/**
 * @file codebig_upload.c
 * @brief CodeBig upload implementation with mTLS support
 */

#include "codebig_upload.h"
#include "downloadUtil.h"
#include "upload_status.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "rdkv_cdl_log_wrapper.h"

/* External status tracking for enhanced wrapper functions */
extern void __uploadutil_set_status(long http_code, int curl_code);


#define URL_MAX 512
#define PATHNAME_MAX 256
#define S3_URL_BUF 1024

/**
 * @brief Create CodeBig authorization signature for upload operation
 * 
 * This function interfaces with CodeBig service to obtain authorization
 * signatures and upload URLs for upload operations based on file and service type.
 */
int doCodeBigSigningForUpload(int server_type, const char* src_file, 
                              char *signurl, size_t signurlsize, 
                              char *outhheader, size_t outHeaderSize)
{
    if (!src_file || !signurl || !outhheader) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    // Validate server type
    if (server_type != HTTP_SSR_CODEBIG && server_type != HTTP_XCONF_CODEBIG) {
        COMMONUTILITIES_ERROR("%s: Invalid CodeBig server type: %d\n", __FUNCTION__, server_type);
        return -1;
    }
    
    COMMONUTILITIES_INFO("%s: CodeBig signing for server_type=%d, file=%s\n", 
               __FUNCTION__, server_type, src_file);

    // Call the actual CodeBig signing implementation from external library
    // This matches rdkfwupdater pattern where doCodeBigSigning is implemented elsewhere
    int signFailed = doCodeBigSigning(server_type, src_file, signurl, signurlsize, outhheader, outHeaderSize);
    
    if (signFailed == 0) {
        COMMONUTILITIES_INFO("%s: CodeBig signing successful\n", __FUNCTION__);
        return 0; // Return 0 for success when URL is provided
    } else {
        COMMONUTILITIES_ERROR("%s: CodeBig signing failed with code: %d\n", __FUNCTION__, signFailed);
        return -1; // Return -1 for failure
    }
}

/**
 * @brief Perform CodeBig metadata POST (Stage 1 - Public API)
 */
int performCodeBigMetadataPost(void *curl, const char *filepath, const char *extra_fields,
                                int server_type, long *http_code_out)
{
    int curl_ret_code = -1;
    long http_code = 0;
    char codebig_url[MAX_CODEBIG_URL] = {0};
    char auth_header[MAX_HEADER_LEN] = {0};

    if (!curl || !filepath || !http_code_out) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    *http_code_out = 0;

    /* Validate server type */
    if (server_type != HTTP_SSR_CODEBIG && server_type != HTTP_XCONF_CODEBIG) {
        COMMONUTILITIES_ERROR("%s: Invalid CodeBig server type: %d\n", __FUNCTION__, server_type);
        return -1;
    }

    /* Apply OCSP setting if enabled */
    extern bool __uploadutil_get_ocsp(void);
    if (__uploadutil_get_ocsp()) {
        CURLcode ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
        if (ret != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s: CURLOPT_SSL_VERIFYSTATUS failed: %s\n",
                                __FUNCTION__, curl_easy_strerror(ret));
        }
    }

    /* Step 1: Get CodeBig signed URL and authorization header */
    if (doCodeBigSigningForUpload(server_type, filepath, codebig_url,
                                  sizeof(codebig_url), auth_header, sizeof(auth_header)) != 0) {
        COMMONUTILITIES_ERROR("%s: CodeBig signing failed\n", __FUNCTION__);
        return -1;
    }

    COMMONUTILITIES_INFO("%s: CodeBig URL: %s\n", __FUNCTION__, codebig_url);

    /* Extract and store FQDN from CodeBig URL */
    char fqdn[256] = {0};
    const char *hostname_start = strstr(codebig_url, "://");
    if (hostname_start) {
        hostname_start += 3;
        const char *hostname_end = hostname_start;
        while (*hostname_end && *hostname_end != ':' && *hostname_end != '/' && *hostname_end != '?') {
            hostname_end++;
        }
        size_t len = hostname_end - hostname_start;
        if (len > 0 && len < sizeof(fqdn)) {
            strncpy(fqdn, hostname_start, len);
            fqdn[len] = '\0';
            __uploadutil_set_fqdn(fqdn);
        }
    }

    /* Step 2: Prepare FileUpload_t structure */
    FileUpload_t file_upload;
    memset(&file_upload, 0, sizeof(FileUpload_t));

    char urlbuf[URL_MAX];
    char pathbuf[PATHNAME_MAX];
    strncpy(urlbuf, codebig_url, URL_MAX - 1);
    urlbuf[URL_MAX - 1] = '\0';
    strncpy(pathbuf, filepath, PATHNAME_MAX - 1);
    pathbuf[PATHNAME_MAX - 1] = '\0';

    file_upload.url = urlbuf;
    file_upload.pathname = pathbuf;
    file_upload.sslverify = 1;
    file_upload.hashData = NULL;
    file_upload.pPostFields = (char*)extra_fields;

    /* Step 3: Perform metadata POST with CodeBig auth */
    curl_ret_code = performHttpMetadataPost(curl, &file_upload, NULL, &http_code);
    *http_code_out = http_code;

    if (curl_ret_code != 0 || http_code < 200 || http_code >= 300) {
        COMMONUTILITIES_ERROR("%s: CodeBig metadata POST failed curl=%d http=%ld\n",
                   __FUNCTION__, curl_ret_code, http_code);
        __uploadutil_set_status(http_code, curl_ret_code);
        return -1;
    }

    COMMONUTILITIES_INFO("%s: CodeBig metadata POST success (HTTP %ld)\n",
               __FUNCTION__, http_code);
    __uploadutil_set_status(http_code, curl_ret_code);
    return 0;
}

/**
 * @brief Perform S3 PUT for CodeBig (Stage 2 - Public API)
 */
int performCodeBigS3Put(const char *s3_url, const char *src_file)
{
    if (!s3_url || !src_file) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    int result = performS3PutUpload(s3_url, src_file, NULL);

    if (result == 0) {
        COMMONUTILITIES_INFO("%s: CodeBig S3 PUT success\n", __FUNCTION__);
    } else {
        COMMONUTILITIES_ERROR("%s: CodeBig S3 PUT failed\n", __FUNCTION__);
    }

    return result;

}
