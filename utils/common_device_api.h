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

#ifndef DEVICE_UTILS_H_
#define DEVICE_UTILS_H_

#include "rdk_fwdl_utils.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>

#ifndef GTEST_ENABLE
#define BOOTSTRAP_FILE          "/opt/secure/RFC/bootstrap.ini"
#define PARTNER_ID_FILE         "/opt/www/authService/partnerId3.dat"
#define DEVICE_PROPERTIES_FILE  "/etc/device.properties"
#define VERSION_FILE            "/version.txt"
#define ESTB_MAC_FILE           "/tmp/.estb_mac"
#else
#define BOOTSTRAP_FILE          "/tmp/bootstrap.ini"
#define PARTNER_ID_FILE         "/tmp/partnerId3.dat"
#define DEVICE_PROPERTIES_FILE  "/tmp/device.properties"
#define VERSION_FILE            "/tmp/version.txt"
#define ESTB_MAC_FILE           "/tmp/estbmacfile"
#endif
/* function GetAccountID - gets the account ID of the device.

        Usage: size_t GetAccountID <char *pAccountID> <size_t szBufSize>

            pAccountID - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetAccountID(char *pAccountID, size_t szBufSize);

/* function GetModelNum - gets the model number of the device.

        Usage: size_t GetModelNum <char *pModelNum> <size_t szBufSize>

            pModelNum - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetModelNum(char *pModelNum, size_t szBufSize );

/* function GetBuildType - gets the build type of the device in lowercase. Optionally, sets an enum
    indication the build type.
    Example: vbn or prod or qa or dev

        Usage: size_t GetBuildType <char *pBuildType> <size_t szBufSize> <BUILDTYPE *peBuildTypeOut>

            pBuildType - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            peBuildTypeOut - a pointer to a BUILDTYPE enum or NULL if not needed by the caller.
                Contains an enum indicating the buildtype if not NULL on function exit.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetBuildType(char *pBuildType, size_t szBufSize, BUILDTYPE *peBuildTypeOut);

/* function GetFirmwareVersion - gets the firmware version of the device.

        Usage: size_t GetFirmwareVersion <char *pFWVersion> <size_t szBufSize>

            pFWVersion - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetFirmwareVersion(char *pFWVersion, size_t szBufSize);

/* function GetEstbMac - gets the eSTB MAC address of the device.

        Usage: size_t GetEstbMac <char *pEstbMac> <size_t szBufSize>

            pEstbMac - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetEstbMac(char *pEstbMac, size_t szBufSize );

/* function GetPartnerId - gets the partner ID of the device.

        Usage: size_t GetPartnerId <char *pPartnerId> <size_t szBufSize>

            pPartnerId - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetPartnerId( char *pPartnerId, size_t szBufSize );

/* function CurrentRunningInst - gets the running instance of binary

        Usage: const char *file File name of service lock file


         RETURN - return TRUE if the any instance already running otherwise false
*/
bool CurrentRunningInst(const char *file);

size_t stripinvalidchar( char *pIn, size_t szIn );

#endif
