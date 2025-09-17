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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <unistd.h>

extern "C" {
#include "downloadUtil.h"
#include "urlHelper.h"
}
#include "mocks/curl_mock.h"

#define GTEST_DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define GTEST_DEFAULT_RESULT_FILENAME "CommonUtilities_urlHelper_gtest_report.json"
#define GTEST_REPORT_FILEPATH_SIZE 256

using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::StrEq;

extern "C" {
    long (*getperformRequest(void))(CURL *, CURLcode *);
}

extern "C" {
    int (*getxferinfo(void)) (void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow); 
}

extern "C" {
    size_t (*getdownload_func(void)) (void* ptr, size_t size, size_t nmemb, void* stream);
}

extern "C" {
    size_t (*getWriteMemoryCB(void)) ( void *pvContents, size_t szOneContent, size_t numContentItems, void *userp ); 
}

extern "C" {
    size_t (*getheader_callback(void)) (char *buffer, size_t size, size_t nitems, void *userdata);
}

CurlWrapperMock *g_CurlWrapperMock = NULL;

class urlHelperTestFixture : public ::testing::Test {
	protected:

        CurlWrapperMock mockCurlWrapper;

        urlHelperTestFixture()
        {
            g_CurlWrapperMock = &mockCurlWrapper;
        }
        virtual ~urlHelperTestFixture()
        {
            g_CurlWrapperMock = NULL;
        }

	virtual void SetUp()
        {
            printf("%s\n", __func__);
        }

        virtual void TearDown()
        {
            printf("%s\n", __func__);
        }

        static void SetUpTestCase()
        {
            printf("%s\n", __func__);
        }

        static void TearDownTestCase()
        {
            printf("%s\n", __func__);
        }
    };

/*1.urlHelperInit*/
  /*curl_global_init is called and unit testcase is not needed*/

/*2.urlHelperCreateCurl*/
  /*curl_easy_init is called and unit testcase is not needed*/

/*3.urlHelperDestroyCurl*/
TEST_F(urlHelperTestFixture, urlHelperDestroyCurl_NULL_curl)
{
    urlHelperDestroyCurl(NULL);
}
TEST_F(urlHelperTestFixture, urlHelperDestroyCurl_curl_Not_NULL)
{
    void *curl = NULL;
    curl = doCurlInit();
    urlHelperDestroyCurl(curl);
}

/*4.setForceStop*/
TEST_F(urlHelperTestFixture, setForceStop_integer)
{
    EXPECT_EQ(setForceStop(6), 0);
}

/*5.performRequest*/
TEST_F(urlHelperTestFixture, performRequest_curl_NULL)
{
    auto myFunctionPtr = getperformRequest();
    void *Curl_req = NULL;
    CURLcode curl_status = CURLE_FAILED_INIT;

    long result = myFunctionPtr(Curl_req, &curl_status); // Indirectly calls performRequest
    EXPECT_EQ(result, 0);
}
TEST_F(urlHelperTestFixture, performRequest_return_NULL)
{
    auto myFunctionPtr = getperformRequest();
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    long result = myFunctionPtr(Curl_req, NULL); // Indirectly calls performRequest
    EXPECT_EQ(result, 0);
}
/*
 * Positive testcase is covered as part of below function
 */
TEST_F(urlHelperTestFixture, performRequest_valid_inputs)
{
    auto myFunctionPtr = getperformRequest();
    void *Curl_req = NULL;
    CURLcode curl_status = CURLE_FAILED_INIT;
    Curl_req = doCurlInit();
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));        
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .Times(4)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    long result = myFunctionPtr(Curl_req, &curl_status); // Indirectly calls performRequest
    EXPECT_EQ(result, 0);
    /*
    performRequest(Curl_req, &curl_status);
    EXPECT_EQ(curl_status, CURLE_OK);
    */
}

