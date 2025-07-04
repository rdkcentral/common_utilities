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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "rdk_debug.h"
//#include "jsonlog.h"
#include "json_parse.h"
#include "rdkv_cdl_log_wrapper.h"

/* jsonget - prints the value for a "name : value" pair in a JSON File
   Usage: jsonget <Input JSON file> <Name to search for>
            Input JSON file - filename containing the JSON to search.

            Name to search for - the name from the JSON's "name : value" pairs.

            OUTPUT - prints the value from the JSON's "name : value" pair, if found.

            RETURN - integer value of 0 if "name" is found, 1 otherwise.
*/


int main( int argc, char *argv[] )
{
    char *pJsonStr = NULL;
    int len, iRet = 1;
    char valbuf[256];

    log_init();
    if( argc >= 3 )
    {
        pJsonStr = GetJson( argv[1] );
        if( pJsonStr != NULL )
        {
            len = GetJsonVal( pJsonStr, NULL, argv[2], valbuf, sizeof(valbuf) );
            if( len )
            {
                printf( "%s\n", valbuf );
                iRet = 0;
            }
            free( pJsonStr );
        }
    }
    else
    {
        COMMONUTILITIES_INFO( "Error: Too few arguments\nUsage %s <input filename> <JSON string to search for>\n", argv[0] );
    }
    log_exit();
    return iRet;
}

