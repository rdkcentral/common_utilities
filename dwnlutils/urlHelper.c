/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.Forg/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "urlHelper.h"

#include <sys/stat.h>
#include <pthread.h>
#include <sys/time.h>

#include "rdkv_cdl_log_wrapper.h"

#define DEFAULT_CONN_IDLE_SECS  118
#define TLSVERSION     CURL_SSLVERSION_TLSv1_2

pthread_once_t initOnce = PTHREAD_ONCE_INIT;

static long performRequest(CURL *curl, CURLcode *curl_code);
/*Use for forcefully stop download */
static int force_stop = 0;

/**
 * Auto initializer: called by pthread_once
 */
static void urlHelperInit(void) {
    curl_global_init(CURL_GLOBAL_ALL);
}

/* Creat or initialize curl request */
CURL *urlHelperCreateCurl(void) {
    pthread_once(&initOnce, urlHelperInit);
    return curl_easy_init();
}

/* Destroy curl */
void urlHelperDestroyCurl(CURL *ctx) {
    if(ctx != NULL) {
        curl_easy_cleanup(ctx);
        curl_global_cleanup();
    }
}
/*Description: Use for setting the force_stop variable. This function should call
 *             from application side.
 * @param: int: value to assign force_stop variable
 * @return: void 
 * */
int setForceStop(int value)
{
    COMMONUTILITIES_INFO("setForceStop(): rcv value=%d\n", value);
    force_stop = value;
    COMMONUTILITIES_INFO("setForceStop(): set force_stop=%d\n", force_stop);
    return 0;
}
/* performRequest(): Use for sending curl request and receive data
 * curl : server url
 * curl_ret_status: parameter used for returning curl status
 * */
static long performRequest(CURL *curl, CURLcode *curl_ret_status) {
    long httpCode = 0;
    char *serverurl = NULL;
    char *ip_addr = NULL;
    long port = 0;
    if(curl == NULL || curl_ret_status == NULL) {
        COMMONUTILITIES_ERROR("performRequest() parameter is NULL\n");
        return 0;
    }

    CURLcode curlcode = curl_easy_perform(curl);

    /* This code is only emitted when mTLS is enabled and the client cert is invalid */
    if(curlcode == CURLE_SSL_CERTPROBLEM) {
        COMMONUTILITIES_ERROR("cURL could not use mTLS certificate\n");
    }

    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &serverurl);
    curl_easy_getinfo(curl, CURLINFO_PRIMARY_IP, &ip_addr);
    curl_easy_getinfo(curl, CURLINFO_PRIMARY_PORT, &port);

    COMMONUTILITIES_INFO("Curl Connected to %s (%s) port %ld\n", serverurl, ip_addr, port);
    COMMONUTILITIES_INFO("Curl return code =%d, http code=%ld\n", curlcode, httpCode);

    if(curlcode != CURLE_OK) {
        COMMONUTILITIES_INFO("Error performing HTTP request. HTTP status: [%ld]; Error code: [%d][%s]\n", httpCode, curlcode, curl_easy_strerror(curlcode));
        switch(curlcode) {
            case CURLE_COULDNT_CONNECT:
            case CURLE_COULDNT_RESOLVE_HOST:
            case CURLE_COULDNT_RESOLVE_PROXY:
            case CURLE_BAD_DOWNLOAD_RESUME:
            case CURLE_INTERFACE_FAILED:
            case CURLE_GOT_NOTHING:
            case CURLE_NO_CONNECTION_AVAILABLE:
            case CURLE_OPERATION_TIMEDOUT:
            case CURLE_PARTIAL_FILE:
            case CURLE_READ_ERROR:
            case CURLE_RECV_ERROR:
            case CURLE_SEND_ERROR:
            case CURLE_SEND_FAIL_REWIND:
            case CURLE_SSL_CONNECT_ERROR:
            case CURLE_UPLOAD_FAILED:
            case CURLE_WRITE_ERROR:
                COMMONUTILITIES_ERROR("Reporting network connectivity concerns.\n");
                break;
            default:
                break;
        }

    }
    *curl_ret_status = curlcode;
    COMMONUTILITIES_INFO("In performRequest curl_ret_status =%d\n", *curl_ret_status);
    return httpCode;
}

/* urlHelperPutReuqest(): Use for curl put request
 * curl : curl pointer which is return type from curl_init call.
 * upDate : FUTURE US
 * out_httpCode : Send back http status.
 * Return :curl_ret_status : Send back curl status
 * NOTE: TODO THIS FUNCTION NEED TO MODIFY FUTURE FOR MAKE MORE GENERIC
 * */

