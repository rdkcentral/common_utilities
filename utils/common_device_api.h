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
#include "system_utils.h"
#include "json_parse.h"

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
#define OUTPUT_JSON_FILE        "/opt/output.json"
#define OUTPUT_JSON_FILE_X86    "/tmp/output.json"
#define TIMEZONE_DST_FILE       "/opt/persistent/timeZoneDST"
#define TIMEZONE_OFFSET_MAP     "/etc/timeZone_offset_map"
#else
#define BOOTSTRAP_FILE          "/tmp/bootstrap.ini"
#define PARTNER_ID_FILE         "/tmp/partnerId3.dat"
#define DEVICE_PROPERTIES_FILE  "/tmp/device.properties"
#define VERSION_FILE            "/tmp/version.txt"
#define ESTB_MAC_FILE           "/tmp/estbmacfile"
#define OUTPUT_JSON_FILE        "/tmp/output.json"
#define OUTPUT_JSON_FILE_X86    "/tmp/output.json"
#define TIMEZONE_DST_FILE       "/tmp/timeZoneDST"
#define TIMEZONE_OFFSET_MAP     "/tmp/timeZone_offset_map"
#define PERIPHERAL_JSON_FILE    "/tmp/rc-proxy-params.json"
#endif

typedef struct metaDataFileList
{
    char fileName[512];
    struct metaDataFileList *next;
}metaDataFileList_st;

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

/* function GetMFRName - gets the manufacturer of the device.
        Usage: size_t GetMFRName <char *pMFRName> <size_t szBufSize>
            pMFRName - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetMFRName(char *pMFRName, size_t szBufSize );

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

/* function GetUTCTime - gets a formatted UTC device time. Example:
    Tue Jul 12 21:56:06 UTC 2022 
        Usage: size_t GetUTCTime <char *pUTCTime> <size_t szBufSize>
 
            pUTCTime - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetUTCTime( char *pUTCTime, size_t szBufSize );

/* function GetCapabilities - gets the device capabilities.
 
        Usage: size_t GetCapabilities <char *pCapabilities> <size_t szBufSize>
 
            pCapabilities - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetCapabilities( char *pCapabilities, size_t szBufSize );

/* function GetServerUrlFile - scans a file for a URL. 
        Usage: size_t GetServerUrlFile <char *pServUrl> <size_t szBufSize> <char *pFileName>
 
            pServUrl - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            pFileName - a character pointer to a filename to scan.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetServerUrlFile( char *pServUrl, size_t szBufSize, char *pFileName );

/* function GetTimezone - gets the timezone of the device.

        Usage: size_t GetTimezone <char *pTimezone> <const char *cpuArch> <size_t szBufSize>

            pTimezone - pointer to a char buffer to store the output string.

            cpuArch - the CPU architecture (can be NULL).

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetTimezone( char *pTimezone, const char *cpuArch, size_t szBufSize );

/* function GetFileContents - gets the contents of a file into a dynamically allocated buffer.

        Usage: size_t GetFileContents <char **pOut> <char *pFileName>

            pOut - the address of a char pointer (char **) where the dynamically allocated
                    character buffer will be located.

            pFileName - the name of the file to read.

            RETURN - number of characters copied to the output buffer.

            Notes - GetFileContents uses malloc to allocate the the buffer where the string is stored.
                    The caller must use free(*pOut) when done using the buffer to avoid memory leaks.
*/
size_t GetFileContents( char **pOut, char *pFileName );

/* function makeHttpHttps - converts http:// URLs to https:// in place.

        Usage: size_t makeHttpHttps <char *pIn> <size_t szpInSize>

            pIn - pointer to a char buffer containing the URL to modify.

            szpInSize - the size of the character buffer in argument 1.

            RETURN - new length of the string after modification.
*/
size_t makeHttpHttps( char *pIn, size_t szpInSize );

/* function get_system_uptime - gets the system uptime in seconds.

        Usage: bool get_system_uptime <double *uptime>

            uptime - pointer to a double to store the uptime value.

            RETURN - true if successful, false on error.
*/
bool get_system_uptime(double *uptime);

