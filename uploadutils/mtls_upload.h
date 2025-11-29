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
 * @file mtls_upload.h
 * @brief mTLS-enabled upload with certificate rotation support
 *
 * Provides upload functionality with automatic certificate rotation
 * using the rdkcertselector library. Supports two-stage upload workflow
 * (metadata POST + S3 PUT) with certificate retry on failure.
 */

#ifndef _RDK_MTLS_UPLOAD_H_
#define _RDK_MTLS_UPLOAD_H_

#include "uploadUtil.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBRDKCERTSELECTOR
//#include "rdkcertselector.h"

/**
 * @enum MtlsAuthStatus
 * @brief mTLS certificate fetch status codes
 */
typedef enum {
    MTLS_CERT_FETCH_FAILURE = -1,   /**< Certificate fetch failed */
    MTLS_CERT_FETCH_SUCCESS = 0     /**< Certificate fetch succeeded */
} MtlsAuthStatus;

/**
 * @brief Retrieve mTLS certificate for upload operation
 * @param sec mTLS authentication structure to populate
 * @param pthisCertSel Certificate selector handle
 * @return MTLS_CERT_FETCH_SUCCESS on success, MTLS_CERT_FETCH_FAILURE on failure
 *
 * Fetches certificate from rdkcertselector library and populates MtlsAuth_t
 * structure with certificate path, password, type, and engine information.
 * Handles file:// URI scheme conversion.
 */
MtlsAuthStatus getCertificateForUpload(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel);

#endif /* LIBRDKCERTSELECTOR */

/**
 * @brief Upload file using two-stage workflow with certificate rotation
 * @param upload_url Target URL for metadata POST
 * @param src_file Local file path to upload
 * @return 0 on success, -1 on failure
 *
 * Implements two-stage upload with automatic certificate rotation:
 * 1. Fetches certificate via getCertificateForUpload()
 * 2. Performs metadata POST to obtain S3 presigned URL
 * 3. Performs S3 PUT to upload file content
 * 4. On failure, requests TRY_ANOTHER certificate and retries
 * 5. Continues until success or certificate exhaustion
 *
 * Both POST and PUT stages use the same certificate per attempt.
 * Certificate rotation occurs only when an upload stage fails.
 */
int uploadFileWithTwoStageFlow(const char *upload_url, const char *src_file);


#ifdef __cplusplus
}
#endif

#endif /* _RDK_MTLS_UPLOAD_H_ */