int urlHelperPutReuqest(CURL *curl, void *upData, int *httpCode_ret_status, CURLcode *curl_ret_status)
{
    int ret = -1;
    CURLcode curl_ret = -1;
    char postdata[2] = "";

    if (curl == NULL || httpCode_ret_status == NULL || curl_ret_status == NULL) {
        COMMONUTILITIES_ERROR("%s: parameter is NULL\n", __FUNCTION__);
	return ret;
    }
    curl_ret = curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    if (curl_ret == CURLE_OK) {
        COMMONUTILITIES_INFO("%s: enable CURLOPT_UPLOAD1 success\n", __FUNCTION__);
    }else {
        COMMONUTILITIES_ERROR("%s: enable CURLOPT_UPLOAD1 fail\n", __FUNCTION__);
    }
    curl_ret = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
    if (curl_ret == CURLE_OK) {
        COMMONUTILITIES_INFO("%s: CURLOPT_POSTFIELDS success\n", __FUNCTION__);
    }else {
        COMMONUTILITIES_ERROR("%s: CURLOPT_POSTFIELDS fail\n", __FUNCTION__);
    }
    *httpCode_ret_status = performRequest(curl, curl_ret_status);
    ret = 0;
    return ret;
}

/* Description: Print curl error message and retun same message inside pointer
 * @param: curl_ret_code : curl perform return code
 * @return: char *: Pointer to Error message
 * */
char *printCurlError(int curl_ret_code) {
    const char *error_msg = NULL;
    CURLcode curl_code = (CURLcode)curl_ret_code;
    error_msg = curl_easy_strerror(curl_code);
    COMMONUTILITIES_INFO("%s: curl_easy_strerror =%s\n", __FUNCTION__, curl_easy_strerror(curl_code));
    return (char *)error_msg;
}

/* Description: Setting cert file and key
 * certFile: Certificate file name.
 * curl : curl object use for setting curl option
 * pPasswd: Key or password
 * */
