// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * Test http client PAL
 * 
 */

#include <stdio.h>
#include <az_http_client.h>
#include <az_str.h>

#define TEST_URL_CONST "http://www.laboratoons.com/wp-content/uploads/2018/11/Alternativas-A.jpg"
#define TEST_FILE_CONST "downloadTest.jpg"

static az_const_span TEST_URL = AZ_CONST_STR(TEST_URL_CONST);
static az_const_span TEST_FILE = AZ_CONST_STR(TEST_FILE_CONST);

int main()
{
    int result;
    
    az_http_client_init();
    az_http_client_set_URL(TEST_URL);
    result = az_http_client_download_to_file(TEST_FILE);

    if (result == AZ_OK) {
        printf("\n Done.");
    } else {
        printf("\n Error:\n");
    }
    
    az_http_client_clean();

    return 0;
}
