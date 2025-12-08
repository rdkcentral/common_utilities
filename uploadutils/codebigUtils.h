/**
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


#ifndef  _CODEBIG_DL_H_
#define  _CODEBIG_DL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdk_fwdl_utils.h"
//#include "rfcapi.h"

#define MAX_DIR_LEN     256
#define MAX_HEADER_LEN  512
#define MAX_FMT_LEN     32

#define INVALID_SERVICE 0
#define SSR_SERVICE     1
#define XCONF_SERVICE   2
#define CIXCONF_SERVICE 4
#define DAC15_SERVICE   14
#define HTTP_XCONF_CODEBIG 3

#define memset_s(dst,max_1,c,max) \
memset(dst,c,max)

#define SWUPDATE_CONF           "/opt/swupdate.conf"
#define RFC_VALUE_BUF_SIZE       512
#define READ_RFC_SUCCESS         1
#define READ_RFC_FAILURE        -1
#define DIRECT_BLOCK_FILENAME   "/tmp/.lastdirectfail_cdl"
#define HTTP_SSR_CODEBIG         1
#define CB_BLOCK_FILENAME        "/tmp/.lastcodebigfail_cdl"
#define URL_MAX_LEN              512

#define MAX_PARAM_LEN     (2*1024)

typedef enum
{
  SUCCESS=0,
  FAILURE,
  NONE,
  EMPTY
}DATATYPE;

typedef enum
{
    WDMP_SUCCESS = 0,                    /**< Success. */
    WDMP_FAILURE,                        /**< General Failure */
    WDMP_ERR_TIMEOUT,
    WDMP_ERR_NOT_EXIST,
    WDMP_ERR_INVALID_PARAMETER_NAME,
    WDMP_ERR_INVALID_PARAMETER_TYPE,
    WDMP_ERR_INVALID_PARAMETER_VALUE,
    WDMP_ERR_NOT_WRITABLE,
    WDMP_ERR_SETATTRIBUTE_REJECTED,
    WDMP_ERR_REQUEST_REJECTED,
    WDMP_ERR_NAMESPACE_OVERLAP,
    WDMP_ERR_UNKNOWN_COMPONENT,
    WDMP_ERR_NAMESPACE_MISMATCH,
    WDMP_ERR_UNSUPPORTED_NAMESPACE,
    WDMP_ERR_DP_COMPONENT_VERSION_MISMATCH,
    WDMP_ERR_INVALID_PARAM,
    WDMP_ERR_UNSUPPORTED_DATATYPE,
    WDMP_STATUS_RESOURCES,
    WDMP_ERR_WIFI_BUSY,
    WDMP_ERR_INVALID_ATTRIBUTES,
    WDMP_ERR_WILDCARD_NOT_SUPPORTED,
    WDMP_ERR_SET_OF_CMC_OR_CID_NOT_SUPPORTED,
    WDMP_ERR_VALUE_IS_EMPTY,
    WDMP_ERR_VALUE_IS_NULL,
    WDMP_ERR_DATATYPE_IS_NULL,
    WDMP_ERR_CMC_TEST_FAILED,
    WDMP_ERR_NEW_CID_IS_MISSING,
    WDMP_ERR_CID_TEST_FAILED,
    WDMP_ERR_SETTING_CMC_OR_CID,
    WDMP_ERR_INVALID_INPUT_PARAMETER,
    WDMP_ERR_ATTRIBUTES_IS_NULL,
    WDMP_ERR_NOTIFY_IS_NULL,
    WDMP_ERR_INVALID_WIFI_INDEX,
    WDMP_ERR_INVALID_RADIO_INDEX,
    WDMP_ERR_ATOMIC_GET_SET_FAILED,
    WDMP_ERR_METHOD_NOT_SUPPORTED,
    WDMP_ERR_SESSION_IN_PROGRESS,
    WDMP_ERR_INTERNAL_ERROR,
    WDMP_ERR_DEFAULT_VALUE,
    WDMP_ERR_MAX_REQUEST
} WDMP_STATUS;

typedef struct _RFC_Param_t {
   char name[MAX_PARAM_LEN];
   char value[MAX_PARAM_LEN];
   DATATYPE type;
} RFC_ParamData_t;

typedef enum {
    eRecovery,
    eAutoExclude,
    eBootstrap,
    eDevXconf,
    eCIXconf,
    eXconf,
    eDac15
} TR181URL ;

/* function GetServerUrlFile - scans a file for a URL.
        Usage: size_t GetServerUrlFile <char *pServUrl> <size_t szBufSize> <char *pFileName>

            pServUrl - pointer to a char buffer to store the output string..

            szBufSize - the size of the character buffer in argument 1.

            pFileName - a character pointer to a filename to scan.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetServerUrlFile(char *pServUrl, size_t szBufSize, char *pFileName);

/* function GetTR181Url - gets a specific URL from tr181 associated with code downloads.

        Usage: size_t GetTR181Url <TR181URL eURL> <char *pRemoteInfo> <size_t szBufSize>

            eURL - the specific URL to query tr181 for.

            pUrlOut - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetTR181Url(TR181URL eURL, char *pUrlOut, size_t szBufSize);

int read_RFCProperty(char* type, const char* key, char *data, size_t datasize);
int doCodeBigSigning(int server_type, const char* SignInput, char *signurl, size_t signurlsize, char *outhheader, size_t outHeaderSize);
int GetSSRCodebigDetails(char *pIn, char *plocation, size_t locsize, char *pauthHeader, size_t authsize);
int GetXconfCodebigDetails(char *pIn, char *plocation, size_t locsize);
int getXconfRequestType(char *data, size_t tSize);
int codebigdownloadFile( int server_type, const char* artifactLocationUrl, const void* localDownloadLocation, char *pPostFields, int *httpCode );
WDMP_STATUS getRFCParameter(const char *pcCallerID, const char* pcParameterName, RFC_ParamData_t *pstParamData);
extern char* strcasestr(const char* s1, const char* s2);
#endif
