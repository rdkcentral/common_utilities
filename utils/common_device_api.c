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





#define PERIPHERAL_JSON_FILE            "/opt/persistent/rdm/peripheral.json"
#define MAX_PERIPHERAL_ITEMS 4

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





