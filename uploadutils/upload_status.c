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
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

void init_upload_status(UploadStatusDetail* status)
{
    if (!status) return;
    
    memset(status, 0, sizeof(UploadStatusDetail));
    status->result_code = -1;
    status->http_code = 0;
    status->curl_code = CURLE_FAILED_INIT;
    status->upload_completed = false;
    status->auth_success = false;
    strcpy(status->error_message, "Not initialized");
}

int uploadFileWithTwoStageFlowEx(const char *upload_url, const char *src_file, UploadStatusDetail* status)
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
    
    // Call original function and capture detailed information
    // For now, simulate the status capture until upload library is enhanced
    int result = uploadFileWithTwoStageFlow(upload_url, src_file);
    
    // Map result to detailed status
    if (result == 0) {
        status->result_code = 0;
        status->http_code = 200;
        status->curl_code = CURLE_OK;
        status->upload_completed = true;
        status->auth_success = true;
        strcpy(status->error_message, "Upload successful");
    } else {
        status->result_code = result;
        
        // Simulate different failure scenarios
        if (result == -1) {
            status->http_code = 500;
            status->curl_code = CURLE_COULDNT_CONNECT;
            status->auth_success = false;
            strcpy(status->error_message, "Connection failed");
        } else if (result == -2) {
            status->http_code = 401;
            status->curl_code = CURLE_OK;
            status->auth_success = false;
            strcpy(status->error_message, "Authentication failed");
        } else {
            status->http_code = 500;
            status->curl_code = CURLE_OPERATION_TIMEDOUT;
            strcpy(status->error_message, "Upload operation failed");
        }
    }
    
    return result;
}

int uploadFileWithCodeBigFlowEx(const char *src_file, int server_type, UploadStatusDetail* status)
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
    
    // Call original function and capture detailed information
    int result = uploadFileWithCodeBigFlow(src_file, server_type);
    
    // Map result to detailed status
    if (result == 0) {
        status->result_code = 0;
        status->http_code = 200;
        status->curl_code = CURLE_OK;
        status->upload_completed = true;
        status->auth_success = true;
        strcpy(status->error_message, "CodeBig upload successful");
    } else {
        status->result_code = result;
        
        // Simulate CodeBig-specific failure scenarios
        if (result == -1) {
            status->http_code = 403;
            status->curl_code = CURLE_OK;
            status->auth_success = false;
            strcpy(status->error_message, "CodeBig authorization failed");
        } else if (result == -2) {
            status->http_code = 404;
            status->curl_code = CURLE_OK;
            status->auth_success = true;
            strcpy(status->error_message, "CodeBig service not found");
        } else {
            status->http_code = 500;
            status->curl_code = CURLE_HTTP_RETURNED_ERROR;
            strcpy(status->error_message, "CodeBig upload failed");
        }
    }
    
    return result;
}

int performS3PutUploadEx(const char *upload_url, const char *src_file, 
                        const HashHeaders *hash_headers, UploadStatusDetail* status)
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
    
    // Call original function and capture detailed information
    int result = performS3PutUpload(upload_url, src_file, hash_headers);
    
    // Map result to detailed status
    if (result == 0) {
        status->result_code = 0;
        status->http_code = 200;
        status->curl_code = CURLE_OK;
        status->upload_completed = true;
        status->auth_success = true;
        strcpy(status->error_message, "S3 upload successful");
    } else {
        status->result_code = result;
        
        // Simulate S3-specific failure scenarios
        if (result == -1) {
            status->http_code = 403;
            status->curl_code = CURLE_OK;
            status->auth_success = false;
            strcpy(status->error_message, "S3 access denied");
        } else if (result == -2) {
            status->http_code = 404;
            status->curl_code = CURLE_OK;
            status->auth_success = true;
            strcpy(status->error_message, "S3 bucket/key not found");
        } else {
            status->http_code = 500;
            status->curl_code = CURLE_SEND_ERROR;
            strcpy(status->error_message, "S3 upload transmission failed");
        }
    }
    
    return result;
}