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

#include <time.h>
#include "common_device_api.h"
#include "rdkv_cdl_log_wrapper.h"

#define PARTNERID_INFO_FILE "/tmp/partnerId.out"

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
