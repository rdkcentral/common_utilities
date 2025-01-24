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
	rdk_logger_init(DEBUG_INI_NAME);
	SWUPDATELOG(LOG_INFO, "RDKLOG init completed\n");
	return 0;
}

void log_exit()
{
	SWUPDATELOG(LOG_INFO, "RDKLOG deinit\n");
	rdk_logger_deinit();
}