/*6.urlHelperPutReuqest*/
TEST_F(urlHelperTestFixture, urlHelperPutReuqest_curl_NULL)
{
    int out_httpCode;
    CURLcode curl_status = CURLE_FAILED_INIT;
    void *Curl_req = NULL;

    if (Curl_req != NULL)
    {
        EXPECT_EQ(urlHelperPutReuqest(Curl_req, NULL, &out_httpCode, &curl_status), -1);
    }
}
TEST_F(urlHelperTestFixture, urlHelperPutReuqest_NULL_param3)
{
    CURLcode curl_status = CURLE_FAILED_INIT;
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    if (Curl_req != NULL)
    {
        EXPECT_EQ(urlHelperPutReuqest(Curl_req, NULL, NULL, &curl_status), -1);
    }
}
TEST_F(urlHelperTestFixture, urlHelperPutReuqest_NULL_param4)
{
    int out_httpCode;
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    if (Curl_req != NULL)
    {
        EXPECT_EQ(urlHelperPutReuqest(Curl_req, NULL, &out_httpCode, NULL), -1);
    }
}

TEST_F(urlHelperTestFixture, urlHelperPutReuqest_valid_inputs)
{
    int out_httpCode;
    CURLcode curl_status = CURLE_FAILED_INIT;
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(2)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));        
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .Times(4)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    if (Curl_req != NULL)
    {
        EXPECT_EQ(urlHelperPutReuqest(Curl_req, NULL, &out_httpCode, &curl_status), 0);
    }
}

/*7.printCurlError*/
TEST_F(urlHelperTestFixture, printCurlError_valid_input)
{
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_strerror(_))
	    .Times(2)
	    .WillOnce(Return("Curl error\n"))
	    .WillOnce(Return("Curl error\n"));
    printf("%s\n", printCurlError(CURLE_OK));
}

/*8.setMtlsHeaders*/
TEST_F(urlHelperTestFixture, setMtlsHeaders_sec_NULL)
{
    CURL *curl;
    curl = urlHelperCreateCurl();
    EXPECT_EQ(setMtlsHeaders( curl, NULL),-1);
}
TEST_F(urlHelperTestFixture, setMtlsHeaders_cert_type_p12)
{
    CURL *curl;
    MtlsAuth_t *sec = NULL;
    curl = urlHelperCreateCurl();
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(5)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(setMtlsHeaders( curl, sec),CURLE_OK);
}
TEST_F(urlHelperTestFixture, setMtlsHeaders_cert_type_p14)
{
    CURL *curl;
    MtlsAuth_t *sec = NULL;
    curl = urlHelperCreateCurl();
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P14", 3);
    memcpy(sec->key_pas, "key_pas", 7);
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(5)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(setMtlsHeaders( curl, sec),CURLE_OK);
}

/*9.xferinfo*/
TEST_F(urlHelperTestFixture, xferinfo_file_not_NULL) 
{
    int ret;
    struct curlprogress *newinfo;
    newinfo = (struct curlprogress *) malloc (sizeof(struct curlprogress));
    newinfo->prog_store = fopen("/tmp/file.txt", "w");
    
    auto myFunctionPtr = getxferinfo();
    EXPECT_EQ(myFunctionPtr(newinfo, 2, 3, 4, 5), 0);
    ret = system("rm -rf /tmp/file.txt");
}
TEST_F(urlHelperTestFixture, xferinfo_file_NULL)
{
    struct curlprogress *newinfo;
        newinfo = (struct curlprogress *) malloc (sizeof(struct curlprogress));
    newinfo->prog_store = NULL;

    auto myFunctionPtr = getxferinfo();
    EXPECT_EQ(myFunctionPtr(newinfo, 2, 3, 4, 5), 0);
}

/*10.download_func*/
TEST_F(urlHelperTestFixture, download_func_forcestop_1) //revisit
{
    setForceStop(1);
    auto myFunctionPtr = getdownload_func();
    EXPECT_EQ(myFunctionPtr(NULL,6,3,NULL), 0);
}
TEST_F(urlHelperTestFixture, download_func_valid) 
{
    int ret;
    void* ptr;
    char content[5]="ab";
    size_t size = 2;
    size_t nmemb = 1;
    DownloadData* pdata;
    pdata = (DownloadData *) malloc(sizeof(DownloadData));

    pdata->pvOut=fopen("/tmp/file.txt", "w");
    ptr=content;
    setForceStop(0);
    
    auto myFunctionPtr = getdownload_func();
    EXPECT_EQ(myFunctionPtr(ptr,size,nmemb,pdata), 1);
    free(pdata);
    ret = system("rm -rf /tmp/file.txt");
}
/*11.WriteMemoryCB*/
TEST_F(urlHelperTestFixture, WriteMemoryCB_memory_allocated) 
{
    char out[20] ="outstring";
    char newstring[20] ="newstringisthenew";
    DownloadData* pdata;
    pdata = (DownloadData *) malloc(sizeof(DownloadData));
    pdata->datasize=9;
    pdata->memsize=20;
    pdata->pvOut= malloc(pdata->memsize);
    memcpy(pdata->pvOut, out, pdata->datasize);
   
    auto myFunctionPtr = getWriteMemoryCB(); 
    EXPECT_EQ(myFunctionPtr(newstring, 1, 17, pdata), 17);
}