CURLcode setMtlsHeaders(CURL *curl, MtlsAuth_t *sec) {
    CURLcode code = -1;
    if(sec == NULL) {
        COMMONUTILITIES_ERROR("%s: parameter is NULL\n", __FUNCTION__);
        return code;
    }
    COMMONUTILITIES_INFO("%s: certfile:%s:cert type:%s\n", __FUNCTION__, sec->cert_name, sec->cert_type);

#ifdef LIBRDKCERTSELECTOR
    COMMONUTILITIES_INFO("%s: engine type:%s\n", __FUNCTION__, sec->engine);
    if (sec->engine[0] == '\0') {
        code = curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
    }else{
        code = curl_easy_setopt(curl, CURLOPT_SSLENGINE, sec->engine);
    }
#else
    code = curl_easy_setopt(curl, CURLOPT_SSLENGINE_DEFAULT, 1L);
#endif	
    if(code != CURLE_OK) {
        COMMONUTILITIES_ERROR("%s : Curl CURLOPT_SSLENGINE_DEFAULT failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
    }
    do {
    if((strcmp(sec->cert_type, "P12")) == 0) {
        COMMONUTILITIES_INFO("%s : set certfile:%s:paswd:<> and type:%s\n", __FUNCTION__, sec->cert_name, sec->cert_type);
        code = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, sec->cert_type);
        if(code != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s : Curl CURLOPT_SSLCERTTYPE P12 failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
	    break;
        }
        /* set the cert for client authentication */
        code = curl_easy_setopt(curl, CURLOPT_SSLCERT, sec->cert_name);
        if(code != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s : Curl CURLOPT_SSLCERT failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
	    break;
        }
        code = curl_easy_setopt(curl, CURLOPT_KEYPASSWD, sec->key_pas);
        if(code != CURLE_OK) {
            COMMONUTILITIES_ERROR("%s : Curl CURLOPT_KEYPASSWD failed with error %s \n", __FUNCTION__, curl_easy_strerror(code));
	    break;
        }
    }else {
        COMMONUTILITIES_INFO(" Going to set PEM certfile:%s:key:%s and cert type:%s\n", sec->cert_name, sec->key_pas, sec->cert_type);
        code = curl_easy_setopt(curl, CURLOPT_SSLCERTTYPE, "PEM");
        if(code != CURLE_OK) {
            COMMONUTILITIES_INFO("%s : Curl CURLOPT_SSLCERTTYPE failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
	    break;
        }
        /* set the cert for client authentication */
        code = curl_easy_setopt(curl, CURLOPT_SSLCERT, sec->cert_name);
        if(code != CURLE_OK) {
            COMMONUTILITIES_INFO("%s : Curl CURLOPT_SSLCERT failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
	    break;
        }
        code = curl_easy_setopt(curl, CURLOPT_SSLKEY, sec->key_pas);
        if(code != CURLE_OK) {
            COMMONUTILITIES_INFO("%s : Curl CURLOPT_SSLKEY failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
	    break;
        }

    }
    /* disconnect if we cannot authenticate */
    code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    if(code != CURLE_OK) {
        COMMONUTILITIES_ERROR("%s : Curl CURLOPT_SSL_VERIFYPEER failed with error %s\n", __FUNCTION__, curl_easy_strerror(code));
	break;
    }
    } while(0);
    COMMONUTILITIES_ERROR("%s : Curl Return Code = %d\n", __FUNCTION__, code);
    return code;
}

/*
 * This is Call back function which is called continuesly at the
 * time of data tranfer. This function will write curl progress data inside file
 * */
static int xferinfo(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    struct curlprogress *myp = (struct curlprogress *) p;
    //CURL *curl = myp->curl;
    //curl_off_t curtime = 0;
    if(myp->prog_store == NULL) {
        myp->prog_store = stdout;
    }

    /*curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &curtime);

     myp->lastruntime = curtime;
     fprintf(myp->prog_store, "TOTAL TIME: %lu.%06lu\n",
     (unsigned long)(curtime / 1000000),
     (unsigned long)(curtime % 1000000));*/

    fprintf(myp->prog_store, "UP: %lu of %lu  DOWN: %lu of %lu\n", (unsigned long) ulnow, (unsigned long) ultotal, (unsigned long) dlnow,
            (unsigned long) dltotal);
    fseek(myp->prog_store, 0, SEEK_SET);
    return 0;
}

/*
 * This is Call back function which is called continuesly at the
 * time of data tranfer. This function will write requested file data inside file
 * */
static size_t download_func(void* ptr, size_t size, size_t nmemb, void* stream) {
    DownloadData* data = stream;
    size_t written;
    /*This logic is use for forcefully stop downlaod when Throttle mode is set to
     * background and throttle speed rfc is set to zero. Here if we return zero curl
     * lib will return 23 error code */
    if (force_stop == 1) {
        COMMONUTILITIES_INFO("download_func Stopping Download\n");
        return 0;
    }

    written = fwrite(ptr, size, nmemb, (FILE *)data->pvOut);

    /* fwrite returns the number of "items" (as in the number of 'nmemb').
     * Thus to get the correct number of bytes we must multiply by the size.
     */
    data->datasize += written * size;

    return written;
}

static size_t WriteMemoryCB( void *pvContents, size_t szOneContent, size_t numContentItems, void *userp )
{
  size_t numBytes = numContentItems * szOneContent;         // num bytes is how big this data block is
  DownloadData* pdata = (DownloadData*)userp;
  char *ptr;

  if( (pdata->datasize + numBytes) >= pdata->memsize )     // if new data plus what we currently have stored >= current mem alloc
  {
      COMMONUTILITIES_INFO( "WriteMemoryCB: reallocating %zu bytes, pdata->pvOut = 0x%p\n", pdata->datasize + numBytes + 1, pdata->pvOut );
      ptr = realloc( pdata->pvOut, pdata->datasize + numBytes + 1 );     // current data size plus new data size plus 1 byte for NULL
      if( ptr != NULL )
      {
          pdata->memsize = pdata->datasize + numBytes + 1;              // memsize now reflects the new allocation
          pdata->pvOut = ptr;                                           // pvOut is points to reallocation region
      }
      else
      {
          COMMONUTILITIES_ERROR( "WriteMemoryCB: realloc failed to find additional memory\n");
          numBytes = 0;
      }
  }
  else
  {
      ptr = (char*)pdata->pvOut;            // otherwise point to already allocated memory
  }
  
  if( ptr != NULL )
  {
      memcpy( (ptr + pdata->datasize), pvContents, numBytes );      // append new data to end of existing
      pdata->datasize += numBytes;                                  // update number of bytes in buffer
      *(ptr + pdata->datasize) = 0;                                 // append NULL terminator to buffer
  }
 
  return numBytes;
}

/*
 * This is Call back function which is called before data transfer start.
 * Which is stores curl request header data.
 * */
static size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    FILE *fp = userdata;
    if(fp != NULL && buffer != NULL) {
        COMMONUTILITIES_INFO("header_callback():=%s\n",buffer);
        fwrite(buffer, nitems, size, fp);
	fflush(fp);
    }else {
        COMMONUTILITIES_ERROR("Inside header_callback() Invalid file pointer");
    }
    return nitems * size;
}

/* urlHelperGetHeaderInfo(): Used for get curl request header data
 * url: Request server url
 * httpCode: Use for return http status to called function.
 * sec: This is structure which is used for receive certificate, key, password etc
 * pathname : Header data Save file name with path
 * curl_ret_status : Use for return curl status to called function.
 * */
int urlHelperGetHeaderInfo(const char* url, MtlsAuth_t *sec, const char* pathname, int* httpCode_ret_status, int *curl_ret_status) {
    CURLcode ret_code = CURLE_OK;
    FILE *headerfile = NULL;
    CURL *curl;

    if(url == NULL || httpCode_ret_status == NULL || pathname == NULL || curl_ret_status == NULL) {
        COMMONUTILITIES_ERROR("urlHelperGetHeaderInfo(): pathname not present or parameter is NULL\n");
        return -1;
    }else {
        COMMONUTILITIES_ERROR("urlHelperGetHeaderInfo(): pathname:%s\n", pathname);
    }
    curl = urlHelperCreateCurl();
    if(curl) {
        ret_code = curl_easy_setopt(curl, CURLOPT_URL, url);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: url set failed\n");
            urlHelperDestroyCurl(curl);
            return ret_code;
        }
        ret_code = curl_easy_setopt(curl, CURLOPT_SSLVERSION, TLSVERSION);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: CURLOPT_SSLVERSION set failed\n");
            urlHelperDestroyCurl(curl);
            return ret_code;
        }
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 600L); //CURL_TLS_TIMEOUT = 600L - 10Min
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); // connection timeout 30s
        if(sec != NULL) {
            setMtlsHeaders(curl, sec);
        }
        headerfile = fopen(pathname, "w");
        if(headerfile == NULL) {
            COMMONUTILITIES_ERROR("CURL: path=%s file unable to open\n", pathname);
            urlHelperDestroyCurl(curl);
            return -1;
        }
        ret_code = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: CURLOPT_HEADERFUNCTION set failed\n");
            urlHelperDestroyCurl(curl);
            fclose(headerfile);
            return ret_code;
        }
        /* Set CURLOPT_HEADERDATA to get header infor inside headerfile callback function */
        ret_code = curl_easy_setopt(curl, CURLOPT_HEADERDATA, headerfile);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: CURLOPT_HEADERDATA set failed\n");
            urlHelperDestroyCurl(curl);
            fclose(headerfile);
            return ret_code;
        }
        /* CURLOPT_NOBODY: If we set this option the curl request data will not tranfer */
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        *httpCode_ret_status = performRequest(curl, (CURLcode *)curl_ret_status); // Sending curl request
        fflush(headerfile);
        fclose(headerfile);
        urlHelperDestroyCurl(curl);
    }
    return 0;
}
/* setCommonCurlOpt(): Use for set curl option
 * curl : curl object
 * url: Server url
 * Return : Type is CURLcode. In case of  Success : CURLE_OK
 *                              Failure case -1
 * */
