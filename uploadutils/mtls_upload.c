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
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mtls_upload.c
 * @brief mTLS-enabled upload with certificate rotation implementation
 */

#include "mtls_upload.h"
#include "downloadUtil.h"
#include <string.h>
#include <stdio.h>
#include "upload_status.h"

#include "rdkv_cdl_log_wrapper.h"

/* External declarations for utility functions */
extern void __uploadutil_set_status(long http_code, int curl_code);
extern bool __uploadutil_get_ocsp(void);
extern const char* __uploadutil_get_md5(void);

#ifdef LIBRDKCERTSELECTOR
#include "rdkcertselector.h"
#endif

#define FILESCHEME "file://"
#define URL_MAX 512
#define PATHNAME_MAX 256
#define S3_URL_BUF 1024

/**
 * @brief Retrieve mTLS certificate for upload operation
 */
#ifdef LIBRDKCERTSELECTOR

MtlsAuthStatus getCertificateForUpload(MtlsAuth_t *sec, rdkcertselector_h* pthisCertSel)
{
    char *certUri = NULL;
    char *certPass = NULL;
    char *engine = NULL;
    char *certFile = NULL;

    if (!sec || !pthisCertSel) {
        COMMONUTILITIES_ERROR("[%s:%d] Invalid parameters\n", __FUNCTION__, __LINE__);
        return MTLS_CERT_FETCH_FAILURE;
    }

    rdkcertselectorStatus_t certStat = rdkcertselector_getCert(*pthisCertSel, &certUri, &certPass);

    if (certStat != certselectorOk || certUri == NULL || certPass == NULL) {
        COMMONUTILITIES_ERROR("[%s:%d] Failed to retrieve certificate for MTLS\n",
                   __FUNCTION__, __LINE__);

        rdkcertselector_free(pthisCertSel);

        if (*pthisCertSel == NULL) {
            COMMONUTILITIES_INFO("[%s:%d] Cert selector memory freed\n",
                       __FUNCTION__, __LINE__);
        } else {
            COMMONUTILITIES_ERROR("[%s:%d] Cert selector memory free failed\n",
                       __FUNCTION__, __LINE__);
        }

        return MTLS_CERT_FETCH_FAILURE;
    }

    /* Handle file:// URI scheme */
    certFile = certUri;
    if (strncmp(certFile, FILESCHEME, sizeof(FILESCHEME) - 1) == 0) {
        certFile += (sizeof(FILESCHEME) - 1);
    }

    /* Populate certificate name */
    strncpy(sec->cert_name, certFile, sizeof(sec->cert_name) - 1);
    sec->cert_name[sizeof(sec->cert_name) - 1] = '\0';

    /* Populate certificate password */
    strncpy(sec->key_pas, certPass, sizeof(sec->key_pas) - 1);
    sec->key_pas[sizeof(sec->key_pas) - 1] = '\0';

    /* Get engine (optional) */
    engine = rdkcertselector_getEngine(*pthisCertSel);
    if (engine == NULL) {
        sec->engine[0] = '\0';
    } else {
        strncpy(sec->engine, engine, sizeof(sec->engine) - 1);
        sec->engine[sizeof(sec->engine) - 1] = '\0';
    }

    /* Set certificate type to P12 */
    strncpy(sec->cert_type, "P12", sizeof(sec->cert_type) - 1);
    sec->cert_type[sizeof(sec->cert_type) - 1] = '\0';

    COMMONUTILITIES_INFO("[%s:%d] MTLS cert success. cert=%s, type=%s, engine=%s\n",
               __FUNCTION__, __LINE__, sec->cert_name, sec->cert_type, sec->engine);

    return MTLS_CERT_FETCH_SUCCESS;

}

#endif

#ifdef LIBRDKCERTSELECTOR
/**
 * @brief Perform metadata POST with certificate rotation (Stage 1 - Public API)
 */
