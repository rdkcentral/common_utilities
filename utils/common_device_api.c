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

#define _GNU_SOURCE  // Required for strcasestr
#include <time.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include "common_device_api.h"
#include "rdkv_cdl_log_wrapper.h"
#include "json_parse.h"
#include "downloadUtil.h"

#define PARTNERID_INFO_FILE "/tmp/partnerId.out"

// Bundle metadata paths - conditional compilation
#ifdef DEVICE_PROFILE_ARM_ATLAS_64X64
    #define BUNDLE_METADATA_NVM_PATH    "/media/apps/etc/certs"
    #define BUNDLE_METADATA_RFS_PATH    "/etc/certs"
#else
    #define BUNDLE_METADATA_NVM_PATH    "/tmp/certs"
    #define BUNDLE_METADATA_RFS_PATH    "/tmp/rfc/certs"
#endif

// Command paths for RunCommand function
#define WPEFRAMEWORKSECURITYUTILITY     "/usr/bin/WPEFrameworkSecurityUtility"
#define MFRUTIL                         "/usr/bin/mfr_util %s"
#define MD5SUM                          "/usr/bin/md5sum %s"
#define RDKSSACLI                       "/usr/sbin/rdkssacli %s"

#ifdef GETRDMMANIFESTVERSION_IN_SCRIPT
#define GETINSTALLEDRDMMANIFESTVERSIONSCRIPT    "/lib/rdk/cdlSupport.sh getInstalledRdmManifestVersion"
#endif

#define PERIPHERAL_JSON_FILE            "/opt/persistent/rdm/peripheral.json"
#define MAX_PERIPHERAL_ITEMS 4

static metaDataFileList_st *getMetaDataFile(char *dir);
static metaDataFileList_st *mergeLists(metaDataFileList_st *nvmList, metaDataFileList_st *rfsList);

// String arrays for BuildRemoteInfo function
static char *pRemCtrlStrings[MAX_PERIPHERAL_ITEMS] = {
    "&remCtrl",
    "&remCtrlAudio",
    "&remCtrlDsp",
    "&remCtrlKwModel"
};

static char *pNullStrings[MAX_PERIPHERAL_ITEMS] = {
    "",
    "",
    "",
    ""
};

static char *pEqualStrings[MAX_PERIPHERAL_ITEMS] = {
    "=",
    "=",
    "=",
    "="
};

static char *pTypeStrings[MAX_PERIPHERAL_ITEMS] = {
    "_firmware_",
    "_audio_",
    "_dsp_",
    "_kw_model_"
};

static char *pExtStrings[] = {
    ".tgz,"
};

static char *pPeripheralName[MAX_PERIPHERAL_ITEMS] = {
    "FwVer",
    "AudioVer",
    "DspVer",
    "KwModelVer"
};