CURLcode setCommonCurlOpt(CURL *curl, const char *url, char *pPostFields, bool sslverify) {
    CURLcode ret_code = -1;

    if(curl == NULL || url == NULL) {
        COMMONUTILITIES_ERROR("setCommonCurlOpt(): curl parameter is NULL\n");
        return ret_code;
    }
#ifdef CURL_DEBUG
    COMMONUTILITIES_INFO("CURL: Going to set url: %s\n", url);
#endif
    ret_code = curl_easy_setopt(curl, CURLOPT_URL, url);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: url set failed\n");
        return ret_code;
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: CURLOPT_FOLLOWLOCATION failed\n");
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_SSLVERSION, TLSVERSION);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: CURLOPT_SSLVERSION failed\n");
        return ret_code;
    }
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TLS_TIMEOUT); // Total curl operation timeout CURL_TLS_TIMEOUT = 7200L - 7200 sec
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L); // connection timeout 30s
    if(sslverify == true) {
        ret_code = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYSTATUS, 1L);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: CURLOPT_SSL_VERIFYSTATUS failed msg:%s\n", curl_easy_strerror(ret_code));
            if(ret_code == CURLE_NOT_BUILT_IN) {
                COMMONUTILITIES_ERROR("CURL: CURLOPT_SSL_VERIFYSTATUS not enable at built time\n");
            }
        }
    }
    /* enable TCP keep-alive for this transfer */
    ret_code = curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    if (ret_code != CURLE_OK) {
	COMMONUTILITIES_ERROR( "CURL: CURLOPT_TCP_KEEPALIVE failed msg:%s\n", curl_easy_strerror(ret_code));
    }
    /* keep-alive idle time to 120 seconds */
    ret_code = curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    if (ret_code != CURLE_OK) {
	COMMONUTILITIES_ERROR( "CURL: CURLOPT_TCP_KEEPIDLE failed msg:%s\n", curl_easy_strerror(ret_code));
    }
    /* interval time between keep-alive probes: 60 seconds */
    ret_code = curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
    if (ret_code != CURLE_OK) {
	COMMONUTILITIES_ERROR( "CURL: CURLOPT_TCP_KEEPINTVL failed msg:%s\n", curl_easy_strerror(ret_code));
    }

    if( pPostFields != NULL )
    {
        ret_code = SetPostFields( curl, pPostFields );
    }
    else
    {
        ret_code = CURLE_OK;
    }
    return ret_code;
}


