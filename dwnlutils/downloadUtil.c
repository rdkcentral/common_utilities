/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
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

#include "downloadUtil.h"
#include "rdkv_cdl_log_wrapper.h"


/* doCurlInit(): ininitialize curl resources
 * param : void
 * Return :void * : Send back curl init pointer
 * */
void *doCurlInit(void)
{
    CURL *curl = NULL;
    curl = urlHelperCreateCurl();
    if (curl == NULL) {
        SWLOG_ERROR("%s: curl init failed\n", __FUNCTION__);
    } else {
        SWLOG_INFO("%s: curl init success\n", __FUNCTION__);
    }
    return (void *)curl;
}
/* doStopDownload(): uninitialize curl resources
 * param : void *curl: the curl instance to stop
 * Return : None
 * */
void doStopDownload(void *curl)
{
    CURL *curl_dest;
    if (curl != NULL) {
        curl_dest = (CURL *)curl;
        SWLOG_INFO("%s : CURL: free resources\n", __FUNCTION__);
        urlHelperDestroyCurl(curl_dest);
    }
}
/* doInteruptDwnl(): Stop the download and start again
 * param : in_curl: curl instance
 * param :max_dwnl_speed : Speed value. 0 means full speed
 * return :int
 * */
int doInteruptDwnl(void *in_curl, unsigned int max_dwnl_speed)
{
    CURLcode ret_code = CURLE_OK;
    CURL *curl = NULL;

    if (in_curl != NULL) {
        curl = (CURL *)in_curl;
        if (max_dwnl_speed > 0) {
            ret_code = curl_easy_pause(curl, CURLPAUSE_ALL);
    	    if(ret_code != CURLE_OK) {
                SWLOG_ERROR("%s : CURL: curl_easy_pause Failed\n", __FUNCTION__);
                return DWNL_FAIL;
    	    } else {
                SWLOG_INFO("%s : CURL: curl_easy_pause Success\n", __FUNCTION__);
            }
    	    ret_code = setThrottleMode(curl, (curl_off_t) max_dwnl_speed);
    	    if(ret_code != CURLE_OK) {
                SWLOG_ERROR("%s : CURL: setThrottleMode Failed:%d\n", __FUNCTION__, ret_code);
    	    } else {
                SWLOG_INFO("%s : CURL: setThrottleMode Success:%u\n", __FUNCTION__, max_dwnl_speed);
            }
            ret_code = curl_easy_pause(curl, CURLPAUSE_CONT);
    	    if(ret_code != CURLE_OK) {
                SWLOG_ERROR("%s : CURL: curl_easy_unpause Failed: %d\n", __FUNCTION__, ret_code);
                return DWNL_UNPAUSE_FAIL;
    	    } else {
                SWLOG_INFO("%s : CURL: curl_easy_unpause Success\n", __FUNCTION__);
            }
        }
    }
    return ret_code;
}
/* doGetDwnlBytes(): Get download bytes completed
 * param : in_curl: curl instance
 * Return :unsigned int : bytes downloaded
 * */
unsigned int doGetDwnlBytes(void *in_curl)
{
    unsigned int bytes = 0;
    CURLcode ret_code = CURLE_OK;
    CURL *curl = NULL;
    curl_off_t dl = 0;
    if (in_curl != NULL) {
        curl = (CURL *)in_curl;
	ret_code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD_T, &dl);
	if (ret_code == CURLE_OK) {
            SWLOG_INFO("%s : CURL: curl_easy_getinfo Success:%llu\n", __FUNCTION__, dl);
	    if (dl > 0) {
                bytes = dl;
                SWLOG_INFO("%s : CURL: Downloaded bytes:%u\n", __FUNCTION__, bytes);
	    }
	}else {
            SWLOG_ERROR("%s : CURL: curl_easy_getinfo failed: %d\n", __FUNCTION__, ret_code);
	}
    } else {
        SWLOG_ERROR("%s : CURL: curl instance is NULL\n", __FUNCTION__);
    }
    return bytes;
}
/* doCurlPutRequest(): Use for curl put request
 * in_curl : curl pointer which is return type from curl_init call.
 * pfile_dwnl : Structure pointer contains post fields, url, download path, chunkdownload retry, sslverify status
 * jsonrpc_auth_token : Hold token to communicate with json rpc
 * out_httpCode : Send back http status.
 * Return :curl_ret_status : Send back curl status
 * NOTE: TODO THIS FUNCTION NEED TO MODIFY FUTURE FOR MAKE MORE GENERIC
 * */