/* function stripinvalidchar - truncates a string when a space or control
    character is encountered.

        Usage: size_t stripinvalidchar <char *pIn> <size_t szIn>

            pIn - pointer to a char buffer to check/modify.

            szIn - the size of the character buffer in argument 1.

            RETURN - number of characters in the buffer upon exit.

            PITFALLS - does not check for NULL input
*/
size_t stripinvalidchar( char *pIn, size_t szIn )
{
    size_t i = 0;

    if( pIn != NULL )
    {
        while( *pIn && szIn )
        {
            if( isspace( *pIn ) || iscntrl( *pIn ) )
            {
                *pIn = 0;
                break;
            }
            ++pIn;
            --szIn;
            ++i;
        }
    }

    return i;
}
/* function GetEstbMac - gets the eSTB MAC address of the device.

        Usage: size_t GetEstbMac <char *pEstbMac> <size_t szBufSize>

            pEstbMac - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetEstbMac( char *pEstbMac, size_t szBufSize )
{
    FILE *fp;
    size_t i = 0;

    if( pEstbMac != NULL )
    {
        *pEstbMac = 0;
        if( (fp = fopen( ESTB_MAC_FILE, "r" )) != NULL )
        {
            fgets( pEstbMac, szBufSize, fp );   // better be a valid string on first line
            fclose( fp );
            i = stripinvalidchar( pEstbMac, szBufSize );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetEstbMac: Error, input argument NULL\n" );
    }
    return i;
}

/* function GetPartnerId - gets the partner ID of the device.

        Usage: size_t GetPartnerId <char *pPartnerId> <size_t szBufSize>

            pPartnerId - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetPartnerId( char *pPartnerId, size_t szBufSize )
{
    char *pTmp;
    FILE *fp;
    size_t i = 0;
    char buf[150];

    COMMONUTILITIES_INFO("*** CALLING GetPartnerId FROM COMMON_UTILITIES/LIBFWUTILS ***\n");

    if( pPartnerId != NULL )
    {
        *pPartnerId = 0;
        if( (fp = fopen( PARTNER_ID_FILE, "r" )) != NULL )
        {
            fgets( pPartnerId, szBufSize, fp );
            fclose( fp );
        }
        else if( (fp = fopen( BOOTSTRAP_FILE, "r" )) != NULL )
        {
            while( fgets( buf, sizeof(buf), fp ) != NULL )
            {
                if( (pTmp = strstr( buf, "X_RDKCENTRAL-COM_Syndication.PartnerId" )) != NULL )
                {
                    while( *pTmp && *pTmp++ != '=' )
                    {
                        ;
                    }
                    snprintf( pPartnerId, szBufSize, "%s", buf );
                }
            }
            fclose( fp );
        }
        else if( (fp = fopen( PARTNERID_INFO_FILE, "r" )) != NULL )
        {
            fgets( pPartnerId, szBufSize, fp );
            fclose( fp );
        }
        else if( (fp = popen( "syscfg get PartnerID", "r" )) != NULL )
        {
            fgets( pPartnerId, szBufSize, fp );
            pclose( fp );
        }			
        else
        {
            snprintf( pPartnerId, szBufSize, "comcast" );
        }
        i = stripinvalidchar( pPartnerId, szBufSize );      // remove newline etc.
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetPartnerId: Error, input argument NULL\n" );
    }
    return i;
}

/* function GetModelNum - gets the model number of the device.

        Usage: size_t GetModelNum <char *pModelNum> <size_t szBufSize>

            pModelNum - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetModelNum( char *pModelNum, size_t szBufSize )
{
    size_t i = 0;
    FILE *fp;
    char *pTmp;
    char buf[150];

    if( pModelNum != NULL )
    {
        *pModelNum = 0;
        if( (fp = fopen( DEVICE_PROPERTIES_FILE, "r" )) != NULL )
        {
            while( fgets( buf, sizeof(buf), fp ) != NULL )
            {
                pTmp = strstr( buf, "MODEL_NUM=" );
                if( pTmp && pTmp == buf )   // if match found and match is first character on line
                {
                    pTmp = strchr( pTmp, '=' );
		    if(pTmp != NULL)
		    {
                    ++pTmp;
                    i = snprintf( pModelNum, szBufSize, "%s", pTmp );
                    i = stripinvalidchar( pModelNum, i );
		    }
                }
            }
            fclose( fp );
        }
        else
        {
            COMMONUTILITIES_ERROR( "GetModelNum: Cannot open %s for reading\n", DEVICE_PROPERTIES_FILE );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetModelNum: Error, input argument NULL\n" );
    }
    return i;
}

/* function GetMFRName - gets the  manufacturer name of the device.
        Usage: size_t GetMFRName <char *pMFRName> <size_t szBufSize>
            pMFRName - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetMFRName( char *pMFRName, size_t szBufSize )
{
    size_t i = 0;
    FILE *fp;
    if( pMFRName != NULL )
    {
        *pMFRName = 0;
        if( (fp = fopen( "/tmp/.manufacturer", "r" )) != NULL )
	{
            fgets(pMFRName, szBufSize, fp);
            fclose( fp );
            i = stripinvalidchar( pMFRName, szBufSize );      // remove newline etc.
	}
        else
        {
            COMMONUTILITIES_ERROR( "GetMFRName: Cannot open %s for reading\n", "/tmp/.manufacturer" );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetMFRName: Error, input argument NULL\n" );
    }
    return i;

}

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
size_t GetBuildType( char *pBuildType, size_t szBufSize, BUILDTYPE *peBuildTypeOut )
{
    FILE *fp;
    char *pTmp, *pOut = NULL;
    size_t i = 0;
    BUILDTYPE eBuildType = eUNKNOWN;
    char buf[150];

    if( pBuildType != NULL )
    {
        *pBuildType = 0;
        if( (fp = fopen( DEVICE_PROPERTIES_FILE, "r" )) != NULL )
        {
            while( fgets( buf, sizeof(buf), fp ) != NULL )
            {
                pTmp = strstr( buf, "BUILD_TYPE=" );
                if( pTmp && pTmp == buf )   // if match found and match is first character on line
                {
                    pTmp = strchr( pTmp, '=' );
		    if(pTmp != NULL){
		    ++pTmp;
                    i = snprintf( pBuildType, szBufSize, "%s", pTmp );
                    i = stripinvalidchar( pBuildType, i );
                    pTmp = pBuildType;
                    while( *pTmp )
                    {
                        *pTmp = tolower( *pTmp );
                        ++pTmp;
                    }
		  }
                }
            }
            fclose( fp );
        }
        if( *pBuildType == 0 )
        {
            GetFirmwareVersion( buf, sizeof(buf) );
            pTmp = buf;
            while( *pTmp )
            {
                *pTmp = tolower( *pTmp );
                ++pTmp;
            }
        }
        else
        {
            pTmp = pBuildType;
        }

        // run the following series of checks to set eBuildType
        // pBuildType must also be set if the value was found with GetFirmwareVersion()
        if( strstr( pTmp, "vbn" ) != NULL )
        {
            pOut = "vbn";
            eBuildType = eVBN;
        }
        else if( strstr( pTmp, "prod" ) != NULL )
        {
            pOut = "prod";
            eBuildType = ePROD;
        }
        else if( strstr( pTmp, "qa" ) != NULL )
        {
            pOut = "qa";
            eBuildType = eQA;
        }
        else if( strstr( pTmp, "dev" ) != NULL )
        {
            pOut = "dev";
            eBuildType = eDEV;
        }

        if( *pBuildType == 0 && pOut != NULL )
        {
            i = snprintf( pBuildType, szBufSize, "%s", pOut );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetBuildType: Error, input argument NULL\n" );
    }
    if( peBuildTypeOut != NULL )
    {
        *peBuildTypeOut = eBuildType;
    }
    return i;
}

/* function GetFirmwareVersion - gets the firmware version of the device.

        Usage: size_t GetFirmwareVersion <char *pFWVersion> <size_t szBufSize>

            pFWVersion - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetFirmwareVersion( char *pFWVersion, size_t szBufSize )
{
    FILE *fp;
    size_t i = 0;
    char *pTmp;
    char buf[150];

    COMMONUTILITIES_INFO("*** CALLING GetFirmwareVersion FROM COMMON_UTILITIES/LIBFWUTILS ***\n");

    if( pFWVersion != NULL )
    {
        *pFWVersion = 0;
        if( (fp = fopen( VERSION_FILE, "r" )) != NULL )
        {
            pTmp = NULL;
            while( fgets( buf, sizeof(buf), fp ) != NULL )
            {
                if( (pTmp = strstr( buf, "imagename:" )) != NULL )
                {
                    while( *pTmp++ != ':' )
                    {
                        ;
                    }
                    break;
                }
            }
            fclose( fp );
            if( pTmp )
            {
                i = snprintf( pFWVersion, szBufSize, "%s", pTmp );
                i = stripinvalidchar( pFWVersion, i );
            }
        }
    }
    else
    {
        COMMONUTILITIES_INFO( "GetFirmwareVersion: Error, input argument NULL\n" );
    }
    return i;
}

bool CurrentRunningInst(const char *file)
{
    int fd = -1;

    fd = open(file, O_CREAT | O_RDWR, 0666);

    if (fd == -1)
    {
        COMMONUTILITIES_ERROR( "CurrentRunningInst: Failed to open lock file\n" );
        return false;
    }

    if (flock(fd, LOCK_EX | LOCK_NB) != 0)
    {
        COMMONUTILITIES_ERROR( "CurrentRunningInst: Failed to aquire lock\n" );
        close(fd);
        return true;
    }

    /* OK to proceed (lock will be released and file descriptor will be closed on exit) */

    return false;
}