/* function RunCommand - runs a predefined system command using secure popen        Usage: size_t RunCommand <DEVUTILS_SYSCMD eSysCmd> <const char *pArgs> <char *pResult> <size_t szResultSize>

            eSysCmd - the predefined system command to execute from DEVUTILS_SYSCMD enum
 
            pArgs - arguments to pass to the command (NULL if no arguments required)
 
            pResult - a pointer to a character buffer to store the command output
 
            szResultSize - the maximum size of the result buffer
 
            RETURN - the number of characters written to the result buffer
 
            PREDEFINED COMMANDS:
            "/usr/bin/WPEFrameworkSecurityUtility"                      eWpeFrameworkSecurityUtility
            "/usr/bin/mfr_util %s"                                      eMfrUtil
            "/usr/bin/md5sum %s"                                        eMD5Sum
            "/usr/sbin/rdkssacli %s"                                    eRdkSsaCli
            "/lib/rdk/cdlSupport.sh getInstalledRdmManifestVersion"     eGetInstalledRdmManifestVersion
 
            %s in the command string indicates an argument (pArgs) is required
*/
size_t RunCommand(DEVUTILS_SYSCMD eSysCmd, const char *pArgs, char *pResult, size_t szResultSize);

size_t stripinvalidchar( char *pIn, size_t szIn );

/* function GetAdditionalFwVerInfo - returns the PDRI filename plus Remote Info for the device.
        Usage: size_t GetAdditionalFwVerInfo <char *pAdditionalFwVerInfo> <size_t szBufSize>
 
            pAdditionalFwVerInfo - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetAdditionalFwVerInfo( char *pAdditionalFwVerInfo, size_t szBufSize );

/* function GetPDRIFileName - returns the PDRI for the device.
        Usage: size_t GetPDRIFileName <char *pPDRIFilename> <size_t szBufSize>
 
            pPDRIFilename - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetPDRIFileName( char *pPDRIFilename, size_t szBufSize );

/* function GetInstalledBundles - gets the bundles installed on a device.
        Usage: size_t GetInstalledBundles <char *pBundles> <size_t szBufSize>
 
            pBundles - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetInstalledBundles(char *pBundles, size_t szBufSize);

/* function GetRemoteInfo - gets the remote info of the device.
        Usage: size_t GetRemoteInfo <char *pRemoteInfo> <size_t szBufSize>
 
            pRemoteInfo - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetRemoteInfo( char *pRemoteInfo, size_t szBufSize );

/* function GetRemoteVers - gets the peripheral versions of the device.
        Usage: size_t GetRemoteVers <char *pRemoteVers > <size_t szBufSize>
 
            pRemoteVers - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetRemoteVers( char *pRemoteVers , size_t szBufSize );

/* function GetRdmManifestVersion - gets the remote info of the device.
        Usage: size_t GetRdmManifestVersion <char *pRdmManifestVersion> <size_t szBufSize>
 
            pRdmManifestVersion - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetRdmManifestVersion( char *pRdmManifestVersion, size_t szBufSize );

/* function BuildRemoteInfo - Formats the "periperalFirmwares" string for remote info part of xconf communication
        Usage: size_t BuildRemoteInfo <JSON *pItem> <char *pRemoteInfo> <size_t szMaxBuf> <bool bAddremCtrl>
 
            pItem - a pointer to a JSON structure that contains the remote info.
            pRemoteInfo - a pointer to a character buffer to store the output
            szMaxBuf - the maximum size of the buffer
            bAddremCtrl - if true then prefix values with &remCtrl. false does not add prefix
            RETURN - the number of characters written to the buffer
*/
size_t BuildRemoteInfo( JSON *pItem, char *pRemoteInfo, size_t szMaxBuf, bool bAddremCtrl );

/* function getInstalledBundleFileList - gets the list of bundles installed on a device.
        Usage: metaDataFileList_st *getInstalledBundleFileList()
        RETURN - pointer to linked list of metadata files, or NULL on error
*/
metaDataFileList_st *getInstalledBundleFileList();

#endif