int doCurlPutRequest(void *in_curl, FileDwnl_t *pfile_dwnl, char *jsonrpc_auth_token, int *out_httpCode)
{
    CURL *curl;
    CURLcode ret_code = CURLE_OK;
    struct curl_slist *slist = NULL;
    //size_t byte_dwnled = 0;
    CURLcode curl_status = -1;
    int ret = -1;

    if (in_curl == NULL || out_httpCode == NULL || pfile_dwnl == NULL) {
        SWLOG_ERROR("%s: Parameter Check Fail\n", __FUNCTION__);
        return DWNL_FAIL;
    }
    curl = (CURL *)in_curl;
    ret_code = setCommonCurlOpt(curl, pfile_dwnl->url, pfile_dwnl->pPostFields, false);
    if(ret_code != CURLE_OK) {
        SWLOG_ERROR("%s : CURL: setCommonCurlOpt Failed\n", __FUNCTION__);
        return DWNL_FAIL;
    } else {
        SWLOG_INFO("%s : CURL: setCommonCurlOpt Success\n", __FUNCTION__);
    }
    /*Adding header and token into the single link list provided by curl lib*/
    if (pfile_dwnl->pHeaderData && *pfile_dwnl->pHeaderData) {
        slist = curl_slist_append( slist, pfile_dwnl->pHeaderData );
        if (slist != NULL) {
            if (jsonrpc_auth_token != NULL) {
	        SWLOG_INFO("%s : CURL: Setting For jsonrpc_auth_token\n", __FUNCTION__);
	        slist = curl_slist_append(slist, jsonrpc_auth_token);
	        if (slist == NULL) {
	            SWLOG_ERROR("%s : CURL: curl_slist_append fail for jsonrpc_auth_token set\n", __FUNCTION__);
	        }
            }
        }else {
            SWLOG_ERROR("%s : CURL: curl_slist_append header fail\n", __FUNCTION__);
	    return DWNL_FAIL;
        }
    }else {
        SWLOG_ERROR("%s : CURL: header field is empty\n", __FUNCTION__);
	return DWNL_FAIL;
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    if (ret_code != CURLE_OK) {
        SWLOG_ERROR( "SetRequestHeaders: CURLOPT_HTTPHEADER failed:%s\n", curl_easy_strerror(ret_code));
        if( slist != NULL ) {
	    curl_slist_free_all( slist );
        }
	return DWNL_FAIL;
    }
    //byte_dwnled = urlHelperDownloadToMem(curl, pfile_dwnl, out_httpCode, &curl_status);
    ret = urlHelperPutReuqest(curl, NULL, out_httpCode, &curl_status); 
    SWLOG_INFO("%s : urlHelperPutReuqest ret=%d\n", __FUNCTION__, ret);
    if( slist != NULL ) {
        curl_slist_free_all( slist );
    }
    return (int)curl_status;
}
/* getJsonRpcData(): Use for http download with out mtls
 * in_curl : curl pointer which is return type from curl_init call.
 * pfile_dwnl : Structure pointer contains post fields, url, download path, chunkdownload retry, sslverify status
 * jsonrpc_auth_token : Hold token to communicate with json rpc
 * out_httpCode : Send back http status.
 * Return :curl_ret_status : Send back curl status
 * */
int getJsonRpcData(void *in_curl, FileDwnl_t *pfile_dwnl, char *jsonrpc_auth_token, int *out_httpCode )
{
    CURL *curl;
    CURLcode ret_code = CURLE_OK;
    struct curl_slist *slist = NULL;
    size_t byte_dwnled = 0;
    CURLcode curl_status = -1;

    if (in_curl == NULL || out_httpCode == NULL || pfile_dwnl == NULL) {
        SWLOG_ERROR("%s: Parameter Check Fail\n", __FUNCTION__);
        return DWNL_FAIL;
    }
    curl = (CURL *)in_curl;
    ret_code = setCommonCurlOpt(curl, pfile_dwnl->url, pfile_dwnl->pPostFields, false);
    if(ret_code != CURLE_OK) {
        SWLOG_ERROR("%s : CURL: setCommonCurlOpt Failed\n", __FUNCTION__);
        return DWNL_FAIL;
    }
    /*Adding header and token into the single link list provided by curl lib*/
    if (pfile_dwnl->pHeaderData && *pfile_dwnl->pHeaderData) {
        slist = curl_slist_append( slist, pfile_dwnl->pHeaderData );
        if (slist != NULL) {
            if (jsonrpc_auth_token != NULL) {
                SWLOG_INFO("%s : CURL: Setting For jsonrpc_auth_token\n", __FUNCTION__);
                slist = curl_slist_append(slist, jsonrpc_auth_token);
                if (slist == NULL) {
                    SWLOG_ERROR("%s : CURL: curl_slist_append fail for jsonrpc_auth_token set\n", __FUNCTION__);
                }
            }
        }else {
            SWLOG_ERROR("%s : CURL: curl_slist_append header fail\n", __FUNCTION__);
            return DWNL_FAIL;
        }
    }else {
        SWLOG_ERROR("%s : CURL: header field is empty\n", __FUNCTION__);
        return DWNL_FAIL;
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
    if (ret_code != CURLE_OK) {
        SWLOG_ERROR( "SetRequestHeaders: CURLOPT_HTTPHEADER failed:%s\n", curl_easy_strerror(ret_code));
        if( slist != NULL ) {
            curl_slist_free_all( slist );
        }
        return DWNL_FAIL;
    }
    byte_dwnled = urlHelperDownloadToMem(curl, pfile_dwnl, out_httpCode, &curl_status);
    SWLOG_INFO("%s : Bytes Downloaded=%u and curl ret status=%d and http code=%d\n", __FUNCTION__, byte_dwnled, curl_status, *out_httpCode);
    SWLOG_INFO("%s : data received =%s\n", __FUNCTION__, (char *)pfile_dwnl->pDlData->pvOut);

    if( slist != NULL ) {
        curl_slist_free_all( slist );
    }
    return (int)curl_status;
}
/* doHttpFileDownload(): Use for http download with out mtls
 * in_curl : curl pointer which is return type from curl_init call.
 * pfile_dwnl : Structure pointer contains post fields, url, download path, chunkdownload retry, sslverify status 
 * auth : Structure contains certificate and key, NULL if not required
 * max_dwnl_speed : Restrict download speed.
 * dnl_start_pos : Use for chunk Download if it is NULL in that case chunkdown load option not set
 * out_httpCode : Send back http status.
 * Return :curl_ret_status : Send back curl status
 * */
int doHttpFileDownload(void *in_curl, FileDwnl_t *pfile_dwnl, MtlsAuth_t *auth, unsigned int max_dwnl_speed, char *dnl_start_pos, int *out_httpCode )
{
    CURL *curl;
    CURLcode ret_code = CURLE_OK;
    size_t byte_dwnled = 0;
    CURLcode curl_status = -1;
#ifdef CURL_DEBUG
    DbgData_t verbosinfo;
    memset(&verbosinfo, '\0', sizeof(DbgData_t));
#endif

    if (in_curl == NULL || pfile_dwnl == NULL || out_httpCode == NULL) {
        SWLOG_ERROR("%s: Parameter Check Fail\n", __FUNCTION__);
        return DWNL_FAIL;
    }
    curl = (CURL *)in_curl;

    ret_code = setCommonCurlOpt(curl, pfile_dwnl->url, pfile_dwnl->pPostFields, pfile_dwnl->sslverify);
    if(ret_code != CURLE_OK) {
        SWLOG_ERROR("%s : CURL: setCommonCurlOpt Failed\n", __FUNCTION__);
        return DWNL_FAIL;
    } else {
        SWLOG_INFO("%s : CURL: setCommonCurlOpt Success\n", __FUNCTION__);
    }


    if( auth != NULL )
    {
        ret_code = setMtlsHeaders(curl, auth);
        if(ret_code != CURLE_OK) {
            SWLOG_ERROR("%s : CURL: setMtlsHeaders Failed\n", __FUNCTION__);
            return DWNL_FAIL;
        } else {
            SWLOG_INFO("%s : CURL: setMtlsHeaders Success\n", __FUNCTION__);
        }
    }

    if(pfile_dwnl->hashData != NULL)
    {
        // Set additional headers
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, pfile_dwnl->hashData->hashvalue);
        headers = curl_slist_append(headers, pfile_dwnl->hashData->hashtime);
        ret_code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        if (ret_code != CURLE_OK)
	{
            SWLOG_ERROR( "SetRequestHeaders: CURLOPT_HTTPHEADER failed:%s\n", curl_easy_strerror(ret_code));
            if( headers != NULL ) {
            curl_slist_free_all( headers );
            }
            return DWNL_FAIL;
        }
    }

    if (max_dwnl_speed > 0) {
    	ret_code = setThrottleMode(curl, (curl_off_t) max_dwnl_speed);
    	if(ret_code != CURLE_OK) {
            SWLOG_ERROR("%s : CURL: setThrottleMode Failed\n", __FUNCTION__);
    	    return DWNL_FAIL;
    	} else {
            SWLOG_INFO("%s : CURL: setThrottleMode Success\n", __FUNCTION__);
        }
    }
#ifdef CURL_DEBUG
    ret_code = setCurlDebugOpt(curl, &verbosinfo);
    if (ret_code != CURLE_OK) {
        SWLOG_ERROR("%s : CURL: Unable to Set Verbos\n", __FUNCTION__);
    } else {
        SWLOG_INFO("%s : CURL: Set Verbos Success\n", __FUNCTION__);
    }
#endif
    if( *pfile_dwnl->pathname )
    {
        byte_dwnled = urlHelperDownloadFile(curl, pfile_dwnl->pathname, dnl_start_pos, pfile_dwnl->chunk_dwnl_retry_time, out_httpCode, &curl_status);
    }
    else
    {
        byte_dwnled = urlHelperDownloadToMem(curl, pfile_dwnl, out_httpCode, &curl_status);
    }
    SWLOG_INFO("%s : After curl operation no of bytes Downloaded=%u and curl ret status=%d and http code=%d\n", __FUNCTION__, byte_dwnled, curl_status, *out_httpCode);

#ifdef CURL_DEBUG
    if (verbosinfo.verboslog) {
        fflush(verbosinfo.verboslog);
        fclose(verbosinfo.verboslog);
    }
#endif    
    return (int)curl_status;
}

/* doAuthHttpFileDownload(): Use for codebig download
 * pfile_dwnl : Structure pointer contains post fields, url, download path, chunkdownload retry, sslverify status
 * auth : Structure contains certificate and key
 * out_httpCode : Send back http status.
 * Return :curl_ret_status : Send back curl status
 * */
int doAuthHttpFileDownload(void *in_curl, FileDwnl_t *pfile_dwnl, int *out_httpCode ){
    CURL *curl;
    CURLcode ret_code = CURLE_OK;
    struct curl_slist *slist = NULL;
    size_t byte_dwnled = 0;
    CURLcode curl_status = -1;

#ifdef CURL_DEBUG
    DbgData_t verbosinfo;
    memset(&verbosinfo, '\0', sizeof(DbgData_t));
#endif

    if (in_curl == NULL || pfile_dwnl == NULL || out_httpCode == NULL) {
        SWLOG_ERROR("%s: Parameter Check Fail\n", __FUNCTION__);
        return DWNL_FAIL;
    }
    curl = (CURL *)in_curl;
    ret_code = setCommonCurlOpt(curl, pfile_dwnl->url, pfile_dwnl->pPostFields, pfile_dwnl->sslverify);
    if(ret_code != CURLE_OK) {
        SWLOG_ERROR("%s : CURL: setCommonCurlOpt Failed\n", __FUNCTION__);
        return DWNL_FAIL;
    } else {
        SWLOG_INFO("%s : CURL: setCommonCurlOpt Success\n", __FUNCTION__);
    }
    if( pfile_dwnl->pHeaderData != NULL && *pfile_dwnl->pHeaderData ) {
        SWLOG_INFO("%s : CURL: Going to Set RequestHeaders\n", __FUNCTION__);
    	slist = SetRequestHeaders(curl, slist, pfile_dwnl->pHeaderData);
    	if(slist == NULL) {
            SWLOG_ERROR("%s : CURL: SetRequestHeaders Failed\n", __FUNCTION__);
            return DWNL_FAIL;
    	} else {
            SWLOG_INFO("%s : CURL: SetRequestHeaders Success\n", __FUNCTION__);
    	}
    }
#ifdef CURL_DEBUG
    ret_code = setCurlDebugOpt(curl, &verbosinfo);
    if (ret_code != CURLE_OK) {
        SWLOG_ERROR("%s : CURL: Unable to Set Verbos\n", __FUNCTION__);
    } else {
        SWLOG_INFO("%s : CURL: Set Verbos Success\n", __FUNCTION__);
    }
#endif
    if( *pfile_dwnl->pathname )
    {
        byte_dwnled = urlHelperDownloadFile(curl, pfile_dwnl->pathname, NULL, 0, out_httpCode, &curl_status);
    }
    else
    {
        byte_dwnled = urlHelperDownloadToMem(curl, pfile_dwnl, out_httpCode, &curl_status);
    }
    SWLOG_INFO("%s : After curl operation no of bytes Downloaded=%u and curl ret status=%d and http code=%d\n", __FUNCTION__, byte_dwnled, curl_status, *out_httpCode);
    if( slist != NULL ) {
        curl_slist_free_all( slist );
    }
#ifdef CURL_DEBUG
    if (verbosinfo.verboslog) {
        fflush(verbosinfo.verboslog);
        fclose(verbosinfo.verboslog);
    }
#endif    
    return (int)curl_status;
}