#ifdef CURL_DEBUG
CURLcode setCurlDebugOpt(CURL *curl, DbgData_t *debug)
{
	CURLcode ret_code = -1;
	COMMONUTILITIES_INFO("setCurlDebugOpt(): Setting Verbos mode\n");
	if (curl == NULL || debug == NULL) {
		COMMONUTILITIES_ERROR( "setCurlDebugOpt(): curl parameter is NULL\n");
		return ret_code;
	}
	debug->verboslog = fopen("/tmp/curl_verbos_data.txt", "w");
	if (debug->verboslog == NULL) {
		COMMONUTILITIES_ERROR( "setCurlDebugOpt(): file open failed So unable to get verbos data\n");
		return ret_code;
	}
	debug->trace_ascii = 1;
	ret_code = curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
	ret_code = curl_easy_setopt(curl, CURLOPT_DEBUGDATA, debug);
	ret_code = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	ret_code = CURLE_OK;
	return ret_code;
}
#endif

/* setCurlProgress(): Use for save curl progress data
 * curl : curl object
 * curl_progress: This is the structer which contains file name to store
 * 		  curl progress.
 * Return : Type is CURLcode. In case of  Success : CURLE_OK
 * 				Failure case -1
 * */
CURLcode setCurlProgress(CURL *curl, struct curlprogress *curl_progress) {
    CURLcode ret_code = -1;
    if(curl == NULL || curl_progress == NULL) {
        COMMONUTILITIES_INFO("setCurlProgress(): curl parameter is NULL\n");
        return ret_code;
    }
    curl_progress->prog_store = fopen(CURL_PROGRESS_FILE, "w");
    if(curl_progress->prog_store == NULL) {
        COMMONUTILITIES_ERROR("CURL:Failed to open %s file\n", CURL_PROGRESS_FILE);
        return ret_code;
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: CURLOPT_XFERINFOFUNCTION failed\n");
    }
    // pass the struct pointer into the xferinfo function
    ret_code = curl_easy_setopt(curl, CURLOPT_XFERINFODATA, curl_progress);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: CURLOPT_XFERINFODATA failed\n");
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: CURLOPT_NOPROGRESS failed\n");
    }
    ret_code = CURLE_OK;
    return ret_code;
}

/* Description: Use forset max receive speed
 * @param curl: curl object
 * @param max_dwnl_speed: Download speed 
 * @return : void
 * */

CURLcode setThrottleMode(CURL *curl, curl_off_t max_dwnl_speed) {
    CURLcode ret_code = -1;
    if(curl == NULL || max_dwnl_speed < 0) {
        COMMONUTILITIES_ERROR("%s : curl parameter is NULL or download speed is < 0\n", __FUNCTION__);
        return ret_code;
    }
    COMMONUTILITIES_INFO("CURL: CURLOPT_MAX_RECV_SPEED_LARGE set speed limit=%" CURL_FORMAT_CURL_OFF_T "\n", max_dwnl_speed);
    ret_code = curl_easy_setopt(curl, CURLOPT_MAX_RECV_SPEED_LARGE, max_dwnl_speed);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("%s: CURL: CURLOPT_MAX_RECV_SPEED_LARGE failed:%s\n", __FUNCTION__, curl_easy_strerror(ret_code));
        return ret_code;
    } else {
        COMMONUTILITIES_INFO("%s: CURL: CURLOPT_MAX_RECV_SPEED_LARGE Success:%s\n", __FUNCTION__, curl_easy_strerror(ret_code));
    }
    return ret_code;
}

/* Description: Use for close the file pointer.
 * @param data: Download file pointer store inside structure.
 * @param prog: Curl progress structure
 * @return : void
 * */
void closeFile(DownloadData *data, struct curlprogress *prog, FILE *fp) 
{
	if ((data != NULL) && (data->pvOut != NULL)) {
		COMMONUTILITIES_INFO( "CURL: CLOSE Data Download file\n");
		fclose((FILE*)data->pvOut);
	}
	if ((prog != NULL) && (prog->prog_store != NULL)) {
		COMMONUTILITIES_INFO( "CURL: CLOSE Curl Progress Bar file\n");
		fclose(prog->prog_store);
	}
	if (fp != NULL) {
		COMMONUTILITIES_INFO( "CURL: CLOSE Header Dump file\n");
		fclose(fp);
	}
}
/* urlHelperDownloadFile(): Use for download a file
 * curl : Curl Object
 * file : path with file name to download
 * dnl_start_pos : Use for chunk Download if it is NULL in that case request is Full Downlaod
 * httpCode_ret_status : Send back http status.
 * curl_ret_status : Send back curl status
 * Return Type size_t : Return no of bytes downloaded.
 * */
