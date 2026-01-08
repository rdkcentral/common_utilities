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
#include "rdkcertselector.h"

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
 * @brief Perform metadata POST with certificate rotation (Stage 1)
 * @param curl Initialized CURL handle
 * @param upload_url Target URL for metadata POST
 * @param filepath Local file path (used for filename parameter)
 * @param extra_fields Additional POST fields like "md5=..." (can be NULL)
 * @param pthisCertSel Certificate selector handle pointer
 * @param sec_out Output parameter for successful mTLS credentials (use for Stage 2)
 * @param http_code_out Output parameter for HTTP response code
 * @return 0 on success (HTTP 200), -1 on failure
 *
 * Performs metadata POST with automatic certificate rotation:
 * 1. Fetches certificate via getCertificateForUpload()
 * 2. Performs metadata POST to obtain S3 presigned URL
 * 3. On auth failure, rotates certificate and retries
 * 4. Saves result to /tmp/httpresult.txt
 * 5. On success, outputs the cert used (for Stage 2 PUT)
 *
 * This function should be called with retry logic by the caller.
 */
int performMetadataPostWithCertRotation(void *curl, const char *upload_url, const char *filepath,
                                        const char *extra_fields, rdkcertselector_h *pthisCertSel,
                                        MtlsAuth_t *sec_out, long *http_code_out);

/**
 * @brief Perform S3 PUT with same certificate (Stage 2)
 * @param s3_url S3 presigned URL
 * @param src_file Local file path to upload
 * @param sec mTLS credentials from successful Stage 1
 * @return 0 on success, -1 on failure
 *
 * Performs S3 PUT using the same certificate that succeeded in Stage 1.
 * No certificate rotation - uses the cert from metadata POST.
 * Caller should handle proxy fallback if this fails.
 */
int performS3PutWithCert(const char *s3_url, const char *src_file, MtlsAuth_t *sec);

/**
 * @brief Perform metadata POST with certificate rotation - simplified API
 * @param upload_url Target URL for metadata POST
 * @param filepath Local file path (used for filename parameter)
 * @param extra_fields Additional POST fields like "md5=..." (can be NULL)
 * @param sec_out Output parameter for successful mTLS credentials (use for Stage 2)
 * @param http_code_out Output parameter for HTTP response code
 * @param output_file File path where HTTP response will be written
 * @return 0 on success (HTTP 200), -1 on failure
 *
 * Simplified wrapper that manages curl and certificate selector internally.
 * Certificate selector persists across calls for rotation state management.
 * Uses __uploadutil_get_ocsp() for OCSP settings.
 * This function should be called with retry logic by the caller.
 */
int performMetadataPostWithCertRotationEx(const char *upload_url, const char *filepath,
                                          const char *extra_fields, MtlsAuth_t *sec_out,
                                          long *http_code_out, const char *output_file);

#ifdef __cplusplus
}
#endif

#endif /* _RDK_MTLS_UPLOAD_H_ */

