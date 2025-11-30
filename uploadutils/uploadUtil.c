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
 * @file uploadUtil.c
 * @brief HTTP/S3 upload utilities implementation
 */

#include "uploadUtil.h"
#include "downloadUtil.h"
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "rdkv_cdl_log_wrapper.h"

void doStopUpload(void *curl)
{
    CURL *curl_dest;
    if (curl != NULL) {
        curl_dest = (CURL *)curl;
        COMMONUTILITIES_INFO("%s : CURL: free resources\n", __FUNCTION__);
        urlHelperDestroyCurl(curl_dest);
    }
}

int extractS3PresignedUrl(const char *result_file, char *out_url, size_t out_url_sz)
{
    if (!result_file || !out_url || out_url_sz == 0) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }
    
    FILE *fp = fopen(result_file, "rb");
    if (!fp) {
        COMMONUTILITIES_ERROR("%s: Unable to open result file %s\n", __FUNCTION__, result_file);
        return -1;
    }
    
    if (!fgets(out_url, (int)out_url_sz, fp)) {
        fclose(fp);
        COMMONUTILITIES_ERROR("%s: Failed to read S3 URL\n", __FUNCTION__);
        return -1;
    }
    
    size_t len = strlen(out_url);
    if (len > 0 && out_url[len - 1] == '\n')
        out_url[len - 1] = '\0';
    
    fclose(fp);
    return 0;
}

int performS3PutUpload(const char *s3url, const char *localfile, MtlsAuth_t *auth)
{
    CURL *curl = NULL;
    CURLcode ret_code = CURLE_OK;
    FILE *fp = NULL;
    long http_code = 0;
    
    if (!s3url || !localfile) {
        COMMONUTILITIES_ERROR("%s: Invalid parameters\n", __FUNCTION__);
        return -1;
    }
    
    curl = (CURL *)doCurlInit();
    if (!curl) {
        COMMONUTILITIES_ERROR("%s: CURL init failed\n", __FUNCTION__);
        return -1;
    }
    
    /* Set common curl options with NULL POST fields for PUT operation */
    ret_code = setCommonCurlOpt(curl, s3url, NULL, true);
    if (ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("%s: setCommonCurlOpt failed: %s\n",
                __FUNCTION__, curl_easy_strerror(ret_code));
        urlHelperDestroyCurl(curl);
        return -1;
    }
    
    /* Apply mTLS if provided */
    if (auth) {
        ret_code = setMtlsHeaders(curl, auth);
        if (ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s: setMtlsHeaders failed: %s\n",
                    __FUNCTION__, curl_easy_strerror(ret_code));
            urlHelperDestroyCurl(curl);
            return -1;
        }
    }
    
    fp = fopen(localfile, "rb");
    if (!fp) {
        COMMONUTILITIES_ERROR("%s: Failed to open %s\n", __FUNCTION__, localfile);
        urlHelperDestroyCurl(curl);
        return -1;
    }
    
    fseek(fp, 0, SEEK_END);
    curl_off_t filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, fp);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, filesize);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    ret_code = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    fclose(fp);
    doStopUpload(curl);

    if (ret_code == CURLE_OK && http_code >= 200 && http_code < 300) {
        COMMONUTILITIES_INFO("%s: S3 PUT success (HTTP %ld)\n", __FUNCTION__, http_code);
        return 0;
    }
    
    COMMONUTILITIES_ERROR("%s: S3 PUT failed: curl=%d, HTTP=%ld\n", __FUNCTION__, ret_code, http_code);
    return -1;
}

int performHttpMetadataPost(void *in_curl,
                            FileUpload_t *pfile_upload,
                            MtlsAuth_t *auth,
                            long *out_httpCode)
{
    CURL *curl;
    CURLcode ret_code = CURLE_OK;
    struct curl_slist *headers = NULL;
    FILE *resp_fp = NULL;

    if (out_httpCode) {
        *out_httpCode = 0;
    }

    if (!in_curl || !pfile_upload || !out_httpCode || 
        !pfile_upload->pathname || !pfile_upload->url) {
        COMMONUTILITIES_ERROR("%s: Parameter validation failed\n", __FUNCTION__);
        return (int)UPLOAD_FAIL;
    }

    curl = (CURL *)in_curl;

    /* Set common curl options */
    ret_code = setCommonCurlOpt(curl, pfile_upload->url, 
                                pfile_upload->pPostFields, pfile_upload->sslverify);
    if (ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("%s: setCommonCurlOpt failed: %s\n",
                __FUNCTION__, curl_easy_strerror(ret_code));
        return (int)ret_code;
    }

    /* Apply mTLS if provided */
    if (auth) {
        ret_code = setMtlsHeaders(curl, auth);
        if (ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s: setMtlsHeaders failed: %s\n",
                    __FUNCTION__, curl_easy_strerror(ret_code));
            return (int)ret_code;
        }
    }

    /* Build POST fields: include filename plus any extra fields */
    char postfields[512];
    if (pfile_upload->pPostFields && pfile_upload->pPostFields[0] != '\0') {
        snprintf(postfields, sizeof(postfields), "filename=%s&%s",
                 pfile_upload->pathname, pfile_upload->pPostFields);
    } else {
        snprintf(postfields, sizeof(postfields), "filename=%s",
                 pfile_upload->pathname);
    }
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields);

    /* Additional headers (hash/time) */
    if (pfile_upload->hashData != NULL) {
        if (pfile_upload->hashData->hashvalue) {
            headers = curl_slist_append(headers, pfile_upload->hashData->hashvalue);
        }
        if (pfile_upload->hashData->hashtime) {
            headers = curl_slist_append(headers, pfile_upload->hashData->hashtime);
        }
        if (headers) {
            ret_code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            if (ret_code != CURLE_OK) {
                COMMONUTILITIES_ERROR("%s: CURLOPT_HTTPHEADER failed: %s\n",
                        __FUNCTION__, curl_easy_strerror(ret_code));
                curl_slist_free_all(headers);
                return (int)ret_code;
            }
        }
    }

    /* Capture response body */
    resp_fp = fopen("/tmp/httpresult.txt", "wb");
    if (!resp_fp) {
        COMMONUTILITIES_ERROR("%s: Failed to open response file\n", __FUNCTION__);
        if (headers) curl_slist_free_all(headers);
        return (int)UPLOAD_FAIL;
    }
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, resp_fp);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    /* Perform request */
    ret_code = curl_easy_perform(curl);
    if (ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("%s: curl_easy_perform failed: %s\n",
                __FUNCTION__, curl_easy_strerror(ret_code));
    } else {
        COMMONUTILITIES_INFO("%s: curl_easy_perform success\n", __FUNCTION__);
    }

    /* Extract HTTP code */
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, out_httpCode);
    COMMONUTILITIES_INFO("%s: HTTP response code=%ld\n", __FUNCTION__, *out_httpCode);

    /* Cleanup */
    fclose(resp_fp);
    if (headers) {
        curl_slist_free_all(headers);
    }

    return (int)ret_code;
}