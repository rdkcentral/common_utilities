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
#include <string.h>
#include <stdarg.h>
#include "rdk_debug.h"
//#include "jsonlog.h"
#include "rdkv_cdl_log_wrapper.h"


int log_init()
{
	// Initialize RDK Logger
    rdk_LogOutput_File filelog;
    /* Extended initialization with programmatic configuration */
    rdk_logger_ext_config_t config = {
        .pModuleName = "LOG.RDK.DCM",     /* Module name */
        .loglevel = RDK_LOG_INFO,         /* Default log level */
        //.output = RDKLOG_OUTPUT_FILE,
        .output = RDKLOG_OUTPUT_CONSOLE,
        .format = RDKLOG_FORMAT_WITH_TS,  /* Timestamped format */
        .pFilePolicy =  NULL        /* using file output */
    };

    if (rdk_logger_ext_init(&config) != RDK_SUCCESS) {
        printf("UPLOADSTB : ERROR - Extended logger init failed\n");
    }
	SWUPDATELOG(LOG_INFO, "RDKLOG init completed\n");
	return 0;
}

void log_exit()
{
	SWUPDATELOG(LOG_INFO, "RDKLOG deinit\n");
	rdk_logger_deinit();
}