/* function GetUTCTime - gets a formatted UTC device time. Example:
    Tue Jul 12 21:56:06 UTC 2022 
        Usage: size_t GetUTCTime <char *pUTCTime> <size_t szBufSize>
 
            pUTCTime - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetUTCTime( char *pUTCTime, size_t szBufSize )
{
    struct tm gmttime;
    time_t seconds;
    size_t i = 0;

    COMMONUTILITIES_DEBUG("GetUTCTime: Called with buffer size %zu\n", szBufSize);

    if( pUTCTime != NULL )
    {
        time( &seconds );
        gmtime_r( &seconds, &gmttime );
        gmttime.tm_isdst = 0;           // UTC doesn't know about DST, perhaps unnecessary but be safe
        i = strftime( pUTCTime, szBufSize, "%a %b %d %X UTC %Y", &gmttime );
        if( !i )    // buffer wasn't big enough for strftime call above
        {
            *pUTCTime = 0;
        }
        COMMONUTILITIES_DEBUG("GetUTCTime: Returning time '%s' with length %zu\n", pUTCTime, i);
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetUTCTime: Error, input argument NULL\n" );
    }
    return i;
}

/* function GetCapabilities - gets the device capabilities.
 
        Usage: size_t GetCapabilities <char *pCapabilities> <size_t szBufSize>
 
            pCapabilities - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetCapabilities( char *pCapabilities, size_t szBufSize )
{
    size_t i = 0;
    const char *capabilities = "rebootDecoupled&capabilities=RCDL&capabilities=supportsFullHttpUrl";

    COMMONUTILITIES_DEBUG("GetCapabilities: Called with buffer size %zu\n", szBufSize);

    if( pCapabilities != NULL )
    {
        i = snprintf( pCapabilities, szBufSize, "%s", capabilities );
        COMMONUTILITIES_DEBUG("GetCapabilities: Returning capabilities '%s' with length %zu\n", pCapabilities, i);
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetCapabilities: Error, input argument NULL\n" );
    }
    return i;
}

/* function GetServerUrlFile - scans a file for a URL. 
        Usage: size_t GetServerUrlFile <char *pServUrl> <size_t szBufSize> <char *pFileName>
 
            pServUrl - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            pFileName - a character pointer to a filename to scan.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetServerUrlFile( char *pServUrl, size_t szBufSize, char *pFileName )
{
    FILE *fp;
    char *pHttp, *pLb;
    size_t i = 0;
    char buf[256];

    COMMONUTILITIES_DEBUG("GetServerUrlFile: Called with filename '%s', buffer size %zu\n", 
                         pFileName ? pFileName : "NULL", szBufSize);

    if( pServUrl != NULL && pFileName != NULL )
    {
        *pServUrl = 0;
        if( (fp = fopen( pFileName, "r" )) != NULL )
        {
            pHttp = NULL;
            while( (pHttp == NULL) && (fgets( buf, sizeof(buf), fp ) != NULL) )
            {
                if( (pHttp = strstr( buf, "https://" )) != NULL )
                {
                    if( (pLb = strchr( buf, (int)'#' )) != NULL )
                    {
                        if( pLb <= pHttp )
                        {
                            pHttp = NULL;
                            continue;       // '#' is before http or at beginning means commented line
                        }
                        else
                        {
                            *pLb = 0;       // otherwise NULL terminate the string
                        }
                    }
                    pLb = pHttp + 8;    // reuse pLb for parsing, pLb should point to first character after https://
                    while( *pLb )   // convert non-alpha numerics (but not '.') or whitespace to NULL terminator
                    {
                        if( (!isalnum( *pLb ) && *pLb != '.' && *pLb != '/' && *pLb != '-' && *pLb != '_' && *pLb != ':') || isspace( *pLb ) )
                        {
                            *pLb = 0;   // NULL terminate at end of URL
                            break;
                        }
                        ++pLb;
                    }
                }
            }
            fclose( fp );
            if( pHttp != NULL )
            {
                i = snprintf( pServUrl, szBufSize, "%s", pHttp );
                COMMONUTILITIES_DEBUG("GetServerUrlFile: Found URL '%s' with length %zu\n", pServUrl, i);
            }
        }
        else
        {
            COMMONUTILITIES_INFO( "GetServerUrlFile: %s can't be opened\n", pFileName );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetServerUrlFile: Error, input argument NULL\n" );
    }
    return i;
}

/* function GetTimezone - gets the timezone of the device.

        Usage: size_t GetTimezone <char *pTimezone> <const char *cpuArch> <size_t szBufSize>

            pTimezone - pointer to a char buffer to store the output string.

            cpuArch - the CPU architecture (can be NULL).

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetTimezone( char *pTimezone, const char *cpuArch, size_t szBufSize )
{
    int ret;
    FILE *fp;
    char *pTmp;
    size_t i = 0;
    char buf[256];
    char *timezonefile;
    char *defaultimezone = "Universal";
    char device_name[32];
    char timeZone_offset_map[50];
    char *zoneValue = NULL;

    COMMONUTILITIES_DEBUG("GetTimezone: Called with cpuArch '%s', buffer size %zu\n", 
                         cpuArch ? cpuArch : "NULL", szBufSize);

    if( pTimezone != NULL )
    {
        *pTimezone = 0;
        *timeZone_offset_map = 0;

        ret = getDevicePropertyData("DEVICE_NAME", device_name, sizeof(device_name));
        if (ret == UTILS_SUCCESS)
        {
            if (0 == (strncmp(device_name, "PLATCO", 6)))
            {
                if((fp = fopen( TIMEZONE_DST_FILE, "r" )) != NULL )
                {
                    COMMONUTILITIES_INFO("GetTimezone: Reading Timezone value from %s file...\n", TIMEZONE_DST_FILE );
                    if (fgets( buf, sizeof(buf), fp ) != NULL)      // only read first line in file, timezone should be there
                    {
                        i = stripinvalidchar( buf, sizeof(buf) );
                        COMMONUTILITIES_INFO("GetTimezone: Device TimeZone:%s\n", buf );
                    }
                    fclose( fp );
                }

                if( i == 0 )    // either TIMEZONE_DST_FILE non-existent or empty, set a default in pTimezone
                {
                    COMMONUTILITIES_INFO("GetTimezone: %s is empty or non-existent, default timezone America/New_York applied\n", TIMEZONE_DST_FILE);
                    snprintf( buf, sizeof(buf), "America/New_York");
                }

                if((fp = fopen( TIMEZONE_OFFSET_MAP, "r" )) != NULL )
                {
                    while ( fgets(timeZone_offset_map, sizeof(timeZone_offset_map), fp ) != NULL )
                    {
                        if( strstr( timeZone_offset_map, buf ) != NULL )
                        {
                            zoneValue = strtok(timeZone_offset_map, ":");
                            if( zoneValue != NULL )  // there's more after ':'
                            {
                                zoneValue = strtok(NULL, ":");
                            }
                            break; //match found, breaks the while loop
                        }
                    }
                    fclose( fp );
                }

                if( zoneValue != NULL )
                {
                    i = snprintf( pTimezone, szBufSize, "%s", zoneValue );
                }
                else
                {
                    i = snprintf( pTimezone, szBufSize, "US/Eastern" );
                    COMMONUTILITIES_INFO("GetTimezone: Given TimeZone not supported by XConf - default timezone US/Eastern is applied\n");
                }
                COMMONUTILITIES_INFO("GetTimezone: TimeZone Information after mapping : pTimezone = %s\n", pTimezone );
            }
            else
            {
                if( cpuArch != NULL && (0 == (strncmp( cpuArch, "x86", 3 ))) )
                {
                    timezonefile = OUTPUT_JSON_FILE_X86; //For cpu arch x86 file path is /tmp
                }
                else
                {
                    timezonefile = OUTPUT_JSON_FILE;      //File path is /opt
                }

                if( (fp = fopen( timezonefile, "r" )) != NULL )
                {
                    COMMONUTILITIES_INFO("GetTimezone: Reading Timezone value from %s file...\n", timezonefile );
                    while( fgets( buf, sizeof(buf), fp ) != NULL )
                    {
                        if( (pTmp = strstr( buf, "timezone" )) != NULL )
                        {
                            while( *pTmp && *pTmp != ':' )  // should be left pointing to ':' at end of while
                            {
                                ++pTmp;
                            }

                            while( !isalnum( *pTmp ) )  // at end of while we should be pointing to first alphanumeric char after ':', this is timezone
                            {
                                ++pTmp;
                            }
                            i = snprintf( pTimezone, szBufSize, "%s", pTmp );
                            i = stripinvalidchar( pTimezone, i );
                            pTmp = pTimezone;
                            i = 0;
                            while( *pTmp != '"' && *pTmp )     // see if we have an end quote
                            {
                                ++i;    // recount chars for safety
                                ++pTmp;
                            }
                            *pTmp = 0;                  // either we're pointing to the end " character or a 0
                            COMMONUTILITIES_INFO("GetTimezone: Got timezone using %s successfully, value:%s\n", timezonefile, pTimezone );
                            // Note: t2ValNotify removed - it's rdkfwupdater-specific
                            break;
                        }
                    }
                    fclose( fp );
                }

                if( !i && (fp = fopen( TIMEZONE_DST_FILE, "r" )) != NULL )    // if we didn't find it above, try default
                {
                    COMMONUTILITIES_INFO("GetTimezone: Timezone value from output.json is empty, Reading from %s file...\n", TIMEZONE_DST_FILE );
                    if (fgets( buf, sizeof(buf), fp ) != NULL)              // only read first line
                    {
                        i = snprintf( pTimezone, szBufSize, "%s", buf );
                        i = stripinvalidchar( pTimezone, i );
                        COMMONUTILITIES_INFO("GetTimezone: Got timezone using %s successfully, value:%s\n", TIMEZONE_DST_FILE, pTimezone );
                    }
                    fclose( fp );
                }

                if ( !i )
                {
                    i = snprintf( pTimezone, szBufSize, "%s", defaultimezone );
                    COMMONUTILITIES_INFO("GetTimezone: Timezone files %s and %s not found, proceeding with default timezone=%s\n", timezonefile, TIMEZONE_DST_FILE, pTimezone);
                }
            }
        }
        else
        {
            COMMONUTILITIES_ERROR("GetTimezone: getDevicePropertyData() for device_name fail\n");
        }
    }
    else
    {
        COMMONUTILITIES_ERROR("GetTimezone: Error, input argument NULL\n");
    }
    
    COMMONUTILITIES_DEBUG("GetTimezone: Returning timezone '%s' with length %zu\n", pTimezone, i);
    return i;
}

/* function GetFileContents - gets the contents of a file into a dynamically allocated buffer.

        Usage: size_t GetFileContents <char **pOut> <char *pFileName>

            pOut - the address of a char pointer (char **) where the dynamically allocated
                    character buffer will be located.

            pFileName - the name of the file to read.

            RETURN - number of characters copied to the output buffer.

            Notes - GetFileContents uses malloc to allocate the the buffer where the string is stored.
                    The caller must use free(*pOut) when done using the buffer to avoid memory leaks.
*/
size_t GetFileContents( char **pOut, char *pFileName )
{
    FILE *fp;
    char *pBuf = NULL;
    char *pPtr;
    size_t len = 0;

    COMMONUTILITIES_DEBUG("GetFileContents: Called with filename '%s'\n", 
                         pFileName ? pFileName : "NULL");

    if( pOut != NULL && pFileName != NULL )
    {
        COMMONUTILITIES_INFO( "GetFileContents: pFileName = %s\n", pFileName );
        if( (len=(size_t)getFileSize( pFileName )) != -1 )
        {
            COMMONUTILITIES_INFO( "GetFileContents: file len = %zu\n", len );
            if( (fp=fopen( pFileName, "r" )) != NULL )
            {
                ++len;  // room for NULL, included in return value
                pBuf = malloc( len );
                if( pBuf != NULL )
                {
                    pPtr = pBuf;
                    while( ((*pPtr=(char)fgetc( fp )) != EOF) && !feof( fp ) )
                    {
                        ++pPtr;
                    }
                    *pPtr = 0;
                    COMMONUTILITIES_DEBUG("GetFileContents: Successfully read file, length %zu\n", len-1);
                }
                else
                {
                    len = 0;
                    COMMONUTILITIES_ERROR("GetFileContents: malloc failed for size %zu\n", len);
                }
                fclose( fp );
            }
            else
            {
                COMMONUTILITIES_ERROR("GetFileContents: Failed to open file %s\n", pFileName);
                len = 0;
            }
        }
        else
        {
            COMMONUTILITIES_ERROR("GetFileContents: getFileSize failed for %s\n", pFileName);
        }
        *pOut = pBuf;
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetFileContents: Error, input argument NULL\n" );
    }
    return len;
}

