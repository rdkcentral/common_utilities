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
 * @file codebig_upload.h
 * @brief CodeBig upload functionality for mTLS-enabled upload operations
 */

#ifndef _CODEBIG_UPLOAD_H_
#define _CODEBIG_UPLOAD_H_

#include "uploadUtil.h"
#include "mtls_upload.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CodeBig server types - matching rdkfwupdater definitions */
#define HTTP_SSR_DIRECT      0
#define HTTP_SSR_CODEBIG     1
#define HTTP_XCONF_DIRECT    2
#define HTTP_XCONF_CODEBIG   3
#define HTTP_UNKNOWN         5

#define MAX_HEADER_LEN       512
#define MAX_CODEBIG_URL      1024

/**
 * @brief Create CodeBig authorization signature for upload operation
 * 
 * @param server_type Type of CodeBig service (HTTP_SSR_CODEBIG, HTTP_XCONF_CODEBIG, etc.)
 * @param src_file Local file path to upload
 * @param signurl Buffer to store the signed CodeBig URL
 * @param signurlsize Size of signurl buffer
 * @param outhheader Buffer to store authorization header
 * @param outHeaderSize Size of outhheader buffer
 * @return 0 on success, non-zero on failure
 */
int doCodeBigSigningForUpload(int server_type, const char* src_file, 
                              char *signurl, size_t signurlsize, 
                              char *outhheader, size_t outHeaderSize);

/**
 * @brief External CodeBig signing function (implemented in separate library)
 * 
 * @param server_type Type of CodeBig service
 * @param SignInput Input data to create signature for
 * @param signurl Buffer to store the signed CodeBig URL
 * @param signurlsize Size of signurl buffer
 * @param outhheader Buffer to store authorization header
 * @param outHeaderSize Size of outhheader buffer
 * @return 0 on success, non-zero on failure
 */
extern int doCodeBigSigning(int server_type, const char* SignInput, 
                            char *signurl, size_t signurlsize, 
                            char *outhheader, size_t outHeaderSize);

/**
 * @brief Perform CodeBig metadata POST (Stage 1)
 * 
 * @param curl Initialized CURL handle
 * @param filepath Local file path (used for filename parameter)
 * @param extra_fields Additional POST fields like "md5=..." (can be NULL)
 * @param server_type CodeBig server type (HTTP_SSR_CODEBIG or HTTP_XCONF_CODEBIG)
 * @param http_code_out Output parameter for HTTP response code
 * @return 0 on success, -1 on failure
 *
 * Performs CodeBig-authorized metadata POST:
 * 1. Gets signed URL and auth header from CodeBig service
 * 2. Performs metadata POST to obtain S3 presigned URL
 * 3. Saves result to /tmp/httpresult.txt
 *
 * This function should be called with retry logic by the caller.
 */
int performCodeBigMetadataPost(void *curl, const char *filepath, const char *extra_fields,
                                int server_type, long *http_code_out);

/**
 * @brief Perform S3 PUT for CodeBig (Stage 2)
 * 
 * @param s3_url S3 presigned URL from Stage 1
 * @param src_file Local file path to upload
 * @return 0 on success, -1 on failure
 *
 * Performs S3 PUT using URL obtained from CodeBig metadata POST.
 * No authorization needed - uses presigned URL.
 * Caller should handle fallback if this fails.
 */
int performCodeBigS3Put(const char *s3_url, const char *src_file);

#ifdef __cplusplus
}
#endif


#endif /* _CODEBIG_UPLOAD_H_ */