size_t urlHelperDownloadFile(CURL *curl, const char *file, char *dnl_start_pos, int chunk_dwnl_retry_time, 
    int *httpCode_ret_status, CURLcode *curl_ret_status) {

    DownloadData data;
    DownloadData *pData = &data;
    struct curlprogress prog; // Use for store curl progress
    data.datasize = 0;
    int seek_place = 0;
    int retry = 2;
    char file_pt_pos[32] = { 0 };
    char file_open_mode[6] = { 0 };
    int seek_ret = -1;
    CURLcode ret_code = -1;
    FILE *headerfile = NULL;
    char header_dump[128];

    if(curl == NULL || file == NULL || httpCode_ret_status == NULL || curl_ret_status == NULL) {
        COMMONUTILITIES_ERROR("urlHelperDownloadFile(): pathname not present or parameter is NULL\n");
        return 0;
    }else {
        COMMONUTILITIES_INFO("urlHelperDownloadFile(): pathname:%s\n", file);
    }

    if(dnl_start_pos == NULL) {
        strncpy(file_open_mode, "wb", sizeof(file_open_mode) - 1);
    }else {
        strncpy(file_open_mode, "rb+", sizeof(file_open_mode) - 1);
    }

    COMMONUTILITIES_INFO("urlHelperDownloadFile() download file name with path:%s and file open mode=%s\n", file, file_open_mode);
    data.pvOut = (void*)fopen(file, file_open_mode);
    if(data.pvOut == NULL) {
        COMMONUTILITIES_INFO("urlHelperDownloadFile(): File open Fail:%s\n", file);
        return 0;
    }
    /*If the download request is not chunk download then dump header information to separate file
     * if the request is chunkdownload then already header information file is present so no need to dump
     * again header information */
    if (dnl_start_pos == NULL) {
        snprintf(header_dump, sizeof(header_dump), "%s.header", file);
        COMMONUTILITIES_INFO("urlHelperDownloadFile(): Dump header info to file:%s\n", header_dump);
        headerfile = fopen(header_dump, "w");
        if(headerfile == NULL) {
            COMMONUTILITIES_ERROR("CURL: path=%s file unable to open\n", header_dump);
	    closeFile(pData, NULL, NULL);
            return 0;
        }
        ret_code = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: CURLOPT_HEADERFUNCTION set failed\n");
            closeFile(pData, NULL, headerfile);
            return ret_code;
        }
        /* Set CURLOPT_HEADERDATA to get header infor inside headerfile callback function */
        ret_code = curl_easy_setopt(curl, CURLOPT_HEADERDATA, headerfile);
        if(ret_code != CURLE_OK) {
            COMMONUTILITIES_ERROR("CURL: CURLOPT_HEADERDATA set failed\n");
            closeFile(pData, NULL, headerfile);
            return ret_code;
        }
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_func);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_INFO("CURL: CURLOPT_WRITEFUNCTION failed\n");
        closeFile(pData, NULL, headerfile);
        return ret_code;
    }
    ret_code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, pData);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_INFO("CURL: CURLOPT_WRITEDATA failed\n");
        closeFile(pData, NULL, headerfile);
        return ret_code;
    }
    ret_code = setCurlProgress(curl, &prog);
    if(ret_code != CURLE_OK) {
        COMMONUTILITIES_ERROR("CURL: Unable to set curl progress\n");
    }
    /* Below block is used for chunk download */
    if(dnl_start_pos != NULL) {
        seek_place = atoi(dnl_start_pos);
        COMMONUTILITIES_INFO("CURL: Chunk Download Operation Start=%d\n", seek_place);
        strncpy(file_pt_pos, dnl_start_pos, sizeof(file_pt_pos)-1);
        while(retry) {
            COMMONUTILITIES_INFO("CURL: file seek position=%d chunk start %s and %s\n", seek_place, dnl_start_pos, file_pt_pos);
            ret_code = curl_easy_setopt(curl, CURLOPT_RANGE, file_pt_pos);
            	if(ret_code != CURLE_OK) {
                    COMMONUTILITIES_ERROR("CURL: CURLOPT_RANGE failed msg:%s\n", curl_easy_strerror(ret_code));
		    closeFile(pData, &prog, headerfile);
		    return ret_code;
                }else {
                    seek_ret = fseek((FILE*)data.pvOut, seek_place, SEEK_SET);
		    if (seek_ret != 0) {
		    /* If file pointer seek fail return curl 33 error so
		     * full download should trigger */
			COMMONUTILITIES_ERROR( "CURL: fseek failed ret=%d\n", seek_ret);
			*httpCode_ret_status = 0;
			*curl_ret_status = 33;
			closeFile(pData, &prog, headerfile);
			return 0;
		     }
                }
                *httpCode_ret_status = performRequest(curl, curl_ret_status);
                 if((*curl_ret_status == 18) || (*curl_ret_status == 28)) {
                     seek_place = 0;
                     seek_place = ftell((FILE*)data.pvOut);
                     memset(file_pt_pos, '\0', sizeof(file_pt_pos));
                     sprintf(file_pt_pos, "%d-", seek_place);
                 }else if ((*curl_ret_status == 33) || (*curl_ret_status == 36)) {
		     COMMONUTILITIES_ERROR( "CURL: Received curl error=%d and go for full Download\n",*curl_ret_status);
		     break;	
		 } else if((*curl_ret_status == 0) && ((*httpCode_ret_status == 206) || (*httpCode_ret_status == 200))) {
                     COMMONUTILITIES_INFO("CURL: File Download Done curl ret=%d and http=%d\n", *curl_ret_status, *httpCode_ret_status);
                     break;
                 }else {
                      retry--;
                 }
                 if(chunk_dwnl_retry_time != 0) {
                     COMMONUTILITIES_INFO("CURL: Reboot flag is false. So Go to sleep For =%d sec\n", chunk_dwnl_retry_time);
                     sleep(chunk_dwnl_retry_time);
                 }
                    continue;
       }
    }else {
        COMMONUTILITIES_INFO("CURL:Download Operation Start\n");
        *httpCode_ret_status = performRequest(curl, curl_ret_status); // Sending curl request
    }
    /* Close Downloaded File */
    fflush((FILE*)data.pvOut);
    fclose((FILE*)data.pvOut);
    /* Close curl progess save file */
    if(prog.prog_store) {
        fflush(prog.prog_store);
        fclose(prog.prog_store);
    }
    if (headerfile != NULL) {
        fclose(headerfile);
    }
    COMMONUTILITIES_INFO("CURL:Download Operation Done. File data.datasize:%zu and curl code=%d\n", data.datasize, *curl_ret_status);
    return data.datasize;
}