TEST_F(urlHelperTestFixture, WriteMemoryCB_memory_not_allocated)
{
    char out[20] ="outstring";
    char newstring[20] ="news";
    DownloadData* pdata;
    pdata = (DownloadData *) malloc(sizeof(DownloadData));
    pdata->datasize=9;
    pdata->memsize=20;
    pdata->pvOut= malloc(pdata->memsize);
    memcpy(pdata->pvOut, out, pdata->datasize);

    auto myFunctionPtr = getWriteMemoryCB(); 
    EXPECT_EQ(myFunctionPtr(newstring, 1, 4, pdata), 4);
}

/*12.header_callback*/
TEST_F(urlHelperTestFixture, header_callback_Null_file) /*Source file*/
{
    char output[20] ="outstring";

    auto myFunctionPtr = getheader_callback(); 
    EXPECT_EQ(myFunctionPtr(output,0,9,NULL), 0);
}
TEST_F(urlHelperTestFixture, header_callback_Null_buffer) 
{
    FILE *fp=fopen("/tmp/file.txt", "w");
    auto myFunctionPtr = getheader_callback(); 
    EXPECT_EQ(myFunctionPtr(NULL,1,0,fp), 0);
}
TEST_F(urlHelperTestFixture, header_callback_valid_inputs)
{
    char output[20] ="outstring";
    FILE *fp=fopen("/tmp/file.txt", "w");
    auto myFunctionPtr = getheader_callback(); 
    EXPECT_EQ(myFunctionPtr(output,1,9,fp), 9);
}

/*13.urlHelperGetHeaderInfo*/
TEST_F(urlHelperTestFixture, urlHelperGetHeaderInfo_arg1_NULL)
{
    MtlsAuth_t *sec = NULL;
    char pathname[20] = "/tmp/file.txt";
    int curl_ret_status;
    int httpCode_ret_status;

    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    EXPECT_EQ(urlHelperGetHeaderInfo(NULL, sec, pathname, &httpCode_ret_status, &curl_ret_status ), -1);
}
TEST_F(urlHelperTestFixture, urlHelperGetHeaderInfo_arg5_NULL)
{
    char url[30] = "http://xfinity.com";
    MtlsAuth_t *sec = NULL;
    char pathname[20] = "/tmp/file.txt";
    int httpCode_ret_status;

    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    EXPECT_EQ(urlHelperGetHeaderInfo(url, sec, pathname, &httpCode_ret_status, NULL ), -1);
}
TEST_F(urlHelperTestFixture, urlHelperGetHeaderInfo_arg3_NULL)
{
    char url[30] = "http://xfinity.com";
    MtlsAuth_t *sec = NULL;
    int curl_ret_status;
    int httpCode_ret_status;

    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    EXPECT_EQ(urlHelperGetHeaderInfo(url, sec, NULL, &httpCode_ret_status, &curl_ret_status ), -1);
}
TEST_F(urlHelperTestFixture, urlHelperGetHeaderInfo_arg4_NULL)
{
    char url[30] = "http://xfinity.com";
    MtlsAuth_t *sec = NULL;
    char pathname[20] = "/tmp/file.txt";
    int curl_ret_status;

    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    EXPECT_EQ(urlHelperGetHeaderInfo(url, sec, pathname, NULL, &curl_ret_status ), -1);
}
TEST_F(urlHelperTestFixture, urlHelperGetHeaderInfo_valid_inputs)
{
    char url[30] = "http://xfinity.com"; 
    MtlsAuth_t *sec = NULL;
    char pathname[20] = "/tmp/file.txt";
    int curl_ret_status;
    int httpCode_ret_status;
    
    sec = (MtlsAuth_t *) malloc(sizeof(MtlsAuth_t));
    memcpy(sec->cert_name, "cert_name", 9);
    memcpy(sec->cert_type, "P12", 3);
    memcpy(sec->key_pas, "key_pas", 7);

    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(12)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .Times(4)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));

    EXPECT_EQ(urlHelperGetHeaderInfo(url, sec, pathname, &httpCode_ret_status, &curl_ret_status ), CURLE_OK);
}

