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

#include <ctype.h>
#include <stddef.h>
//#include <unistd.h>
#include "rdk_fwdl_utils.h"

char *PropFiles[] = {
    DEVICE_PROPERTIES_FILE,
    INCLUDE_PROPERTIES_FILE,
    NULL
};

/* Keep this array indexed correctly with PROPVAL below */
const char *PropNames[] = {
    "BUILD_TYPE=",
    "DEVICE_NAME=",
    "DEVICE_TYPE=",
    "DIFW_PATH=",
    "LOG_PATH=",
    "PERSISTENT_PATH=",
    "ENABLE_MAINTENANCE=",
    "FORCE_MTLS=",
    "MODEL_NUM=",
    "ENABLE_SOFTWARE_OPTOUT=",
    NULL
};

/* Keep this enum indexed correctly with PropNames above */
typedef enum {
    eBUILD_TYPE = 0,
    eDEVICE_NAME,
    eDEVICE_TYPE,
    eDIFW_PATH,
    eLOG_PATH,
    ePERSISTENT_PATH,
    eENABLE_MAINTENANCE,
    eFORCE_MTLS,
    eMODEL_NUM,
    eENABLE_SOFTWARE_OPTOUT,
    eEND_OF_LIST
} PROPVALS;

size_t DevicePropertyIndexes[] = {
    offsetof( DeviceProperty_t, eBuildType ),
    offsetof( DeviceProperty_t, dev_name ),
    offsetof( DeviceProperty_t, dev_type ),
    offsetof( DeviceProperty_t, difw_path ),
    offsetof( DeviceProperty_t, log_path ),
    offsetof( DeviceProperty_t, persistent_path ),
    offsetof( DeviceProperty_t, maint_status ),
    offsetof( DeviceProperty_t, mtls ),
    offsetof( DeviceProperty_t, model ),
    offsetof( DeviceProperty_t, sw_optout )
};

DeviceProperty_t *pDeviceInfo;  // not used except to fill in the Size array below

size_t DevicePropertySizes[] = {
    sizeof( pDeviceInfo->eBuildType ),
    sizeof( pDeviceInfo->dev_name ),
    sizeof( pDeviceInfo->dev_type ),
    sizeof( pDeviceInfo->difw_path ),
    sizeof( pDeviceInfo->log_path ),
    sizeof( pDeviceInfo->persistent_path ),
    sizeof( pDeviceInfo->maint_status ),
    sizeof( pDeviceInfo->mtls ),
    sizeof( pDeviceInfo->model ),
    sizeof( pDeviceInfo->sw_optout )
};


BUILDTYPE getbuild( char *pBldStr )
{
    BUILDTYPE eBldType;
    char *pTmp;
    int i;
    char buf[10];

    i = sizeof(buf);
    pTmp = buf;
    while( *pBldStr && --i )
    {
        *pTmp = toupper((unsigned char)*pBldStr);
        ++pBldStr;
        ++pTmp;
    }
    buf[sizeof(buf)-1] = 0;             // force NULL terminator so strstr never can overrun
    if( (pTmp=strstr( buf, "PROD")) != NULL )
    {
        eBldType = ePROD;
    }
    else if( (pTmp=strstr( buf, "VBN")) != NULL )
    {
        eBldType = eVBN;
    }
    else if( (pTmp=strstr( buf, "DEV")) != NULL )
    {
        eBldType = eDEV;
    }
    else if( (pTmp=strstr( buf, "QA")) != NULL )
    {
        eBldType = eDEV;
    }
    else
    {
        eBldType = eUNKNOWN;
    }
//    SWLOG_INFO("getbuild: eBldType = %d\n", (int)eBldType );
    return eBldType;
}

/** Description: Getting device property by reading device.property file
 * @param: device_info : structure of required device info field
 * @return int success 1 and fail -1
 * */
