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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "codebigUtils.h"
#include "rdk_fwdl_utils.h"
#include <urlHelper.h>
#include <system_utils.h>
#include <ctype.h>
#include "rdm_types.h"
#include "rdm_utils.h"
#include "json_parse.h"

// DEBUG_CODEBIG_CDL can be turned on with distro feature debug_codebig_cdl

#define URL_DELIMITER   '?'


/* function doCodeBigSigning - creates an authorization signature and finds the Codebig URL to use for Codebig communication.
        Usage: int doCodeBigSigning <int server_type> <const char *SignInput> <char *signurl> <size_t signurlsize> <char *outhheader> <size_t outHeaderSize>

            server_type - HTTP_XCONF_CODEBIG if XCONF signing, defaults to CDL server otherwise

            SignInput - pointer to character string to create signature for.

            signurl - a character buffer to store the Codebig URL output string.

            signurlsize - the signurl buffer maximum length in bytes.

            outhheader - a character buffer to store the authorization signature output.

            outHeaderSize - the outhheader buffer maximum length in bytes.

            RETURN - number of characters copied to the output buffer.
*/

int doCodeBigSigning( int server_type, const char *SignInput, char *signurl, size_t signurlsize, char *outhheader, size_t outHeaderSize )
{
    return 0;
}

/* Description: Get Xconf communication codebig url details
 * @param pIn : Buffer containing response of codebig signing
 * @param plocation: Signed XCONF URL
 * @param locsize : Signed URL size
 * @return int 0 on SUCCESS
 * */
int GetXconfCodebigDetails( char *pIn, char *plocation, size_t locsize )
{
    return 0;
}

/* Description: Get SSR download server codebig url details
 * @param pIn : Buffer containing response of codebig signing
 * @param plocation: Signed SSR URL
 * @param locsize : Signed URL size
 * @param pauthHeader : Authorization header string for SSR URL
 * @param authsize : Size of Authorization header
 * @return int 0 on SUCCESS
 * */

int GetSSRCodebigDetails( char *pIn, char *plocation, size_t locsize, char *pauthHeader, size_t authsize )
{
    return 0;
}

/* Description: Get Xconf codebig url details
 * @param data : Xconf URL
 * @param tDataSize: Size of URL
 * @return int 0 for type 1 URL, int 1 for type 2 URL etc
 * */
int getXconfRequestType( char *data, size_t tDataSize )
{
    return 0;
}

/* Description: Reading rfc data
 * @param type : rfc type
 * @param key: rfc key
 * @param data : Store rfc value
 * @return int 1 READ_RFC_SUCCESS on success and READ_RFC_FAILURE -1 on failure
 * */
int read_RFCProperty(char* type, const char* key, char *out_value, size_t datasize) {
    return 0;
}
/* function GetServerUrlFile - scans a file for a URL.
        Usage: size_t GetServerUrlFile <char *pServUrl> <size_t szBufSize> <char *pFileName>

            pServUrl - pointer to a char buffer to store the output string..

            szBufSize - the size of the character buffer in argument 1.

            pFileName - a character pointer to a filename to scan.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetServerUrlFile( char *pServUrl, size_t szBufSize, char *pFileName )
{
    return 0;
}

/* function GetTR181Url - gets a specific URL from tr181 associated with code downloads.

        Usage: size_t GetTR181Url <TR181URL eURL> <char *pRemoteInfo> <size_t szBufSize>

            eURL - the specific URL to query tr181 for.

            pUrlOut - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetTR181Url( TR181URL eURL, char *pUrlOut, size_t szBufSize )
{
    return 0;
}