/*14.setCommonCurlOpt*/
TEST_F(urlHelperTestFixture, setCommonCurlOpt_valid_inputs)
{
    void *Curl_req = NULL;
    char url[30] = "http://xfinity.com";
    char PostFields[10] = "pf";
    Curl_req = doCurlInit();
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(10)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));
    /*
    EXPECT_CALL(*g_CurlWrapperMock,SetPostFields (_, _))
                .Times(1)
                .WillOnce(Invoke([]( CURL *curl, char *pPostFields ) {
                return CURLE_OK;
        }));
	*/

    EXPECT_EQ(setCommonCurlOpt( Curl_req, url, PostFields, false), CURLE_OK);
}
TEST_F(urlHelperTestFixture, setCommonCurlOpt_arg3_NULL)
{
    void *Curl_req = NULL;
    char url[30] = "http://xfinity.com";
    Curl_req = doCurlInit();
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(8)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(setCommonCurlOpt( Curl_req, url, NULL, false), CURLE_OK);
}
TEST_F(urlHelperTestFixture, setCommonCurlOpt_arg1_NULL)
{
    char url[30] = "http://xfinity.com";

    EXPECT_EQ(setCommonCurlOpt( NULL, url, NULL, false), -1);
}
TEST_F(urlHelperTestFixture, setCommonCurlOpt_arg2_NULL)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    EXPECT_EQ(setCommonCurlOpt( Curl_req, NULL, NULL, false), -1);
}

/*15.setCurlDebugOpt*/
#ifdef CURL_DEBUG  /*Only needed when CURL_DEBUG is defined*/
TEST_F(urlHelperTestFixture, setCurlDebugOpt_valid_inputs)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    DbgData_t *debug;
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(3)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(setCurlDebugOpt(Curl_req, debug), CURLE_OK);
}
#endif

/*16.setCurlProgress*/
TEST_F(urlHelperTestFixture, setCurlProgress_arg1_NULL)
{
    struct curlprogress *newinfo;
    newinfo = (struct curlprogress *) malloc (sizeof(struct curlprogress));

    EXPECT_EQ(setCurlProgress(NULL, newinfo), -1);
}
TEST_F(urlHelperTestFixture, setCurlProgress_arg2_NULL)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    EXPECT_EQ(setCurlProgress(Curl_req, NULL), -1);
}
TEST_F(urlHelperTestFixture, setCurlProgress_valid_inputs)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    struct curlprogress *newinfo;
    newinfo = (struct curlprogress *) malloc (sizeof(struct curlprogress));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(3)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(setCurlProgress(Curl_req, newinfo), CURLE_OK);
}

/*17.setThrottleMode*/
TEST_F(urlHelperTestFixture, setThrottleMode_arg1_NULL)
{
    curl_off_t max_dwnl_speed = 25;

    EXPECT_EQ(setThrottleMode( NULL, max_dwnl_speed), -1);
}
TEST_F(urlHelperTestFixture, setThrottleMode_arg2_lt_0)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    curl_off_t max_dwnl_speed = -1;

    EXPECT_EQ(setThrottleMode(Curl_req, max_dwnl_speed), -1);
}
TEST_F(urlHelperTestFixture, setThrottleMode_valid_inputs)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    curl_off_t max_dwnl_speed = 25;
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(1)
            .WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_strerror(_)).Times(1).WillOnce(Return("Curl error\n"));

    EXPECT_EQ(setThrottleMode(Curl_req, max_dwnl_speed), CURLE_OK);
}