/* urlHelperDownloadToMem(): Use to download data and store in dynamically allocated memory
 * curl : Curl Object
 * pMem : pointer to dynamically allocated memory where the data will be stored. Allocated memory will grow as needed
 * pszMem : pointer to the allocated memory size. If a reallocation occurs, this value is updated
 * httpCode_ret_status : Send back http status.
 * curl_ret_status : Send back curl status
 * Return Type size_t : Return no of bytes downloaded.
 * */
size_t urlHelperDownloadToMem( CURL *curl, FileDwnl_t *pfile_dwnl, int *httpCode_ret_status, CURLcode *curl_ret_status )
{
    CURLcode ret_code = -1;
    size_t len = 0;

    if( curl != NULL && pfile_dwnl != NULL && pfile_dwnl->pDlData != NULL && httpCode_ret_status != NULL && curl_ret_status != NULL )
    {
        *((char *)pfile_dwnl->pDlData->pvOut) = 0;
        pfile_dwnl->pDlData->datasize = 0;
        
	if(pfile_dwnl->pDlHeaderData != NULL)
	{
            *((char *)pfile_dwnl->pDlHeaderData->pvOut) = 0;
            pfile_dwnl->pDlHeaderData->datasize = 0;
	    
	    ret_code = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, WriteMemoryCB);
	    if( ret_code == CURLE_OK )
	    {
	        curl_easy_setopt(curl, CURLOPT_HEADERDATA, pfile_dwnl->pDlHeaderData);
                if( ret_code == CURLE_OK )
	        {
                    COMMONUTILITIES_INFO("urlHelperDownloadToMem: Header Data Request Set\n");
	        }
	        else
	        {
                     COMMONUTILITIES_ERROR("urlHelperDownloadToMem: CURLOPT_WRITEFUNCTION failed\n");
	        }
	     }
	     else
	     {
                 COMMONUTILITIES_ERROR("urlHelperDownloadToMem: CURLOPT_WRITEFUNCTION failed\n");
	     }
	}

        ret_code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCB);
        if( ret_code == CURLE_OK )
        {
            ret_code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, pfile_dwnl->pDlData);
            if( ret_code == CURLE_OK )
            {
               *httpCode_ret_status = performRequest(curl, curl_ret_status); // Sending curl request
            }
            else
	    {
                COMMONUTILITIES_ERROR("urlHelperDownloadToMem: CURLOPT_WRITEDATA failed\n");
                *httpCode_ret_status = 0;
            }
         }
         else
         {
             COMMONUTILITIES_ERROR("urlHelperDownloadToMem: CURLOPT_WRITEFUNCTION failed\n");
             *httpCode_ret_status = 0;
         }
         len = pfile_dwnl->pDlData->datasize;
    }

    return len;
}

