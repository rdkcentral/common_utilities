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
 * @file mtls_upload.c
 * @brief mTLS-enabled upload with certificate rotation implementation
 */

#include "mtls_upload.h"
#include "downloadUtil.h"
#include <string.h>
#include <stdio.h>

#include "rdkv_cdl_log_wrapper.h"

/* External status tracking for enhanced wrapper functions */
extern void __uploadutil_set_status(long http_code, int curl_code);

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
/**
 * @brief Two-stage upload with certificate rotation
 */
#ifdef LIBRDKCERTSELECTOR
static int performTwoStageUploadWithCertRotation(void *curl, FileUpload_t *file_upload,
                                                 const char *src_file, rdkcertselector_h *pthisCertSel,
                                                 const char *upload_url)
{

    MtlsAuthStatus mtls_status = MTLS_CERT_FETCH_SUCCESS;
    MtlsAuth_t sec;
    int curl_ret_code = -1;
    long http_code = 0;
    int result = -1;

    if (!curl || !file_upload || !src_file || !pthisCertSel || !upload_url) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }

    do {
        memset(&sec, 0, sizeof(MtlsAuth_t));
        http_code = 0;

        /* Fetch certificate */
        mtls_status = getCertificateForUpload(&sec, pthisCertSel);
        if (mtls_status == MTLS_CERT_FETCH_FAILURE) {
            COMMONUTILITIES_ERROR("%s: Certificate fetch failed\n", __FUNCTION__);
            result = -1;
            break;
        }

        /* Stage 1: Metadata POST with mTLS */
        curl_ret_code = performHttpMetadataPost(curl, file_upload, &sec, &http_code);

        if (curl_ret_code != 0 || http_code < 200 || http_code >= 300) {
            COMMONUTILITIES_ERROR("%s: Metadata POST failed curl=%d http=%ld\n",
                       __FUNCTION__, curl_ret_code, http_code);
            result = -1;
            continue;
        }

        COMMONUTILITIES_INFO("%s: Metadata POST success (HTTP %ld)\n",
                   __FUNCTION__, http_code);

        /* Stage 2: Extract S3 URL and perform S3 PUT with same mTLS cert */
        char s3_url[S3_URL_BUF];
        if (extractS3PresignedUrl("/tmp/httpresult.txt", s3_url, sizeof(s3_url)) != 0) {
            COMMONUTILITIES_ERROR("%s: Failed to extract S3 URL\n", __FUNCTION__);
            result = -1;
            continue;
        }

        if (performS3PutUpload(s3_url, src_file, &sec) != 0) {
            COMMONUTILITIES_ERROR("%s: S3 PUT failed\n", __FUNCTION__);
            result = -1;
            continue;
        }

        COMMONUTILITIES_INFO("%s: Complete two-stage upload success\n", __FUNCTION__);
        result = 0;
        break;

    } while (rdkcertselector_setCurlStatus(*pthisCertSel, curl_ret_code, upload_url) == TRY_ANOTHER);

    /* Report final status for enhanced wrapper functions */
    __uploadutil_set_status(http_code, curl_ret_code);

    return result;

}
#endif

/**
 * @brief Upload file using two-stage workflow with certificate rotation
 */
int uploadFileWithTwoStageFlow(const char *upload_url, const char *src_file)
{
    void *curl = NULL;
    FileUpload_t file_upload;
    int status = -1;
#ifdef LIBRDKCERTSELECTOR
    static rdkcertselector_h thisCertSel = NULL;
#endif

    if (!upload_url || !src_file) {
        COMMONUTILITIES_ERROR("%s: Invalid arguments\n", __FUNCTION__);
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
    file_upload.sslverify  = 1;
    file_upload.hashData   = NULL;
    
    /* Set MD5 in POST fields if provided (matches script line 318) */
    extern const char* __uploadutil_get_md5(void);
    const char *md5 = __uploadutil_get_md5();
    char postfields[256] = {0};
    if (md5) {
        snprintf(postfields, sizeof(postfields), "md5=%s", md5);
        file_upload.pPostFields = postfields;
    } else {
        file_upload.pPostFields = NULL;
    }

    curl = doCurlInit();
    if (!curl) {
        COMMONUTILITIES_ERROR("%s: CURL init failed\n", __FUNCTION__);
        return -1;
    }

    /* Apply OCSP setting if enabled (matches script line 357) */
    extern bool __uploadutil_get_ocsp(void);
    if (__uploadutil_get_ocsp()) {
        CURLcode ret = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
        if (ret != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s: CURLOPT_SSL_VERIFYSTATUS failed: %s\n", 
                                __FUNCTION__, curl_easy_strerror(ret));
        }
    }

#ifdef LIBRDKCERTSELECTOR
    if (!thisCertSel) {
        thisCertSel = rdkcertselector_new(NULL, NULL, "MTLS");
        if (!thisCertSel) {
            COMMONUTILITIES_ERROR("%s: Cert selector init failed\n", __FUNCTION__);
            doStopUpload(curl);
            return -1;
        }
    }

    /* Perform two-stage upload with certificate rotation */
    status = performTwoStageUploadWithCertRotation(curl, &file_upload, src_file,
                                                   &thisCertSel, upload_url);

    if (curl) {
        doStopUpload(curl);
        curl = NULL;
    }
    rdkcertselector_free(&thisCertSel);
#else
    COMMONUTILITIES_ERROR("%s: Two-stage upload not supported without LIBRDKCERTSELECTOR\n", __FUNCTION__);
    doStopUpload(curl);
    status = -1;
#endif

    return status;
}