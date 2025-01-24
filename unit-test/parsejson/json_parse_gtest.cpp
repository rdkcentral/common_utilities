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
#include "json_parse.h"
}

#define GTEST_DEFAULT_RESULT_FILEPATH "/tmp/Gtest_Report/"
#define GTEST_DEFAULT_RESULT_FILENAME "CommonUtilities_JsonParse_gtest_report.json"
#define GTEST_REPORT_FILEPATH_SIZE 256

using namespace testing;
using namespace std;
using ::testing::Return;
using ::testing::StrEq;

extern "C" {
    int (*getwriteItemVal(void))( FILE* fpout, char *pName, char *pVal, int setenvvars );
}

extern "C" {
    void (*getconvertInvalidChars(void))( char *pStr );
}

extern "C" {
    size_t (*get_getitemval_function(void))( cJSON *pcitem, char *pOut, size_t szpOutSize );
}

class Json_Parse_TestFixture : public ::testing::Test {
        protected:
                    // Member variables and functions here
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

//1.writeItemVal
TEST_F(Json_Parse_TestFixture, writeItemVal_valid_args)
{
    FILE *fp = fopen("/tmp/file.txt", "w");
    char name[20] = "name";
    char val[10] = "20";
    int setenvvars = 1;

    auto myFunctionPtr = getwriteItemVal();
    EXPECT_EQ(myFunctionPtr(fp, name, val, setenvvars), 0);
    fclose(fp);
    system("rm -rf /tmp/file.txt");
}
TEST_F(Json_Parse_TestFixture, writeItemVal_name_null)
{
    FILE *fp = fopen("/tmp/file.txt", "w");
    char val[10] = "20";
    int setenvvars = 1;

    auto myFunctionPtr = getwriteItemVal();
    EXPECT_EQ(myFunctionPtr(fp, NULL, val, setenvvars), 1);
    fclose(fp);
    system("rm -rf /tmp/file.txt");
}
TEST_F(Json_Parse_TestFixture, writeItemVal_val_NULL)
{
    FILE *fp = fopen("/tmp/file.txt", "w");
    char name[20] = "name";
    int setenvvars = 1;

    auto myFunctionPtr = getwriteItemVal();
    EXPECT_EQ(myFunctionPtr(fp, name, NULL, setenvvars), 1);
    fclose(fp);
    system("rm -rf /tmp/file.txt");
}
TEST_F(Json_Parse_TestFixture, writeItemVal_fpout_NULL)
{
    char name[20] = "name";
    char val[10] = "20";
    int setenvvars = 1;

    auto myFunctionPtr = getwriteItemVal();
    EXPECT_EQ(myFunctionPtr(NULL, name, val, setenvvars), 0);
}
TEST_F(Json_Parse_TestFixture, writeItemVal_setenvvars_0)
{
    FILE *fp = fopen("/tmp/file.txt", "w");
    char name[20] = "name";
    char val[10] = "20";

    auto myFunctionPtr = getwriteItemVal();
    EXPECT_EQ(myFunctionPtr(fp, name, val, 0), 0);
    fclose(fp);
    system("rm -rf /tmp/file.txt");
}

//2.convertInvalidChars
TEST_F(Json_Parse_TestFixture, convertInvalidChars_name_NULL)
{
    auto myFunctionPtr = getconvertInvalidChars();
    myFunctionPtr(NULL);
}
TEST_F(Json_Parse_TestFixture, convertInvalidChars_name_not_NULL)
{
    char name[20] = "name";
    auto myFunctionPtr = getconvertInvalidChars();
    myFunctionPtr(name);
    printf("String after converting invalid characters : %s\n", name);
}
TEST_F(Json_Parse_TestFixture, convertInvalidChars_name_with_invalid_chars)
{
    char name[20] = "na=m/e";
    auto myFunctionPtr = getconvertInvalidChars();
    myFunctionPtr(name);
    printf("String after converting invalid characters : %s\n", name);
}

//3.getitemval
TEST_F(Json_Parse_TestFixture, getitemval_item_available)
{
    char ValToGet[20] = "city";
    char val[20];
    cJSON *item = NULL;
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    item=cJSON_GetObjectItem( pJson, ValToGet );

    auto myFunctionPtr = get_getitemval_function();
    EXPECT_NE(myFunctionPtr(item, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, getitemval_item_unavailable)
{
    char ValToGet[20] = "citi";
    char val[20];
    cJSON *item = NULL;
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    item=cJSON_GetObjectItem( pJson, ValToGet );

    auto myFunctionPtr = get_getitemval_function();
    EXPECT_EQ(myFunctionPtr(item, val, 20), 0);
}

//4.SetJsonVars
TEST_F(Json_Parse_TestFixture, SetJsonVars_NULL_input_file)
{
    EXPECT_NE(SetJsonVars(NULL, NULL, 1), 0);
}
TEST_F(Json_Parse_TestFixture, SetJsonVars_NULL_output_file)
{
    char file[20] = "/tmp/json_file";
    EXPECT_NE(SetJsonVars(file, NULL, 1), 0);
}
TEST_F(Json_Parse_TestFixture, SetJsonVars_setenvvars_0)
{
    char file[20] = "/tmp/json_file";
    char outfile[20] = "/tmp/out_file";
    EXPECT_NE(SetJsonVars(file, outfile, 0), 0);
    system("cat /tmp/out_file");
    system("rm -rf /tmp/out_file");
}
TEST_F(Json_Parse_TestFixture, SetJsonVars_valid_json_file)
{
    char file[20] = "/tmp/json_file";
    char outfile[20] = "/tmp/out_file";
    char *command = "echo \"{\\\"name\\\":\\\"John\\\", \\\"age\\\":30, \\\"city\\\":\\\"New York\\\"}\" > /tmp/json_file";
    system(command);
    system("touch /tmp/out_file");
    EXPECT_EQ(SetJsonVars(file, outfile, 1), 0);
    system("cat /tmp/out_file");
    system("rm -rf /tmp/out_file");
    system("rm -rf /tmp/json_file");
}

//5.ParseJsonStr
TEST_F(Json_Parse_TestFixture, ParseJsonStr_NULL_input)
{
    EXPECT_EQ(ParseJsonStr(NULL), nullptr);
}
TEST_F(Json_Parse_TestFixture, ParseJsonStr_valid_json)
{
    char jsonString[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_NE(ParseJsonStr(jsonString), nullptr);
}

//6.FreeJson
TEST_F(Json_Parse_TestFixture, FreeJson_NULL_input)
{
    EXPECT_EQ(FreeJson(NULL), -1);
}
TEST_F(Json_Parse_TestFixture, FreeJson_valid_input)
{
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(FreeJson(pJson), 0);
}

//7.GetJsonItem
TEST_F(Json_Parse_TestFixture, GetJsonItem_NULL_Json)
{
    char item[20] = "name";
    EXPECT_EQ(GetJsonItem(NULL, item), nullptr);
}
TEST_F(Json_Parse_TestFixture, GetJsonItem_NULL_item)
{
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(GetJsonItem(pJson, NULL), nullptr);
}
TEST_F(Json_Parse_TestFixture, GetJsonItem_valid_input)
{
    char item[20] = "name";
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_NE(GetJsonItem(pJson, item), nullptr);
}

//8.GetJsonValFromString
TEST_F(Json_Parse_TestFixture, GetJsonValFromString_valid_input)
{
    char item[20] = "name";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_NE(GetJsonValFromString(pJsonStr, item, val, 20), 0);
    printf("value: %s\n", val);
}
TEST_F(Json_Parse_TestFixture, GetJsonValFromString_Json_NULL)
{
    char item[20] = "name";
    char val[20];
    EXPECT_EQ(GetJsonValFromString(NULL, item, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValFromString_val_NULL)
{
    char item[20] = "name";
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_EQ(GetJsonValFromString(pJsonStr, item, NULL, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValFromString_NULL_item)
{
    char item[20] = "name";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_EQ(GetJsonValFromString(pJsonStr, NULL, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValFromString_unavailable_item)
{
    char item[20] = "names";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_EQ(GetJsonValFromString(pJsonStr, item, val, 20), 0);
}

//9.GetJsonVal
TEST_F(Json_Parse_TestFixture, GetJsonVal_valid_input)
{
    char item[20] = "city";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_NE(GetJsonVal(pJson, item, val, 20), 0);
    printf("value: %s\n", val);
}
TEST_F(Json_Parse_TestFixture, GetJsonVal_Json_NULL)
{
    char item[20] = "name";
    char val[20];
    EXPECT_EQ(GetJsonVal(NULL, item, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonVal_val_NULL)
{
    char item[20] = "name";
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(GetJsonVal(pJson, item, NULL, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonVal_NULL_item)
{
    char item[20] = "name";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(GetJsonVal(pJson, NULL, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonVal_unavailable_item)
{
    char item[20] = "names";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(GetJsonVal(pJson, item, val, 20), 0);
}

//10.GetJsonValContainingFromString
TEST_F(Json_Parse_TestFixture, GetJsonValContainingFromString_valid_input)
{
    char item[20] = "ag";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_NE(GetJsonValContainingFromString(pJsonStr, item, val, 20), 0);
    printf("value: %s\n", val);
}
TEST_F(Json_Parse_TestFixture, GetJsonValContainingFromString_Json_NULL)
{
    char item[20] = "name";
    char val[20];
    EXPECT_EQ(GetJsonValContainingFromString(NULL, item, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValContainingFromString_val_NULL)
{
    char item[20] = "name";
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_EQ(GetJsonValContainingFromString(pJsonStr, item, NULL, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValContainingFromString_NULL_item)
{
    char item[20] = "name";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    EXPECT_EQ(GetJsonValContainingFromString(pJsonStr, NULL, val, 20), 0);
}

//11.GetJsonValContaining
TEST_F(Json_Parse_TestFixture, GetJsonValContaining_valid_input)
{
    char item[20] = "name";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_NE(GetJsonValContaining(pJson, item, val, 20), 0);
    printf("value: %s\n", val);
}
TEST_F(Json_Parse_TestFixture, GetJsonValContaining_Json_NULL)
{
    char item[20] = "name";
    char val[20];
    EXPECT_EQ(GetJsonValContaining(NULL, item, val, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValContaining_val_NULL)
{
    char item[20] = "name";
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(GetJsonValContaining(pJson, item, NULL, 20), 0);
}
TEST_F(Json_Parse_TestFixture, GetJsonValContaining_NULL_item)
{
    char item[20] = "name";
    char val[20];
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );
    EXPECT_EQ(GetJsonValContaining(pJson, NULL, val, 20), 0);
}

//12.GetJson
TEST_F(Json_Parse_TestFixture, GetJson_NULL_input)
{
    EXPECT_EQ(GetJson(NULL), nullptr);
}
TEST_F(Json_Parse_TestFixture, GetJson_valid_json)
{
    char file[20] = "/tmp/json_file";
    char *output = NULL;
    char *command = "echo \"{\\\"name\\\":\\\"John\\\", \\\"age\\\":30, \\\"city\\\":\\\"New York\\\"}\" > /tmp/json_file";
    system(command);
    output = GetJson(file);
    EXPECT_NE(output, nullptr);
    printf("output = %s \n", output);
    free(output);
    system("rm -rf /tmp/json_file");
}

//13.IsJsonArray
TEST_F(Json_Parse_TestFixture, IsJsonArray_not_array)
{
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );

    EXPECT_EQ(IsJsonArray(pJson), false);
}

TEST_F(Json_Parse_TestFixture, IsJsonArray_array)
{
    char pJsonStr[100] = "[{\"name\": \"John\", \"age\": 30}, {\"name\": \"Jane\", \"age\": 25}, {\"name\": \"Doe\", \"age\": 22}]";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );

    EXPECT_NE(IsJsonArray(pJson), false);
}

//14.GetJsonArraySize
TEST_F(Json_Parse_TestFixture, GetJsonArraySize_not_array)
{
    char pJsonStr[100] = "{\"name\":\"John\", \"age\":30, \"city\":\"New York\"}";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );

    EXPECT_EQ(GetJsonArraySize(pJson), 0);
}

TEST_F(Json_Parse_TestFixture, GetJsonArraySize_array)
{
    char pJsonStr[100] = "[{\"name\": \"John\", \"age\": 30}, {\"name\": \"Jane\", \"age\": 25}, {\"name\": \"Doe\", \"age\": 22}]";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );

    EXPECT_NE(GetJsonArraySize(pJson), 0);
}

//15.GetJsonArrayItem
TEST_F(Json_Parse_TestFixture, GetJsonArrayItem_NULL_json)
{
    EXPECT_EQ(GetJsonArrayItem(NULL, 2), nullptr);
}
TEST_F(Json_Parse_TestFixture, GetJsonArrayItem_valid_index)
{
    char pJsonStr[100] = "[{\"name\": \"John\", \"age\": 30}, {\"name\": \"Jane\", \"age\": 25}, {\"name\": \"Doe\", \"age\": 22}]";
    JSON *pJson = NULL;
    JSON *pJsonOut = NULL;
    pJson = ParseJsonStr( pJsonStr );

    pJson = GetJsonArrayItem(pJson, 2);
    EXPECT_NE(pJson, nullptr);
}
TEST_F(Json_Parse_TestFixture, GetJsonArrayItem_invalid_index)
{
    char pJsonStr[100] = "[{\"name\": \"John\", \"age\": 30}, {\"name\": \"Jane\", \"age\": 25}, {\"name\": \"Doe\", \"age\": 22}]";
    JSON *pJson = NULL;
    pJson = ParseJsonStr( pJsonStr );

    EXPECT_EQ(GetJsonArrayItem(pJson, 4), nullptr);
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