/*18.closeFile*/
TEST_F(urlHelperTestFixture, closeFile_arg1)
{
    int ret;
    DownloadData* pdata;
    pdata = (DownloadData *) malloc(sizeof(DownloadData));

    pdata->pvOut=fopen("/tmp/file.txt", "w");
    closeFile(pdata, NULL, NULL);
    free(pdata);
    ret = system("rm -rf /tmp/file.txt");
}
TEST_F(urlHelperTestFixture, closeFile_arg2)
{
    int ret;
    struct curlprogress *newinfo;
    newinfo = (struct curlprogress *) malloc (sizeof(struct curlprogress));
    newinfo->prog_store = fopen("/tmp/file.txt", "w");
    closeFile(NULL, newinfo, NULL);
    free(newinfo);
    ret = system("rm -rf /tmp/file.txt");
}
TEST_F(urlHelperTestFixture, closeFile_arg3)
{
    int ret;
    FILE *fp = fopen("/tmp/file.txt", "w");
    closeFile(NULL, NULL, fp);
    ret = system("rm -rf /tmp/file.txt");
}
TEST_F(urlHelperTestFixture, closeFile_all_arg)
{
    int ret;
    DownloadData* pdata;
    pdata = (DownloadData *) malloc(sizeof(DownloadData));
    curlprogress *newinfo;
    newinfo = (struct curlprogress *) malloc (sizeof(struct curlprogress));
    newinfo->prog_store = fopen("/tmp/file.txt", "w");
    pdata->pvOut=fopen("/tmp/file2.txt", "w");
    FILE *fp = fopen("/tmp/file3.txt", "w");

    closeFile(pdata, newinfo, fp);
    free(newinfo);
    free(pdata);
    ret = system("rm -rf /tmp/file.txt");
    ret = system("rm -rf /tmp/file2.txt");
    ret = system("rm -rf /tmp/file3.txt");
}
TEST_F(urlHelperTestFixture, closeFile_all_arg_NULL)
{
    closeFile(NULL, NULL, NULL);
}

/*19.urlHelperDownloadFile*/
TEST_F(urlHelperTestFixture, urlHelperDownloadFile)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();

    char pathname[20] = "/tmp/file.txt";
    CURLcode curl_status = -1;
    int httpcode=0;

    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));
    EXPECT_EQ(urlHelperDownloadFile(Curl_req, pathname, NULL, 0, &httpcode, &curl_status), 0);
}
TEST_F(urlHelperTestFixture, urlHelperDownloadFile_NULL_inputs)
{
    EXPECT_EQ(urlHelperDownloadFile(NULL, NULL, NULL, 0, NULL, NULL), 0);
}
TEST_F(urlHelperTestFixture, urlHelperDownloadFile_Curl56_Retry_Success)
{
    void *Curl_req = nullptr;
    Curl_req = doCurlInit();

    char pathname[20] = "/tmp/file.txt";
    CURLcode curl_status = -1;
    int httpcode = 0;

    // Simulate: first call returns CURLE_RECV_ERROR (56), second returns CURLE_OK
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_, _, _))
        .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_, _, _))
        .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_))
        .Times(2)
        .WillOnce(Return(CURLE_RECV_ERROR))  // Simulate error 56
        .WillOnce(Return(CURLE_OK));           // Success on retry

    // urlHelperDownloadFile should retry internally and succeed
    EXPECT_EQ(urlHelperDownloadFile(Curl_req, pathname, (char*)"0", 0, &httpcode, &curl_status), 0);
}


/*20.urlHelperDownloadToMem*/
TEST_F(urlHelperTestFixture, urlHelperDownloadToMem_pDlHeaderData_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    CURLcode curl_status = CURLE_FAILED_INIT;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData->datasize = 7;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));

    EXPECT_EQ(urlHelperDownloadToMem(Curl_req, &req_data, &httpCode, &curl_status), CURLE_OK);
}