int getDeviceProperties(DeviceProperty_t *pDevice_info) {
    FILE *fp;
    char *pTmp, *pDest;
    PROPVALS i;
    int a;
    int ret = UTILS_FAIL;
    char buf[MAX_DEVICE_PROP_BUFF_SIZE];

    if( pDevice_info != NULL )      // not to be confused with pDeviceInfo  !!!
    {
        ret = UTILS_SUCCESS;
        pDevice_info->eBuildType = eUNKNOWN;
        pDevice_info->dev_name[0] = 0;
        pDevice_info->dev_type[0] = 0;
        pDevice_info->difw_path[0] = 0;
        pDevice_info->log_path[0] = 0;
        pDevice_info->persistent_path[0] = 0;
        pDevice_info->maint_status[0] = 0;
        pDevice_info->mtls[0] = 0;
        pDevice_info->model[0] = 0;
        pDevice_info->sw_optout[0] = 0;
        for( a=0; PropFiles[a] != NULL; a++ )   // check all property files in list
        {
            if( (fp=fopen( PropFiles[a], "r" )) != NULL )
            {
                while( fgets( buf, sizeof(buf), fp ) != NULL )
                {
                    pTmp = buf;
                    while( *pTmp && isblank( *pTmp ) )   // skip over tabs and spaces to fist char, make sure not NULL
                    {
                        COMMONUTILITIES_INFO("getDeviceProperties: skipping spaces\n" );
                        ++pTmp;
                    }
                    if( *pTmp == '#' || *pTmp == 0 )
                    {
                        continue;       // comment line or no value, go read the next line
                    }
                    i=eBUILD_TYPE;
                    while( PropNames[i] != NULL && i < eEND_OF_LIST )
                    {
                        pDest = ((char*)pDevice_info) + DevicePropertyIndexes[i]; // pDest points to start of each member in pDevice_info structure
                        if( !*pDest || ( i == eBUILD_TYPE && pDevice_info->eBuildType == eUNKNOWN ) )   // if a value hasn't been filled in yet then write it, otherwise skip
                        {
                            pTmp=strstr( buf, PropNames[i] );
                            if( pTmp && pTmp == buf )   // if match found and match is first character on line
                            {
                                pTmp = strchr( pTmp, '=' );
 				if( pTmp)
				{
                                    ++pTmp;
				}
				else
				{
				    continue;
				}                               
                                if( i == eBUILD_TYPE )
                                {
                                    pDevice_info->eBuildType = getbuild( pTmp );
                                }
                                else
                                {
                                    snprintf( pDest, DevicePropertySizes[i], "%s", pTmp );
                                    if( (pTmp=strchr( pDest, '\n' )) != NULL )
                                    {
                                        *pTmp = 0;
                                    }
                                }
                                break;
                            }
                        }
                        ++i;
                    }
                }
                fclose( fp );
            }
            else
            {
                COMMONUTILITIES_ERROR( "%s : cannot open %s\n", __FUNCTION__, PropFiles[a] );
            }
        }
//        SWLOG_INFO( "pDevice_info->eBuildType = %d\n", (int)pDevice_info->eBuildType );;
//        SWLOG_INFO( "pDevice_info->dev_name = %s\n", pDevice_info->dev_name );
//        SWLOG_INFO( "pDevice_info->dev_type = %s\n", pDevice_info->dev_type );
//        SWLOG_INFO( "pDevice_info->difw_path = %s\n", pDevice_info->difw_path );
//        SWLOG_INFO( "pDevice_info->log_path = %s\n", pDevice_info->log_path );
//        SWLOG_INFO( "pDevice_info->persistent_path = %s\n", pDevice_info->persistent_path );
//        SWLOG_INFO( "pDevice_info->maint_status = %s\n", pDevice_info->maint_status );
//        SWLOG_INFO( "pDevice_info->mtls = %s\n", pDevice_info->mtls );
//        SWLOG_INFO( "pDevice_info->model = %s\n", pDevice_info->model );
//        SWLOG_INFO( "pDevice_info->sw_optout = %s\n", pDevice_info->sw_optout );
//        sleep(2);
    }
    else
    {
        COMMONUTILITIES_ERROR( "%s : parameter is NULL\n", __FUNCTION__ );
    }

    return ret;
}

/** Description: Getting device property by reading device.property file
 * @param: dev_prop_name : Device property name to get from file
 * @param: out_data : pointer to hold the device property get from file
 * @param: buff_size : Buffer size of the out_data.
 * @return int: Success: UTILS_SUCCESS  and Fail : UTILS_FAIL
 * */
int getDevicePropertyData(const char *dev_prop_name, char *out_data, unsigned int buff_size)
{
    int ret = UTILS_FAIL;
    FILE *fp;
    char tbuff[MAX_DEVICE_PROP_BUFF_SIZE];
    char *tmp;
    int index;

    if (out_data == NULL || dev_prop_name == NULL) {
        COMMONUTILITIES_ERROR("%s : parameter is NULL\n", __FUNCTION__);
        return ret;
    }
    if (buff_size == 0 || buff_size > MAX_DEVICE_PROP_BUFF_SIZE) {
        COMMONUTILITIES_ERROR("%s : buff size not in the range. size should be < %d\n", __FUNCTION__, MAX_DEVICE_PROP_BUFF_SIZE);
        return ret;
    }
    COMMONUTILITIES_INFO("%s : Trying device property data for %s and buf size=%u\n", __FUNCTION__, dev_prop_name, buff_size);
    fp = fopen(DEVICE_PROPERTIES_FILE, "r");
    if(fp == NULL) {
        COMMONUTILITIES_ERROR("%s : device.property File not found\n", __FUNCTION__);
        return ret;
    }
    while((fgets(tbuff, sizeof(tbuff), fp) != NULL)) {
        if(strstr(tbuff, dev_prop_name)) {
            index = strcspn(tbuff, "\n");
            if (index > 0) {
                tbuff[index] = '\0';
            }
            tmp = strchr(tbuff, '=');
            if(tmp != NULL) {
                snprintf(out_data, buff_size, "%s", tmp+1);
                COMMONUTILITIES_INFO("%s : %s=%s\n", __FUNCTION__, dev_prop_name, out_data);
                ret = UTILS_SUCCESS;
                break;
            } else {
                COMMONUTILITIES_ERROR("%s : strchr failed. '=' not found. str=%s\n", __FUNCTION__, tbuff);
	    }
	}
    }
    fclose(fp);
    return ret;
}