struct curl_slist* SetRequestHeaders( CURL *curl, struct curl_slist *pslist, char *pHeader )
{
    CURLcode ret_code  = CURLE_OK; 

    if( (pHeader != NULL) && (curl != NULL)  )
    {
        pslist = curl_slist_append( pslist, pHeader );
        // try to set headers in below block. 
        if( pslist != NULL )
        {
            ret_code = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, pslist);
            if (ret_code != CURLE_OK)
            {
                COMMONUTILITIES_ERROR( "SetRequestHeaders: CURLOPT_HTTPHEADER failed:%s\n", curl_easy_strerror(ret_code));
            }
        }
        else
        {
            COMMONUTILITIES_INFO("SetRequestHeaders: pslist Empty!!\n");
        }
    }
    else
    {
        COMMONUTILITIES_INFO("SetRequestHeaders: Input Empty!!\n");
    }
    return pslist;
}

CURLcode SetPostFields( CURL *curl, char *pPostFields )
{
    CURLcode ret_code  = CURLE_OK;

    if( curl != NULL && pPostFields != NULL )
    {
        ret_code = curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE, (long)strlen(pPostFields) );
        if( ret_code == CURLE_OK )
        {
            ret_code = curl_easy_setopt( curl, CURLOPT_POSTFIELDS, pPostFields );
            if (ret_code != CURLE_OK)
            {
                COMMONUTILITIES_ERROR( "SetPostFields: CURLOPT_POSTFIELDS failed:%s\n", curl_easy_strerror(ret_code));
            }
        }
        else
        {
            COMMONUTILITIES_ERROR( "SetPostFields: CURLOPT_POSTFIELDSIZE failed:%s\n", curl_easy_strerror(ret_code));
        }
    }
    else
    {
        COMMONUTILITIES_ERROR("SetPostFields: Input Empty!!\n");
    }
    return ret_code;
}

int allocDowndLoadDataMem( DownloadData *pDwnData, int szDataSize )
{
    void *ptr;
    int iRet = 1;

    if( pDwnData != NULL )
    {
        pDwnData->datasize = 0;
        ptr = malloc( szDataSize );
        pDwnData->pvOut = ptr;
        pDwnData->datasize = 0;
        if( ptr != NULL )
        {
            pDwnData->memsize = szDataSize;
            *(char *)ptr = 0;
            iRet = 0;
        }
        else
        {
            pDwnData->memsize = 0;
            COMMONUTILITIES_ERROR("allocDowndLoadDataMem: Failed to allocate memory for XCONF download\n");
        }
    }
    return iRet;
}

bool checkDeviceInternetConnection(long timeout_ms)
{
	bool status = false;
	CURL *curl;
    CURLcode res;
    char *url = "http://xfinity.com";

    // Initialize the curl session
    curl = curl_easy_init();
    if(curl) {
        // Set the URL to retrieve
        curl_easy_setopt(curl, CURLOPT_URL, url);

        // Set private data and custom headers
        curl_easy_setopt(curl, CURLOPT_PRIVATE,url);
        struct curl_slist *chunk = NULL;
        chunk = curl_slist_append(chunk, "Cache-Control: no-cache, no-store");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

        // Set user agent
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "RDKCaptiveCheck/1.0");

        // Set request type to GET
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);

        // Set write callback function
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunction);

        // Set timeout in milliseconds
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);

        // Make the request
        res = curl_easy_perform(curl);

        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
		
        // Check for errors
        if(res != CURLE_OK)
		    COMMONUTILITIES_ERROR( "curl_easy_perform() failed: %s\n\n",curl_easy_strerror(res));

        // Clean up the curl session
        curl_easy_cleanup(curl);
        curl_slist_free_all(chunk); // Free the custom headers list
		
		if(http_code == 302)
		{
			status = true;
		}
    }
    return status;
}

// Define your write callback function
size_t writeFunction(void *contents, size_t size, size_t nmemb, void *userp)
{
    (void)userp; // Unused

    return size * nmemb;
}

#ifdef GTEST_ENABLE
long (*getperformRequest(void)) (CURL *, CURLcode *) {
	return &performRequest;
}

int (*getxferinfo(void)) (void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
        return &xferinfo;
}

size_t (*getdownload_func(void)) (void* ptr, size_t size, size_t nmemb, void* stream) {
	return &download_func;
}

size_t (*getWriteMemoryCB(void)) ( void *pvContents, size_t szOneContent, size_t numContentItems, void *userp ) {
	return &WriteMemoryCB;
}

size_t (*getheader_callback(void)) (char *buffer, size_t size, size_t nitems, void *userdata) {
	return &header_callback;
}
#endif
