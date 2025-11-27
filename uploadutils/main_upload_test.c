/*
 * Copyright 2025 RDK Management
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

/**
 * @file main_upload_test.c
 * @brief Test program for upload utilities
 */

#include <stdio.h>
#include <stdlib.h>
#include "uploadUtil.h"
#include "mtls_upload.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <upload_url> <file_path>\n", argv[0]);
        return 1;
    }

    const char *upload_url = argv[1];
    const char *file_path = argv[2];

    printf("Testing upload: %s -> %s\n", file_path, upload_url);

    int result = uploadFileWithTwoStageFlow(upload_url, file_path);

    if (result == 0) {
        printf("Upload successful\n");
    } else {
        printf("Upload failed with code: %d\n", result);
    }

    return result;
}