/* function makeHttpHttps - converts http:// URLs to https:// in place.

        Usage: size_t makeHttpHttps <char *pIn> <size_t szpInSize>

            pIn - pointer to a char buffer containing the URL to modify.

            szpInSize - the size of the character buffer in argument 1.

            RETURN - new length of the string after modification.
*/
size_t makeHttpHttps( char *pIn, size_t szpInSize )
{
    char *pTmp, *pEnd;
    int i;
    size_t len = 0;

    COMMONUTILITIES_DEBUG("makeHttpHttps: Called with buffer size %zu\n", szpInSize);

    if( pIn != NULL && szpInSize )
    {
       pEnd = pIn;
        i = szpInSize - 1;
        while( *pEnd && i )
        {
            --i;
            ++pEnd;     // should be left pointing to '\0' with room to insert
        }
        len = pEnd - pIn;
        if( i )     // check if room to insert
        {
            if( (pTmp = strstr( pIn, "http://" )) != NULL )
            {
                pTmp += 3;
                while( pEnd > pTmp )
                {
                    *(pEnd + 1) = *pEnd;    // copy current char 1 position to right
                    --pEnd;
                }
                ++pTmp;
                *pTmp = 's';
                ++len;
                COMMONUTILITIES_DEBUG("makeHttpHttps: Converted http:// to https://, new length %zu\n", len);
            }
        }
        else
        {
            COMMONUTILITIES_DEBUG("makeHttpHttps: No room to insert 's', buffer too small\n");
        }
    }
    else
    {
        COMMONUTILITIES_ERROR("makeHttpHttps: Error, input argument NULL or size 0\n");
    }
    return len;
}