/** Description: Getting device property by reading include.property file
 * @param: dev_prop_name : Device property name to get from file
 * @param: out_data : pointer to hold the device property get from file
 * @param: buff_size : Buffer size of the out_data.
 * @return int: Success: UTILS_SUCCESS  and Fail : UTILS_FAIL
 * */
int getIncludePropertyData(const char *dev_prop_name, char *out_data, unsigned int buff_size)
{
    int ret = UTILS_FAIL;
    FILE *fp;
    char tbuff[MAX_DEVICE_PROP_BUFF_SIZE];
    char *tmp;
    int index;

    if (out_data == NULL || dev_prop_name == NULL) {
        COMMONUTILITIES_ERROR("%s : parameter is NULL\n", __FUNCTION__);
        return ret;
    }
    if (buff_size == 0 || buff_size > MAX_DEVICE_PROP_BUFF_SIZE) {
        COMMONUTILITIES_ERROR("%s : buff size not in the range. size should be < %d\n", __FUNCTION__, MAX_DEVICE_PROP_BUFF_SIZE+1);
        return ret;
    }

    fp = fopen(INCLUDE_PROPERTIES_FILE, "r");
    if(fp == NULL) {
        COMMONUTILITIES_ERROR("%s :include.property File not found\n", __FUNCTION__);
        return ret;
    }
    while((fgets(tbuff, sizeof(tbuff), fp) != NULL)) {
        index = strcspn(tbuff, "\n");
        if (index > 0) {
            tbuff[index] = '\0';
        }
        if(strstr(tbuff, dev_prop_name)) {
            tmp = strchr(tbuff, '=');
            if(tmp != NULL) {
                snprintf(out_data, buff_size, "%s", tmp+1);
                COMMONUTILITIES_INFO("%s=%s\n", dev_prop_name, out_data);
                ret = UTILS_SUCCESS;
                break;
            } else {
                COMMONUTILITIES_ERROR("%s : strchr failed. '=' not found. str=%s\n", __FUNCTION__, tbuff);
            }
        }
    }
    fclose(fp);
    return ret;
}


/* Description: Checking device type
 * @param : void
 * @return : mediaclient true and not mediaclient false
 * */
bool isMediaClientDevice(void){
    bool isMediaClientDevice = false ;
    int ret = UTILS_FAIL;
    char *dev_prop_name = "DEVICE_TYPE";
    char dev_type[16];

    // The device type field from device.properties and determine platform type
    ret = getDevicePropertyData(dev_prop_name, dev_type, sizeof(dev_type));
    if (ret == UTILS_SUCCESS) {
        COMMONUTILITIES_INFO("%s: device name from device.property file=%s\n", __FUNCTION__, dev_type);
    } else {
        COMMONUTILITIES_INFO("%s: device name not present device.property file\n", __FUNCTION__);
        return isMediaClientDevice;
    }
    if ((strncmp(dev_type, "mediaclient", 11)) == 0) {
        COMMONUTILITIES_INFO("Device is a Mediaclient\n");
        isMediaClientDevice = true ;
    } else {
        COMMONUTILITIES_INFO("Device is not a Mediaclient\n");
    }
    return isMediaClientDevice ;

}


/* Description: Get image details by reading version.txt file
 * @param : void
 * @return : int 1: Success and -1: Failure
 * */
int getImageDetails(ImageDetails_t *cur_img_detail) {
    FILE *fp;
    char tbuff[80];
    char *tmp;
    int ret = UTILS_FAIL;

    if (cur_img_detail == NULL) {
        COMMONUTILITIES_ERROR("getImageDetails(): Parameter is NULL ret=%d\n", ret);
        return ret;
    }

    fp = fopen(IMAGE_DETAILS, "r");
    if(fp == NULL) {
        COMMONUTILITIES_ERROR("getImageDetails() File not found %s\n", IMAGE_DETAILS);
        return ret;
    }
    while((fgets(tbuff, sizeof(tbuff), fp) != NULL)) {
        tbuff[strcspn(tbuff, "\n")] = '\0';
        tmp = NULL;
        if(strstr(tbuff, "imagename")) {
            tmp = strchr(tbuff, ':');
            if(tmp != NULL) {
                snprintf(cur_img_detail->cur_img_name, sizeof(cur_img_detail->cur_img_name), "%s", tmp + 1);
                break;
            }
        }
    }
    fclose(fp);
    ret = UTILS_SUCCESS;
    return ret;
}