int performMetadataPostWithCertRotation(void *curl, const char *upload_url, const char *filepath,
                                        const char *extra_fields, rdkcertselector_h *pthisCertSel,
                                        MtlsAuth_t *sec_out, long *http_code_out)
{
    MtlsAuthStatus mtls_status;
    MtlsAuth_t sec;
    int curl_ret_code = -1;
    long http_code = 0;

    if (!curl || !upload_url || !filepath || !pthisCertSel || !sec_out || !http_code_out) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    *http_code_out = 0;

    /* Apply OCSP setting if enabled */
    if (__uploadutil_get_ocsp()) {
        CURLcode ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
        if (ret != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s: CURLOPT_SSL_VERIFYSTATUS failed: %s\n",
                                __FUNCTION__, curl_easy_strerror(ret));
        }
    }

    /* Prepare FileUpload_t structure */
    FileUpload_t file_upload;
    memset(&file_upload, 0, sizeof(FileUpload_t));
    
    char urlbuf[URL_MAX];
    char pathbuf[PATHNAME_MAX];
    strncpy(urlbuf, upload_url, URL_MAX - 1);
    urlbuf[URL_MAX - 1] = '\0';
    strncpy(pathbuf, filepath, PATHNAME_MAX - 1);
    pathbuf[PATHNAME_MAX - 1] = '\0';
    
    file_upload.url = urlbuf;
    file_upload.pathname = pathbuf;
    file_upload.sslverify = 1;
    file_upload.hashData = NULL;
    file_upload.pPostFields = (char*)extra_fields;

    do {
        memset(&sec, 0, sizeof(MtlsAuth_t));
        http_code = 0;

        /* Fetch certificate */
        mtls_status = getCertificateForUpload(&sec, pthisCertSel);
        if (mtls_status == MTLS_CERT_FETCH_FAILURE) {
            COMMONUTILITIES_ERROR("%s: Certificate fetch failed\n", __FUNCTION__);
            return -1;
        }

        /* Perform metadata POST with mTLS */
        curl_ret_code = performHttpMetadataPost(curl, &file_upload, &sec, &http_code);
        *http_code_out = http_code;

        if (curl_ret_code == 0 && http_code >= 200 && http_code < 300) {
            COMMONUTILITIES_INFO("%s: Metadata POST success (HTTP %ld)\n", __FUNCTION__, http_code);
            /* Save the successful certificate for Stage 2 */
            memcpy(sec_out, &sec, sizeof(MtlsAuth_t));
            __uploadutil_set_status(http_code, curl_ret_code);
            return 0;
        }

        COMMONUTILITIES_ERROR("%s: Metadata POST failed curl=%d http=%ld\n",
                   __FUNCTION__, curl_ret_code, http_code);

    } while (rdkcertselector_setCurlStatus(*pthisCertSel, curl_ret_code, upload_url) == TRY_ANOTHER);

    __uploadutil_set_status(http_code, curl_ret_code);
    return -1;
}

/**
 * @brief Perform S3 PUT with provided certificate (Stage 2 - Public API)
 */
int performS3PutWithCert(const char *s3_url, const char *src_file, MtlsAuth_t *sec)
{
    if (!s3_url || !src_file) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    int result = performS3PutUpload(s3_url, src_file, sec);
    
    if (result == 0) {
        COMMONUTILITIES_INFO("%s: S3 PUT success\n", __FUNCTION__);
    } else {
        COMMONUTILITIES_ERROR("%s: S3 PUT failed\n", __FUNCTION__);
    }
    
    return result;
}

/**
 * @brief Wrapper for metadata POST with certificate rotation - manages cert selector
 * @param upload_url Target URL for metadata POST
 * @param filepath Local file path
 * @param extra_fields Extra POST fields (e.g., MD5), can be NULL
 * @param sec_out Output: successful certificate for Stage 2
 * @param http_code_out Output: HTTP response code
 * @return 0 on success, -1 on failure
 */
int performMetadataPostWithCertRotationEx(const char *upload_url, const char *filepath,
                                          const char *extra_fields, MtlsAuth_t *sec_out,
                                          long *http_code_out)
{
#ifdef LIBRDKCERTSELECTOR
    void *curl = NULL;
    static rdkcertselector_h certSelector = NULL;
    int result = -1;

    if (!upload_url || !filepath || !sec_out || !http_code_out) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    /* Initialize certificate selector if not already done */
    if (!certSelector) {
        certSelector = rdkcertselector_new(NULL, NULL, "MTLS");
        if (!certSelector) {
            COMMONUTILITIES_ERROR("%s: Failed to initialize certificate selector\n", __FUNCTION__);
            return -1;
        }
    }

    /* Initialize curl */
    curl = doCurlInit();
    if (!curl) {
        COMMONUTILITIES_ERROR("%s: Failed to initialize curl\n", __FUNCTION__);
        return -1;
    }

    /* Apply OCSP setting if enabled */
    if (__uploadutil_get_ocsp()) {
        CURLcode ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
        if (ret != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s: CURLOPT_SSL_VERIFYSTATUS failed: %s\n",
                                __FUNCTION__, curl_easy_strerror(ret));
        }
    }

    /* Call the actual rotation function */
    result = performMetadataPostWithCertRotation(curl, upload_url, filepath,
                                                 extra_fields, &certSelector,
                                                 sec_out, http_code_out);

    /* Cleanup curl */
    doStopUpload(curl);

    return result;
#else
    COMMONUTILITIES_ERROR("%s: Not supported without LIBRDKCERTSELECTOR\n", __FUNCTION__);
    return -1;
#endif
}


#endif