/* function get_system_uptime - gets the system uptime in seconds.

        Usage: bool get_system_uptime <double *uptime>

            uptime - pointer to a double to store the uptime value.

            RETURN - true if successful, false on error.
*/
bool get_system_uptime(double *uptime) 
{
    FILE* uptime_file = fopen("/proc/uptime", "r");
    
    COMMONUTILITIES_DEBUG("get_system_uptime: Called\n");
    
    if ((uptime_file != NULL) && (uptime != NULL)) {
        if (fscanf(uptime_file, "%lf", uptime) == 1) {
            fclose(uptime_file);
            COMMONUTILITIES_DEBUG("get_system_uptime: Successfully read uptime %.2f seconds\n", *uptime);
            return true;
        }
    }
    if (uptime_file != NULL) {
        fclose(uptime_file); 
    }
    COMMONUTILITIES_ERROR("get_system_uptime: Failed to read system uptime\n");
    return false;
}

/* Helper function - secure popen wrapper for RunCommand */
static FILE* v_secure_popen(const char* mode, const char* command, ...)
{
    char cmd_buffer[512];
    va_list args;
    
    va_start(args, command);
    vsnprintf(cmd_buffer, sizeof(cmd_buffer), command, args);
    va_end(args);
    
    COMMONUTILITIES_DEBUG("v_secure_popen: Executing command: %s\n", cmd_buffer);
    return popen(cmd_buffer, mode);
}

/* Helper function - secure pclose wrapper for RunCommand */
static int v_secure_pclose(FILE* fp)
{
    return pclose(fp);
}

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
size_t RunCommand( DEVUTILS_SYSCMD eSysCmd, const char *pArgs, char *pResult, size_t szResultSize )
{
    FILE *fp;
    size_t nbytes_read = 0;

    COMMONUTILITIES_INFO("*** CALLING RunCommand FROM COMMON_UTILITIES/LIBFWUTILS ***\n");

    if( pResult != NULL && szResultSize >= 1 )
    {
        *pResult = 0;
        switch( eSysCmd )
        {
           case eMD5Sum :
               if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", MD5SUM, pArgs );
               }
               else
               {
                   fp = NULL;
                   COMMONUTILITIES_ERROR( "RunCommand: Error, md5sum requires an input argument\n" );
               }
               break;

           case eRdkSsaCli :
               if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", RDKSSACLI, pArgs );
               }
               else
               {
                   fp = NULL;
                   COMMONUTILITIES_ERROR( "RunCommand: Error, rdkssacli requires an input argument\n" );
               }
               break;

           case eMfrUtil :
               if( pArgs != NULL )
               {
                   fp = v_secure_popen( "r", MFRUTIL, pArgs );
               }
               else
               {
                   fp = NULL;
                   COMMONUTILITIES_ERROR( "RunCommand: Error, mfr_util requires an input argument\n" );
               }
               break;

           case eWpeFrameworkSecurityUtility :
               fp = v_secure_popen( "r", WPEFRAMEWORKSECURITYUTILITY );
               break;

#ifdef GETRDMMANIFESTVERSION_IN_SCRIPT
           case eGetInstalledRdmManifestVersion :
               fp = v_secure_popen( "r", GETINSTALLEDRDMMANIFESTVERSIONSCRIPT );
               break;
#endif

           default:
               fp = NULL;
               COMMONUTILITIES_ERROR( "RunCommand: Error, unknown request type %d\n", (int)eSysCmd );
               break;
        }

        if( fp != NULL )
        {
            nbytes_read = fread( pResult, 1, szResultSize - 1, fp );
            v_secure_pclose( fp );
            if( nbytes_read != 0 )
            {
                COMMONUTILITIES_INFO( "RunCommand: Successful read %zu bytes\n", nbytes_read );
                pResult[nbytes_read] = '\0';
                nbytes_read = strnlen( pResult, szResultSize ); // fread might include NULL characters, get accurate count
            }
            else
            {
                COMMONUTILITIES_ERROR( "RunCommand: fread fails:%zu\n", nbytes_read );
            }
            COMMONUTILITIES_DEBUG( "RunCommand: output=%s\n", pResult );
        }
        else
        {
            COMMONUTILITIES_ERROR( "RunCommand: Failed to open pipe command execution\n" );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "RunCommand: Error, input argument invalid\n" );
    }
    return nbytes_read;
}

