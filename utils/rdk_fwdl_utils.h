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

#ifndef VIDEO_UTILS_RDK_UTILS_H_
#define VIDEO_UTILS_RDK_UTILS_H_

#include "rdkv_cdl_log_wrapper.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFF_SIZE 512
#define MAX_BUFF_SIZE1 256
#define MIN_BUFF_SIZE 64
#define MIN_BUFF_SIZE1 32
#define SMALL_SIZE_BUFF 8
#define URL_MAX_LEN 512
#define DWNL_PATH_FILE_LEN 128

#define UTILS_SUCCESS 1
#define UTILS_FAIL -1
#define MAX_DEVICE_PROP_BUFF_SIZE 80

#ifndef GTEST_ENABLE
#define DEVICE_PROPERTIES_FILE  "/etc/device.properties"
#define INCLUDE_PROPERTIES_FILE "/etc/include.properties"
#define IMAGE_DETAILS           "/version.txt"
#else
#define DEVICE_PROPERTIES_FILE  "/tmp/device.properties"
#define INCLUDE_PROPERTIES_FILE "/tmp/include.properties"
#define IMAGE_DETAILS           "/tmp/version.txt"
#endif

typedef enum {
    eUNKNOWN,
    eDEV,
    eVBN,
    ePROD,
    eQA
} BUILDTYPE;

#ifndef GETRDMMANIFESTVERSION_IN_SCRIPT
    #define GETRDMMANIFESTVERSION_IN_SCRIPT
#endif

typedef enum {
    eDEVUTILS_MD5Sum,
    eDEVUTILS_RdkSsaCli,
    eDEVUTILS_MfrUtil,
    eDEVUTILS_WpeFrameworkSecurityUtility
#ifdef GETRDMMANIFESTVERSION_IN_SCRIPT
    ,eDEVUTILS_GetInstalledRdmManifestVersion
#endif
} DEVUTILS_SYSCMD;

/* Below structure contains data from /etc/device.property */
typedef struct deviceproperty {
        BUILDTYPE eBuildType;           // keep buildtype as an enum, easier to compare
        char dev_name[MIN_BUFF_SIZE1];
        char dev_type[MIN_BUFF_SIZE1];
        char difw_path[MIN_BUFF_SIZE1];
        char log_path[MIN_BUFF_SIZE1];
        char persistent_path[MIN_BUFF_SIZE1];
        char maint_status[MIN_BUFF_SIZE1];
        char mtls[MIN_BUFF_SIZE1];
        char model[MIN_BUFF_SIZE1];
        char sw_optout[MIN_BUFF_SIZE1];
}DeviceProperty_t;

/* Below structure contails data from /version.txt */
typedef struct imagedetails {
        char cur_img_name[MIN_BUFF_SIZE];
}ImageDetails_t;

int getDeviceProperties(DeviceProperty_t *pDevice_info);

/**
 * Returns true if device is mediaclient device
 */
bool isMediaClientDevice(void);

/**
 * Get current firmware version
 */
int getImageDetails(ImageDetails_t *);

int getIncludePropertyData(const char *dev_prop_name, char *data, unsigned int buff_size);
int getDevicePropertyData(const char *dev_prop_name, char *out_data, unsigned int buff_size);

#endif /* VIDEO_UTILS_RDK_UTILS_H_ */
