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
} UploadStatusDetail;

/**
 * @brief Initialize upload status structure
 * 
 * @param status Pointer to UploadStatusDetail structure
 */
void init_upload_status(UploadStatusDetail* status);

/**
 * @brief Enhanced mTLS upload with detailed status return
 * 
 * @param upload_url Upload endpoint URL
 * @param src_file Local file path to upload
 * @param status Pointer to structure to receive detailed status
 * @return int Overall result code (0=success, negative=failure)
 */
int uploadFileWithTwoStageFlowEx(const char *upload_url, const char *src_file, UploadStatusDetail* status);

/**
 * @brief Enhanced CodeBig upload with detailed status return
 * 
 * @param src_file Local file path to upload  
 * @param server_type CodeBig server type
 * @param status Pointer to structure to receive detailed status
 * @return int Overall result code (0=success, negative=failure)
 */
int uploadFileWithCodeBigFlowEx(const char *src_file, int server_type, UploadStatusDetail* status);

/**
 * @brief Enhanced S3 PUT upload with detailed status return
 * 
 * @param upload_url S3 presigned URL
 * @param src_file Local file path to upload
 * @param hash_headers Optional hash headers (can be NULL)
 * @param status Pointer to structure to receive detailed status
 * @return int Overall result code (0=success, negative=failure)
 */
int performS3PutUploadEx(const char *upload_url, const char *src_file, 
                        const HashHeaders *hash_headers, UploadStatusDetail* status);

#ifdef __cplusplus
}
#endif

#endif /* _RDK_UPLOAD_STATUS_H_ */