/* function GetPDRIFileName - returns the PDRI for the device. 
        Usage: size_t GetPDRIFileName <char *pPDRIFilename> <size_t szBufSize>
 
            pPDRIFilename - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetPDRIFileName( char *pPDRIFilename, size_t szBufSize )
{
    char *pTmp;
    size_t len = 0;

    if( pPDRIFilename != NULL )
    {
        len = RunCommand( eMfrUtil, "--PDRIVersion", pPDRIFilename, szBufSize );
        if( len && ((pTmp = strcasestr( pPDRIFilename, "failed" )) == NULL) )   // if "failed" is not found
        {
            COMMONUTILITIES_INFO( "GetPDRIFileName: PDRI Version = %s\n", pPDRIFilename );
            // t2ValNotify("PDRI_Version_split", pPDRIFilename); // TODO: Add telemetry support if needed
        }
        else
        {
            *pPDRIFilename = 0;
            len = 0;
            COMMONUTILITIES_ERROR( "GetPDRIFileName: PDRI filename retrieving Failed ...\n" );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetPDRIFileName: Error, input argument NULL\n" );
    }
    return len;
}

/* function BuildRemoteInfo - Formats the "periperalFirmwares" string for remote info part of xconf communication
        Usage: size_t BuildRemoteInfo <JSON *pItem> <char *pRemoteInfo> <size_t szMaxBuf> <bool bAddremCtrl>
 
            pItem - a pointer to a JSON structure that contains the remote info.
            pRemoteInfo - a pointer to a character buffer to store the output
            szMaxBuf - the maximum size of the buffer
            bAddremCtrl - if true then prefix values with &remCtrl. false does not add prefix
            RETURN - the number of characters written to the buffer
*/
size_t BuildRemoteInfo( JSON *pItem, char *pRemoteInfo, size_t szMaxBuf, bool bAddremCtrl )
{
    char **pPrefix;
    char **pMid;
    char *pSuffix;
    size_t szBufRemaining, szRunningLen = 0;
    int iLen = 0;
    int i = 0;
    int x;
    char productBuf[100];
    char versionBuf[50];

    if( pItem != NULL && pRemoteInfo != NULL )
    {
        szBufRemaining = szMaxBuf;
        COMMONUTILITIES_INFO( "BuildRemoteInfo: Start\n" );

        i = GetJsonVal( pItem, "Product", productBuf, sizeof(productBuf) );
        if( i )
        {
            if( bAddremCtrl == true )
            {
                pPrefix = pRemCtrlStrings;
                pMid = pEqualStrings;
                pSuffix = *pNullStrings;
            }
            else
            {
                pPrefix = pNullStrings;
                pMid = pTypeStrings;
                pSuffix = *pExtStrings;
            }

            /*
                Now try to find the json values in the listed in the pPeripheralName array.
                If bAddRemCtrl is true then the output will be formatted similar to the following for each value in the list;
                    &remCtrlXR11-20=1.1.1.1&remCtrlAudioXR11-20=0.1.0.0&remCtrlDspXR11-20=0.1.0.0&remCtrlKwModelXR11-20=0.1.0.0
                Otherwise the output will be formatted similar to the following for each value in the list;
                    XR11-20_firmware_1.1.1.1.tgz,XR11-20_audio_0.1.0.0.tgz,XR11-20_dsp_0.1.0.0.tgz,XR11-20_kw_model_0.1.0.0.tgz
                Note that model name and version numbers are variables depending on the device
            */

            for( x=0; x < MAX_PERIPHERAL_ITEMS; x++ )
            {
                i = GetJsonVal( pItem, pPeripheralName[x], versionBuf, sizeof(versionBuf) );
                if( i )
                {
                    iLen = snprintf( pRemoteInfo + szRunningLen, szBufRemaining, "%s%s%s%s%s", *pPrefix, productBuf, *pMid, versionBuf, pSuffix );
                    if (iLen >= szBufRemaining) {
                        COMMONUTILITIES_INFO( "Buffer is Full\n" );
                        iLen = szBufRemaining;
                        break;
                    }
                    ++pPrefix;
                    ++pMid;
                    szBufRemaining -= iLen;
                    szRunningLen += iLen;
                }
            }
        }
        COMMONUTILITIES_INFO( "BuildRemoteInfo: End\n" );
    }
    else
    {
        COMMONUTILITIES_ERROR( "BuildRemoteInfo: Error, input argument(s) invalid\n" );
    }
    return (size_t)iLen;
}

