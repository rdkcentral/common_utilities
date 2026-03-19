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
#include "json_parse.h"
#include "rdkv_cdl_log_wrapper.h"


/* jsonwrite- optionally writes the values for all "name : value" pairs in a JSON File in the
   format name=value and/or set environment variables using name and value.
   Usage: int jsonwrite <Input JSON file> <Output File> <setenv Flag>

            Input JSON file - filename containing the JSON to parse and output.

            Output file - where to write the output name=value pairs. If NULL
            then no output file is created.

            Set Environment Variables - optional flage to specify whether environment variables
            should be set using the setenv function. Set to 0 (zero) to skip setting of 
            environment variables or non-zero to set them. Defaults to 1 if argument is
            not present.

            RETURN - integer value of 0 on success, 1 otherwise.
*/

int main( int argc, char *argv[] )
{
    char *inpfile = NULL;
    char *outfile = NULL;
    int setenvvars = 1;
    int opt;
    int iRet = 1;

    log_init();
    COMMONUTILITIES_INFO("Starting jsonwrite\n");
    if( argc >= 3  && *argv[2] )
    {
        outfile = argv[2];
    }
    if( argc == 4 )
    {
        setenvvars = atoi( argv[3] );
    }

    if( argc >=2 )
    {
       iRet = SetJsonVars( argv[1], outfile, setenvvars );
    }
    else
    {
        COMMONUTILITIES_ERROR( "Error: Too few arguments, must contain an input file name\nUsage %s <input filename> <optional output file name>\n", argv[0] );
    }
    log_exit();
    return iRet;
}
