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
 * @file upload_status.h
 * @brief Enhanced upload status structure with detailed response information
 */

#ifndef _RDK_UPLOAD_STATUS_H_
#define _RDK_UPLOAD_STATUS_H_

#include <stdbool.h>
#include "urlHelper.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detailed upload operation result
 */
typedef struct {
    int result_code;        /**< Overall result: 0=success, negative=failure */
    long http_code;         /**< HTTP response code (200, 404, 500, etc.) */
    int curl_code;          /**< Curl result code (CURLE_OK=0, errors>0) */
    bool upload_completed;  /**< Whether upload stage completed */
    bool auth_success;      /**< Whether authentication succeeded */
    char error_message[256]; /**< Human-readable error description */
    char fqdn[256];         /**< Fully Qualified Domain Name (hostname) from upload URL */
} UploadStatusDetail;

/**
 * @brief Initialize upload status structure
 * 
 * @param status Pointer to UploadStatusDetail structure
 */
void init_upload_status(UploadStatusDetail* status);

/**
 * @brief Internal: Set status codes for capture by wrapper functions
 * @note This is used internally by upload functions - do not call directly
 */
void __uploadutil_set_status(long http_code, int curl_code);

/**
 * @brief Internal: Get and reset status codes
 * @note This is used internally by wrapper functions - do not call directly
 */
void __uploadutil_get_status(long *http_code, int *curl_code);

/**
 * @brief Internal: Set FQDN for capture by wrapper functions
 * @note This is used internally by upload functions - do not call directly
 */
void __uploadutil_set_fqdn(const char *fqdn);

/**
 * @brief Internal: Get and reset FQDN
 * @note This is used internally by wrapper functions - do not call directly
 */
void __uploadutil_get_fqdn(char *fqdn, size_t size);

/**
 * @brief Internal: Set OCSP enabled flag
 * @note This is used internally by wrapper functions - do not call directly
 */
void __uploadutil_set_ocsp(bool enabled);

/**
 * @brief Internal: Get OCSP enabled flag
 * @note This is used by upload functions to determine OCSP behavior
 */
bool __uploadutil_get_ocsp(void);

/**
 * @brief Internal: Set MD5 hash for upload
 * @note This is used internally by wrapper functions - do not call directly
 */
void __uploadutil_set_md5(const char *md5);

/**
 * @brief Internal: Get MD5 hash for upload
 * @note This is used by upload functions to include MD5 in POST data
 * @return MD5 string or NULL if not set
 */
const char* __uploadutil_get_md5(void);

/**
 * @brief Enhanced mTLS upload with detailed status return
 * 
 * @param upload_url Upload endpoint URL
 * @param src_file Local file path to upload
 * @param md5_base64 Optional MD5 hash (base64 encoded) for integrity check (can be NULL)
 * @param ocsp_enabled Enable OCSP certificate validation
 * @param status Pointer to structure to receive detailed status
 * @return int Overall result code (0=success, negative=failure)
 */
int uploadFileWithTwoStageFlowEx(const char *upload_url, const char *src_file, 
                                 const char *md5_base64, bool ocsp_enabled, UploadStatusDetail* status);

/**
 * @brief Enhanced CodeBig upload with detailed status return
 * 
 * @param src_file Local file path to upload  
 * @param server_type CodeBig server type
 * @param md5_base64 Optional MD5 hash (base64 encoded) for integrity check (can be NULL)
 * @param ocsp_enabled Enable OCSP certificate validation
 * @param status Pointer to structure to receive detailed status
 * @return int Overall result code (0=success, negative=failure)
 */
int uploadFileWithCodeBigFlowEx(const char *src_file, int server_type, 
                                const char *md5_base64, bool ocsp_enabled, UploadStatusDetail* status);

/**
 * @brief Enhanced S3 PUT upload with detailed status return
 * 
 * @param upload_url S3 presigned URL
 * @param src_file Local file path to upload
 * @param auth Optional mTLS authentication (can be NULL)
 * @param md5_base64 Optional MD5 hash (base64 encoded) for integrity check (can be NULL)
 * @param ocsp_enabled Enable OCSP certificate validation
 * @param status Pointer to structure to receive detailed status
 * @return int Overall result code (0=success, negative=failure)
 */
int performS3PutUploadEx(const char *upload_url, const char *src_file, 
                        MtlsAuth_t *auth, const char *md5_base64, bool ocsp_enabled, UploadStatusDetail* status);

#ifdef __cplusplus
}
#endif

#endif /* _RDK_UPLOAD_STATUS_H_ */