/* function GetRemoteInfo - gets the remote info of the device.
        Usage: size_t GetRemoteInfo <char *pRemoteInfo> <size_t szBufSize>
 
            pRemoteInfo - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetRemoteInfo( char *pRemoteInfo, size_t szBufSize )
{
    size_t len, sztotlen = 0;
    JSON *pJson, *pItem;
    char *pJsonStr;
    size_t szBufRemaining;
    unsigned index, num;

    if( pRemoteInfo != NULL )
    {
        *pRemoteInfo = 0;
        pJsonStr = GetJson( PERIPHERAL_JSON_FILE );
        if( pJsonStr != NULL )
        {
            szBufRemaining = szBufSize;
            pJson = ParseJsonStr( pJsonStr );
            if( pJson != NULL )
            {
                if( IsJsonArray( pJson ) )
                {
                    num = GetJsonArraySize( pJson );
                    for( index = 0; index < num; index++ )
                    {
                        pItem = GetJsonArrayItem( pJson, index );
                        if( pItem != NULL )
                        {
                            len = BuildRemoteInfo( pItem, pRemoteInfo + sztotlen, szBufRemaining, true );
                            sztotlen += len;
                            if( len <= szBufRemaining ) // make sure not to roll value over
                            {
                                szBufRemaining -= len;
                            }
                            if( szBufRemaining <= 1 )    // if it's 1 then buf is full since NULL isn't counted
                            {
                                COMMONUTILITIES_INFO( "%s: WARNING, buffer is full and will be truncated, sztotlen=%zu\n", __FUNCTION__, sztotlen );
                                break;
                            }
                        }
                    }
                }
                else
                {
                    sztotlen = BuildRemoteInfo( pJson, pRemoteInfo, szBufSize, true );
                }
                FreeJson( pJson );
            }
            free( pJsonStr );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetRemoteInfo: Error, input argument NULL\n" );
    }
    COMMONUTILITIES_INFO( "%s: returning sztotlen=%zu\n", __FUNCTION__, sztotlen );
    return sztotlen;
}

/* function GetInstalledBundles - gets the bundles installed on a device. 
        Usage: size_t GetInstalledBundles <char *pBundles> <size_t szBufSize>
 
            pBundles - pointer to a char buffer to store the output string.
            szBufSize - the size of the character buffer in argument 1.
            RETURN - number of characters copied to the output buffer.
*/
size_t GetInstalledBundles(char *pBundles, size_t szBufSize)
{
    JSON *pJsonTop;
    JSON *pJson;
    char *pJsonStr;
    size_t len;
    size_t szRunningLen = 0;
    metaDataFileList_st *installedBundleListNode = NULL, *tmpNode = NULL;

    if (pBundles != NULL)
    {
        *pBundles = 0;
        installedBundleListNode = getInstalledBundleFileList();

        while (installedBundleListNode != NULL)
        {
            COMMONUTILITIES_INFO("GetInstalledBundles: calling GetJson with arg = %s\n", installedBundleListNode->fileName);
            pJsonStr = GetJson(installedBundleListNode->fileName);
            if (pJsonStr != NULL)
            {
                COMMONUTILITIES_INFO("GetInstalledBundles: pJsonStr = %s\n", pJsonStr);
                pJsonTop = ParseJsonStr(pJsonStr);
                COMMONUTILITIES_INFO("GetInstalledBundles: ParseJsonStr returned =%s\n", pJsonStr);
                pJson = pJsonTop;
                if (pJsonTop != NULL)
                {
                    while (pJson != NULL)
                    {
                        len = GetJsonVal(pJson, "name", installedBundleListNode->fileName, sizeof(installedBundleListNode->fileName));
                        if (len)
                        {
                            if (szRunningLen)
                            {
                                *(pBundles + szRunningLen) = ',';
                                ++szRunningLen;
                            }
                            len = snprintf(pBundles + szRunningLen, szBufSize - szRunningLen, "%s:", installedBundleListNode->fileName);
                            szRunningLen += len;
                            len = GetJsonVal(pJson, "version", installedBundleListNode->fileName, sizeof(installedBundleListNode->fileName));
                            len = snprintf(pBundles + szRunningLen, szBufSize - szRunningLen, "%s", installedBundleListNode->fileName);
                            COMMONUTILITIES_INFO("Updated Bundles = %s\n", pBundles);
                            szRunningLen += len;
                        }
                        pJson = pJson->next; // need "next" getter function
                    }
                    FreeJson(pJsonTop);
                }
                free(pJsonStr);
            }
            tmpNode = installedBundleListNode;
            installedBundleListNode = installedBundleListNode->next;
            free(tmpNode);
        }
        COMMONUTILITIES_INFO("GetInstalledBundles: pBundles = %s\n", pBundles);
        COMMONUTILITIES_INFO("GetInstalledBundles: szRunningLen = %zu\n", szRunningLen);
    }

    return szRunningLen;
}

/* function getInstalledBundleFileList - gets the list of bundles installed on a device.
        Usage: metaDataFileList_st *getInstalledBundleFileList()
        Input : void
        RETURN - List of installed Bundle in NVM and RFS directory
*/
metaDataFileList_st *getInstalledBundleFileList()
{
    metaDataFileList_st *metadataNVMls = NULL, *metadataRFSls = NULL, *metaDataList = NULL;

    metadataNVMls = getMetaDataFile(BUNDLE_METADATA_NVM_PATH);
    if (metadataNVMls == NULL)
    {
        COMMONUTILITIES_INFO("Certificate does not exist in NVM Path\n");
    }

    metadataRFSls = getMetaDataFile(BUNDLE_METADATA_RFS_PATH);
    if (metadataRFSls == NULL)
    {
        COMMONUTILITIES_INFO("Certificate does not exist in RFS Path\n");
    }

    if ((metadataNVMls == NULL) && (metadataRFSls == NULL))
    {
        COMMONUTILITIES_INFO("No metadata found only in CPE");
    }
    else if ((metadataNVMls) && (metadataRFSls == NULL))
    {
        metaDataList = metadataNVMls;
        COMMONUTILITIES_INFO("No metadata found only in %s\n", BUNDLE_METADATA_NVM_PATH);
    }
    else if ((metadataNVMls == NULL) && (metadataRFSls))
    {
        metaDataList = metadataRFSls;
        COMMONUTILITIES_INFO("No metadata found only in %s\n", BUNDLE_METADATA_RFS_PATH);
    }
    else if ((metadataNVMls) && (metadataRFSls))
    {
        metaDataList = mergeLists(metadataNVMls, metadataRFSls);
    }

    return metaDataList;
}

/* function getMetaDataFile - gets the files list in the directory
        Usage: metaDataFileList_st *getMetaDataFile(char *dir)
        dir : directory of NVM or RFS Path
        RETURN - List of installed Bundle in NVM or RFS directory
*/
static metaDataFileList_st *getMetaDataFile(char *dir)
{
    metaDataFileList_st *newnode = NULL, *prevnode = NULL, *headNode = NULL;
    struct dirent *pDirent = NULL;

    DIR *directory = opendir(dir);
    if (directory)
    {
        while ((pDirent = readdir(directory)) != NULL)
        {
            if (pDirent->d_type == DT_REG && strstr(pDirent->d_name, "_package.json") != NULL)
            {
                newnode = (metaDataFileList_st *)malloc(sizeof(metaDataFileList_st));
                COMMONUTILITIES_INFO("GetInstalledBundles: found %s\n", pDirent->d_name);
                snprintf(newnode->fileName, sizeof(newnode->fileName), "%s/%s", dir, pDirent->d_name);
                newnode->next = NULL;
                if (headNode == NULL)
                {
                    headNode = newnode;
                    prevnode = headNode;
                }
                else
                {
                    prevnode->next = newnode;
                    prevnode = newnode;
                }
            }
        }
        closedir(directory);
    }
    else
    {
        COMMONUTILITIES_INFO("%s does not exist\n", dir);
    }
    return headNode;
}

