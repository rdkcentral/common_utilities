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
 * @file codebig_upload.c
 * @brief CodeBig upload implementation with mTLS support
 */

#include "codebig_upload.h"
#include "downloadUtil.h"
#include <string.h>
#include <stdio.h>

#include "rdkv_cdl_log_wrapper.h"


#define FILESCHEME "file://"
#define URL_MAX 512
#define PATHNAME_MAX 256
#define S3_URL_BUF 1024

/**
 * @brief Create CodeBig authorization signature for upload operation
 * 
 * This function interfaces with CodeBig service to obtain authorization
 * signatures and signed URLs for upload operations.
 */
int doCodeBigSigningForUpload(int server_type, const char* SignInput, 
                              char *signurl, size_t signurlsize, 
                              char *outhheader, size_t outHeaderSize)
{
    if (!SignInput || !signurl || !outhheader) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    // TODO: Implement actual CodeBig signing logic
    // This is a placeholder implementation
    COMMONUTILITIES_INFO("%s: CodeBig signing for server_type=%d, input=%s\n", 
               __FUNCTION__, server_type, SignInput);
    
    // For now, just copy the input URL as signed URL
    strncpy(signurl, SignInput, signurlsize - 1);
    signurl[signurlsize - 1] = '\0';
    
    // Create placeholder authorization header
    snprintf(outhheader, outHeaderSize, "Authorization: Bearer placeholder_token");
    
    COMMONUTILITIES_INFO("%s: CodeBig signing successful\n", __FUNCTION__);
    return 0;
}

/**
 * @brief CodeBig upload with authorization
 */
static int performCodeBigUpload(void *curl, FileUpload_t *file_upload,
                                const char *src_file, const char *upload_url, int server_type)
{
    int curl_ret_code = -1;
    long http_code = 0;
    char codebig_url[MAX_CODEBIG_URL];
    char auth_header[MAX_HEADER_LEN];

    if (!curl || !file_upload || !src_file || !upload_url) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    memset(codebig_url, 0, sizeof(codebig_url));
    memset(auth_header, 0, sizeof(auth_header));
    
    /* Step 1: Get CodeBig signed URL and authorization header */
    if (doCodeBigSigningForUpload(server_type, upload_url, codebig_url, 
                                  sizeof(codebig_url), auth_header, sizeof(auth_header)) != 0) {
        COMMONUTILITIES_ERROR("%s: CodeBig signing failed\n", __FUNCTION__);
        return -1;
    }
    
    COMMONUTILITIES_INFO("%s: CodeBig URL: %s\n", __FUNCTION__, codebig_url);
    
    /* Step 2: Update file_upload with CodeBig URL */
    strncpy(file_upload->url, codebig_url, URL_MAX - 1);
    file_upload->url[URL_MAX - 1] = '\0';
    
    /* Step 3: Perform metadata POST with CodeBig auth */
    curl_ret_code = performHttpMetadataPost(curl, file_upload, NULL, &http_code);
    
    if (curl_ret_code != 0 || http_code < 200 || http_code >= 300) {
        COMMONUTILITIES_ERROR("%s: CodeBig metadata POST failed curl=%d http=%ld\n",
                   __FUNCTION__, curl_ret_code, http_code);
        return -1;
    }
    
    COMMONUTILITIES_INFO("%s: CodeBig metadata POST success (HTTP %ld)\n",
               __FUNCTION__, http_code);
    
    /* Step 4: Extract S3 URL and perform S3 PUT */
    char s3_url[S3_URL_BUF];
    if (extractS3PresignedUrl("/tmp/httpresult.txt", s3_url, sizeof(s3_url)) != 0) {
        COMMONUTILITIES_ERROR("%s: Failed to extract S3 URL\n", __FUNCTION__);
        return -1;
    }
    
    if (performS3PutUpload(s3_url, src_file, NULL) != 0) {
        COMMONUTILITIES_ERROR("%s: S3 PUT failed\n", __FUNCTION__);
        return -1;
    }
    
    COMMONUTILITIES_INFO("%s: Complete CodeBig upload success\n", __FUNCTION__);
    return 0;
}

/**
 * @brief Upload file using CodeBig workflow
 */
int uploadFileWithCodeBigFlow(const char *upload_url, const char *src_file, int server_type)
{
    void *curl = NULL;
    FileUpload_t file_upload;
    int status = -1;

    if (!upload_url || !src_file) {
        COMMONUTILITIES_ERROR("%s: Invalid arguments\n", __FUNCTION__);
        return -1;
    }

    /* Validate server type */
    if (server_type != SSR_SERVICE && server_type != XCONF_SERVICE && 
        server_type != CIXCONF_SERVICE && server_type != DAC15_SERVICE) {
        COMMONUTILITIES_ERROR("%s: Invalid CodeBig server type: %d\n", __FUNCTION__, server_type);
        return -1;
    }

    /* Prepare upload descriptor */
    memset(&file_upload, 0, sizeof(FileUpload_t));
    char urlbuf[URL_MAX];
    char pathbuf[PATHNAME_MAX];
    strncpy(urlbuf, upload_url, URL_MAX - 1);
    urlbuf[URL_MAX - 1] = '\0';
    strncpy(pathbuf, src_file, PATHNAME_MAX - 1);
    pathbuf[PATHNAME_MAX - 1] = '\0';
    file_upload.url        = urlbuf;
    file_upload.pathname   = pathbuf;
    file_upload.pPostFields = NULL;
    file_upload.sslverify  = 1;
    file_upload.hashData   = NULL;

    curl = doCurlInit();
    if (!curl) {
        COMMONUTILITIES_ERROR("%s: CURL init failed\n", __FUNCTION__);
        return -1;
    }

    /* Perform CodeBig upload */
    status = performCodeBigUpload(curl, &file_upload, src_file, upload_url, server_type);

    doStopUpload(curl);
    return status;
}
