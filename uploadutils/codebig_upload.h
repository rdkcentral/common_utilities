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

/* CodeBig server types */
#define INVALID_SERVICE    0
#define SSR_SERVICE        1
#define XCONF_SERVICE      2
#define CIXCONF_SERVICE    4
#define DAC15_SERVICE      14

#define MAX_HEADER_LEN     512
#define MAX_CODEBIG_URL    1024

/**
 * @brief Create CodeBig authorization signature for upload operation
 * 
 * @param server_type Type of CodeBig service (SSR_SERVICE, XCONF_SERVICE, etc.)
 * @param SignInput Input data to create signature for (typically upload URL)
 * @param signurl Buffer to store the signed CodeBig URL
 * @param signurlsize Size of signurl buffer
 * @param outhheader Buffer to store authorization header
 * @param outHeaderSize Size of outhheader buffer
 * @return 0 on success, non-zero on failure
 */
int doCodeBigSigningForUpload(int server_type, const char* SignInput, 
                              char *signurl, size_t signurlsize, 
                              char *outhheader, size_t outHeaderSize);

/**
 * @brief Upload file using CodeBig two-stage workflow with mTLS
 * 
 * @param upload_url Original upload URL to be signed with CodeBig
 * @param src_file Local file path to upload
 * @param server_type CodeBig server type
 * @return 0 on success, negative value on failure
 */
int uploadFileWithCodeBigFlow(const char *upload_url, const char *src_file, int server_type);

#ifdef __cplusplus
}
#endif

#endif /* _CODEBIG_UPLOAD_H_ */