/* function mergeLists - merge the RFS and NVM file list.
        Usage: metaDataFileList_st * mergeLists(metaDataFileList_st *nvmList, metaDataFileList_st *rfsList)
        nvmList : NVM files list
        rfsList : RFS files list
        RETURN - common files list of installed Bundle in NVM and RFS directory
*/
static metaDataFileList_st * mergeLists(metaDataFileList_st *nvmList, metaDataFileList_st *rfsList)
{
   metaDataFileList_st  tmp;
   metaDataFileList_st *currentNVMNode = nvmList;
   metaDataFileList_st *currentRFSNode = rfsList;
   metaDataFileList_st *metaDataList = &tmp;
   metaDataFileList_st *next = NULL, *removeDup = NULL;
   int cmpval = 0;
  
  tmp.next = NULL;
  while(currentRFSNode && currentNVMNode)
  {
      cmpval = strncmp(currentRFSNode->fileName,currentNVMNode->fileName, sizeof(currentRFSNode->fileName));
      next = (cmpval < 0) ? currentRFSNode : currentNVMNode;

      metaDataList->next = next;
      metaDataList = next;
     
      if (cmpval < 0) 
      {
          currentRFSNode = currentRFSNode->next;
      } 
      else 
      {
          currentNVMNode = currentNVMNode->next;
      }
   }

  metaDataList->next = currentRFSNode ? currentRFSNode : currentNVMNode;

  removeDup = tmp.next;
  
  while(removeDup)
  {
      if(removeDup->next && strncmp(removeDup->fileName, removeDup->next->fileName, sizeof(removeDup->fileName)) == 0)
      {
          removeDup->next = removeDup->next->next;
      }
      removeDup = removeDup->next;
  }
  
  return tmp.next;
}

/* function GetAdditionalFwVerInfo - returns the PDRI filename plus Remote Info for the device. 
        Usage: size_t GetAdditionalFwVerInfo <char *pAdditionalFwVerInfo> <size_t szBufSize>
 
            pAdditionalFwVerInfo - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetAdditionalFwVerInfo( char *pAdditionalFwVerInfo, size_t szBufSize )
{
    size_t len = 0;

    COMMONUTILITIES_INFO("*** CALLING GetAdditionalFwVerInfo FROM COMMON_UTILITIES/LIBFWUTILS ***\n");

    if( pAdditionalFwVerInfo != NULL )
    {
        len = GetPDRIFileName( pAdditionalFwVerInfo, szBufSize );
	if( len < szBufSize )
        {
            len += GetRemoteInfo( (pAdditionalFwVerInfo + len), (szBufSize - len) );
        }
    }
    else
    {
        COMMONUTILITIES_ERROR( "GetAdditionalFwVerInfo: Error, input argument NULL\n" );
    }

    return len;
}

/* function GetRemoteVers - gets the peripheral versions of the device.
 
        Usage: size_t GetRemoteVers <char *pRemoteVers > <size_t szBufSize>
 
            pRemoteVers - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
        (this is identical to GetRemoteInfo except there is no prefix to the string)
*/
size_t GetRemoteVers( char *pRemoteVers , size_t szBufSize )
{
    size_t len = 0;
    JSON *pJson, *pItem;
    char *pJsonStr;
    unsigned index, num;

    COMMONUTILITIES_INFO("*** CALLING GetRemoteVers FROM COMMON_UTILITIES/LIBFWUTILS ***\n");

    if( pRemoteVers  != NULL )
    {
        *pRemoteVers  = 0;
        pJsonStr = GetJson( PERIPHERAL_JSON_FILE );
        if( pJsonStr != NULL )
        {
            pJson = ParseJsonStr( pJsonStr );
            if( pJson != NULL )
            {
                if( IsJsonArray( pJson ) )
                {
                    num = GetJsonArraySize( pJson );
                    for( index = 0; index < num; index++ )
                    {
                        pItem = GetJsonArrayItem( pJson, index );
                        if( pItem != NULL )
                        {
                            len += BuildRemoteInfo( pItem, pRemoteVers  + len, szBufSize - len, false );
                        }
                    }
                }
                else
                {
                    len = BuildRemoteInfo( pJson, pRemoteVers , szBufSize, false );
                }
                FreeJson( pJson );
            }
            free( pJsonStr );
        }
    }
    else
    {
        COMMONUTILITIES_INFO( "GetRemoteVers: Error, input argument NULL\n" );
    }

    return len;
}

/* function GetRdmManifestVersion - gets the remote info of the device.
 
        Usage: size_t GetRdmManifestVersion <char *pRdmManifestVersion> <size_t szBufSize>
 
            pRdmManifestVersion - pointer to a char buffer to store the output string.

            szBufSize - the size of the character buffer in argument 1.

            RETURN - number of characters copied to the output buffer.
*/
size_t GetRdmManifestVersion( char *pRdmManifestVersion, size_t szBufSize )
{
    size_t len = 0;

    COMMONUTILITIES_INFO("*** CALLING GetRdmManifestVersion FROM COMMON_UTILITIES/LIBFWUTILS ***\n");

#ifdef GETRDMMANIFESTVERSION_IN_SCRIPT
    if( pRdmManifestVersion != NULL )
    {
	*pRdmManifestVersion = 0;
        len = RunCommand( eGetInstalledRdmManifestVersion, NULL, pRdmManifestVersion, szBufSize );
    }
    else
    {
        COMMONUTILITIES_INFO( "GetRdmManifestVersion: Error, input argument NULL\n" );
    }
#else
    // TODO: Add C implemementation
#endif
    return len;
}