TEST_F(urlHelperTestFixture, urlHelperDownloadToMem_pDlHeaderData_Not_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    CURLcode curl_status = CURLE_FAILED_INIT;
    char header[64]  = "Content-Type: application/json";
    char pvout[256];
    DownloadData dData;
    dData.datasize = 7;

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;

    req_data.pDlHeaderData = &dData;
    req_data.pDlData = &dData;
    req_data.pDlHeaderData->pvOut = malloc(sizeof(pvout));
    req_data.pDlData->pvOut = malloc(sizeof(pvout));
    *((char *)req_data.pDlHeaderData->pvOut) = pvout;
    *((char *)req_data.pDlData->pvOut) = pvout;
    req_data.pPostFields = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));

    EXPECT_EQ(urlHelperDownloadToMem(Curl_req, &req_data, &httpCode, &curl_status), CURLE_OK);
}
TEST_F(urlHelperTestFixture, urlHelperDownloadToMem_curl_NULL)
{
    FileDwnl_t req_data;
    int httpCode = 0;
    CURLcode curl_status = CURLE_FAILED_INIT;
    char header[64]  = "Content-Type: application/json";

    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(urlHelperDownloadToMem(NULL, &req_data, &httpCode, &curl_status), CURLE_OK);
}
TEST_F(urlHelperTestFixture, urlHelperDownloadToMem_reqdata_NULL)
{
    void *Curl_req = NULL;
    int httpCode = 0;
    CURLcode curl_status = CURLE_FAILED_INIT;

    Curl_req = doCurlInit();

    EXPECT_EQ(urlHelperDownloadToMem(Curl_req, NULL, &httpCode, &curl_status), CURLE_OK);
}
TEST_F(urlHelperTestFixture, urlHelperDownloadToMem_httpCode_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    CURLcode curl_status = CURLE_FAILED_INIT;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(urlHelperDownloadToMem(Curl_req, &req_data, NULL, &curl_status), CURLE_OK);
}
TEST_F(urlHelperTestFixture, urlHelperDownloadToMem_curlStatus_NULL)
{
    FileDwnl_t req_data;
    void *Curl_req = NULL;
    int httpCode = 0;
    char header[64]  = "Content-Type: application/json";

    Curl_req = doCurlInit();
    req_data.pHeaderData = header;
    req_data.pDlHeaderData = NULL;
    req_data.pPostFields = NULL;
    req_data.pDlData = NULL;
    snprintf(req_data.url, sizeof(req_data.url), "%s", "http://127.0.0.1:9998/jsonrpc");

    EXPECT_EQ(urlHelperDownloadToMem(Curl_req, &req_data, &httpCode, NULL), CURLE_OK);
}

/*21.SetRequestHeaders*/
TEST_F(urlHelperTestFixture, SetRequestHeaders_arg1_NULL)
{
    struct curl_slist * list;
    list = (struct curl_slist *)malloc(sizeof(struct curl_slist));
    char header[20] = "header";
    EXPECT_EQ(SetRequestHeaders(NULL, list, header), list);
    free(list);
}
TEST_F(urlHelperTestFixture, SetRequestHeaders_arg3_NULL)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    struct curl_slist * list;
    list = (struct curl_slist *)malloc(sizeof(struct curl_slist));
    EXPECT_EQ(SetRequestHeaders(Curl_req, list, NULL), list);
    free(list);
}
TEST_F(urlHelperTestFixture, SetRequestHeaders_list_not_NULL)
{
    void *Curl_req = NULL;
    char header[20] = "header";
    Curl_req = doCurlInit();
    struct curl_slist * list;
    list = (struct curl_slist *)malloc(sizeof(struct curl_slist));
    list->data = header;
    list->next = NULL;
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(1)
            .WillOnce(Return(CURLE_OK));

    EXPECT_NE(SetRequestHeaders(Curl_req, list, header), nullptr);
    free(list);
}
TEST_F(urlHelperTestFixture, SetRequestHeaders_list_NULL)
{
    void *Curl_req = NULL;
    char header[20] = "header";
    Curl_req = doCurlInit();
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(1)
            .WillOnce(Return(CURLE_OK));
    EXPECT_NE(SetRequestHeaders(Curl_req, NULL, header), nullptr);
}

/*22.SetPostFields*/
TEST_F(urlHelperTestFixture, SetPostFields_arg1_NULL)
{
    char postfields[20] = "p12";
    EXPECT_EQ(SetPostFields(NULL, postfields), CURLE_OK);
}
TEST_F(urlHelperTestFixture, SetPostFields_arg2_NULL)
{
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    EXPECT_EQ(SetPostFields(Curl_req, NULL), CURLE_OK);
}
TEST_F(urlHelperTestFixture, SetPostFields_valid_inputs)
{
    char postfields[20] = "p12";
    void *Curl_req = NULL;
    Curl_req = doCurlInit();
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .Times(2)
            .WillOnce(Return(CURLE_OK))
            .WillOnce(Return(CURLE_OK));

    EXPECT_EQ(SetPostFields(Curl_req, postfields), CURLE_OK);
}

