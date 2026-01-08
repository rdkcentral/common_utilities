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
 */

/**
 * @file uploadUtil.h
 * @brief HTTP/S3 upload utilities for RDK components
 *
 * Provides common upload functionality including:
 * - HTTP POST for metadata submission
 * - S3 PUT for file uploads
 * - Optional mTLS authentication support
 * - Two-stage upload workflow (POST + PUT)
 */

#ifndef _RDK_UPLOADUTIL_H_
#define _RDK_UPLOADUTIL_H_

#include "urlHelper.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================
 * Status Codes and Enums
 * ======================================================================== */

/**
 * @brief Upload operation status codes
 */
typedef enum {
    UPLOAD_SUCCESS = 0,      /**< Upload completed successfully */
    UPLOAD_FAIL    = -1      /**< Upload failed */
} UploadStatus;

/* ========================================================================
 * Data Structures - Content Metadata
 * ======================================================================== */

/**
 * @brief Optional hash headers for upload metadata
 */
typedef struct {
    const char *hashvalue;   /**< Hash header (e.g., "x-md5: <value>") */
    const char *hashtime;    /**< Timestamp header (e.g., "x-upload-time: <iso8601>") */
} UploadHashData_t;

/* ========================================================================
 * Data Structures - Request Descriptors
 * ======================================================================== */

/**
 * @brief Upload request descriptor for metadata POST stage
 */
typedef struct {
    char *url;                      /**< Target endpoint URL */
    char *pathname;                 /**< Local file path (used for filename parameter) */
    char *pPostFields;              /**< Additional POST fields (optional) */
    int sslverify;                  /**< SSL peer verification (0=disabled, 1=enabled) */
    UploadHashData_t *hashData;     /**< Optional hash/timestamp headers */
} FileUpload_t;

/* ========================================================================
 * Core Upload Functions
 * ======================================================================== */

/**
 * @brief Stop upload and free CURL resources
 * @param curl CURL handle to cleanup
 */
void doStopUpload(void *curl);

/**
 * @brief Extract S3 presigned URL from HTTP response file
 * @param result_file Path to file containing response data
 * @param out_url Output buffer for extracted URL
 * @param out_url_sz Size of output buffer
 * @return 0 on success, -1 on failure
 *
 * Reads first line from response file and extracts S3 presigned URL.
 * Automatically strips trailing newlines.
 */
int extractS3PresignedUrl(const char *result_file, char *out_url, size_t out_url_sz);

/**
 * @brief Perform S3 PUT upload with optional mTLS authentication
 * @param s3url S3 presigned URL for upload
 * @param localfile Local file path to upload
 * @param auth mTLS authentication credentials (NULL for plain HTTPS)
 * @return 0 on success, -1 on failure
 *
 * Uploads file to S3 using HTTP PUT method. Supports optional mTLS
 * authentication for secure uploads. Validates HTTP response codes
 * (expects 2xx for success).
 */
int performS3PutUpload(const char *s3url, const char *localfile, MtlsAuth_t *auth);

/**
 * @brief Perform HTTP metadata POST with optional mTLS authentication
 * @param in_curl Initialized CURL handle
 * @param pfile_upload Upload request descriptor
 * @param auth mTLS authentication credentials (NULL for plain HTTPS)
 * @param out_httpCode Output parameter for HTTP response code
 * @return 0 on success, CURL error code on failure
 *
 * Posts metadata to server including filename and optional custom fields.
 * Response body is saved to the specified output file for subsequent processing.
 * Supports optional hash headers and mTLS authentication.
 */
int performHttpMetadataPost(void *in_curl,
                            FileUpload_t *pfile_upload,
                            MtlsAuth_t *auth,
                            long *out_httpCode,
                            const char *output_file);

#ifdef __cplusplus
}
#endif

#endif /* _RDK_UPLOADUTIL_H_ */
