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

#ifndef  __JSONLOG_H_
#define __JSONLOG_H_

#include <time.h>
//#include "rdk_debug.h"

int log_init();
void log_exit();


#define DEBUG_INI_NAME  "/etc/debug.ini"


#define LOG_INFO RDK_LOG_INFO
#define LOG_ERR RDK_LOG_ERROR
#define LOG_DBG RDK_LOG_INFO


#define SWUPDATELOG(level, ...) do { \
			RDK_LOG(level, "LOG.RDK.FWUPG", __VA_ARGS__); \
			} while (0);
#define COMMONUTILITIESLOG(level, ...) do { \
			RDK_LOG(level, "LOG.RDK.COMMONUTILITIES", __VA_ARGS__); \
			} while (0);

#endif