/*23.allocDowndLoadDataMem*/
TEST_F(urlHelperTestFixture, allocDowndLoadDataMem_validinput)
{
    DownloadData DwnLoc;
    DwnLoc.pvOut = NULL;
    DwnLoc.datasize = 0;
    DwnLoc.memsize = 0;

    EXPECT_EQ(allocDowndLoadDataMem(&DwnLoc, 1024), 0);
}
TEST_F(urlHelperTestFixture, allocDowndLoadDataMem_NULL_input)
{
    EXPECT_EQ(allocDowndLoadDataMem(NULL, 1024), 1);
}

/*24.checkDeviceInternetConnection*/
TEST_F(urlHelperTestFixture, checkDeviceInternetConnection_valid)
{
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .WillOnce(Invoke([](CURL *curl, CURLINFO info, void *param){
				    long httpcode =302;
				    return CURLE_OK;
			    }));
    EXPECT_EQ(checkDeviceInternetConnection(2000), false);
}
TEST_F(urlHelperTestFixture, checkDeviceInternetConnection_timeout_0)
{
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_setopt(_,_,_))
            .WillRepeatedly(Return(CURLE_OK));
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_perform(_)).Times(1).WillOnce(Return(CURLE_OK));
    long http_code = 302;
    EXPECT_CALL(*g_CurlWrapperMock, curl_easy_getinfo(_,_,_))
            .Times(1)
            .WillOnce(Return(0));
    EXPECT_EQ(checkDeviceInternetConnection(0), false);
}

/*25.writeFunction*/
TEST_F(urlHelperTestFixture, writeFunction_valid_Inputs)
{
    size_t a=6;
    size_t b=3;
    char test[10] = "test";
    EXPECT_EQ(writeFunction(NULL, a, b, &test), a*b );
}

/*26.Test that passing NULL returns NULL */
TEST(UrlEncodeStringTest, NullInputReturnsNull) {
    char* result = urlEncodeString(nullptr);
    EXPECT_EQ(result, nullptr);
}

/*27.Test that encoding an empty string returns an empty string */
TEST(UrlEncodeStringTest, EmptyString) {
    char* result = urlEncodeString("");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    free(result);
}

/*28.Test that a simple alphanumeric string is unchanged */
TEST(UrlEncodeStringTest, SimpleString) {
    char* result = urlEncodeString("abc123");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "abc123");
    free(result);
}

/*29.Test that spaces are encoded as %20 */
TEST(UrlEncodeStringTest, SpaceEncoding) {
    char* result = urlEncodeString("hello world");
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "hello%20world");
    free(result);
}

/*30.Test encoding of a string with all ASCII special characters */
TEST(UrlEncodeStringTest, AllAscii) {
    // Using a string with various ASCII characters that need encoding
    const char* input = "!*'();:@&=+$,/?#[]";
    char* result = urlEncodeString(input);
    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "%21%2A%27%28%29%3B%3A%40%26%3D%2B%24%2C%2F%3F%23%5B%5D");
    free(result);
}

GTEST_API_ int main(int argc, char *argv[]){
    char testresults_fullfilepath[GTEST_REPORT_FILEPATH_SIZE];
    char buffer[GTEST_REPORT_FILEPATH_SIZE];

    memset( testresults_fullfilepath, 0, GTEST_REPORT_FILEPATH_SIZE );
    memset( buffer, 0, GTEST_REPORT_FILEPATH_SIZE );

    snprintf( testresults_fullfilepath, GTEST_REPORT_FILEPATH_SIZE, "json:%s%s" , GTEST_DEFAULT_RESULT_FILEPATH , GTEST_DEFAULT_RESULT_FILENAME);
    ::testing::GTEST_FLAG(output) = testresults_fullfilepath;
	    ::testing::InitGoogleTest(&argc, argv);
	        //testing::Mock::AllowLeak(mock);
		return RUN_ALL_TESTS();
	}
