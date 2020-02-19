// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/platform.h"

#include "azstorage_blob_api.h"
#include "azstorage_storage_config.h"
#include "blob_sample_config.h"
#include "azstorage_helper.h"

static const unsigned char data[] = "0123456789\n";
static unsigned char buffer_download[AZSTORAGE_BUFFER_SIZE];
static unsigned char file[AZSTORAGE_AVAILABLE_STORAGE];
static size_t file_size = AZSTORAGE_AVAILABLE_STORAGE;

int main(void)
{
    // start sample
    (void)printf("********** Sample '%s' START **********\r\n\n", AZSTORAGE_CONFIG_SAMPLE_NAME);

    // WINDOWS: initialize winsock
    // LINUX: initialize ssl
    platform_init();
    AZSTORAGE_RESULT result;
    AZSTORAGE_BLOB blob_storage_client;
    AZSTORAGE_BLOB_TYPE blob_type = AZSTORAGE_BLOB_TYPE_APPEND_BLOB;

    (void)printf("Begin uploading append blob\r\n");

    // create blob storage client
    if((result = azstorage_blob_storage_client_create(&blob_storage_client, AZSTORAGE_CONFIG_ACCOUNT_HOST_NAME))
        != AZSTORAGE_SUCCESS)
    {
        (void)printf("ERROR 0x%x: cannot create storage client\r\n", result);
    }
    // initialize append blob
    else if((result = azstorage_blob_put(&blob_storage_client, blob_type, AZSTORAGE_CONFIG_PUT_BLOB_PATH_WITH_SAS,
            NULL, (size_t)0)) != AZSTORAGE_SUCCESS)
    {
        azstorage_blob_storage_client_destroy(&blob_storage_client);
        (void)printf("ERROR 0x%x: cannot initialize append blob\r\n", result);
    }
    // append data to append block
    else if((result = azstorage_blob_append_block(&blob_storage_client, AZSTORAGE_CONFIG_APPEND_BLOB_PATH_WITH_SAS,
            data, sizeof(data))) != AZSTORAGE_SUCCESS)
    {
        azstorage_blob_storage_client_destroy(&blob_storage_client);
        (void)printf("ERROR 0x%x: cannot upload or append data to append blob\r\n", result);
    }
    // retrieve blob data from page blob
    else if((result = azstorage_blob_get_whole(&blob_storage_client, AZSTORAGE_CONFIG_PUT_BLOB_PATH_WITH_SAS,
        AZSTORAGE_BUFFER_SIZE, buffer_download, file, &file_size))
        != AZSTORAGE_SUCCESS)
    {
        azstorage_blob_storage_client_destroy(&blob_storage_client);
        (void)printf("ERROR 0x%x: retrieving data from blob.\r\n", result);
    }
    // destroy blob storage client
    else
    {
        azstorage_blob_storage_client_destroy(&blob_storage_client);
        (void)printf("Successfully uploaded and appended data to append blob!\r\n");
        (void)printf("Uploaded Data:\n%s\n", data);
        (void)printf("Downloaded Data:\n%.*s\n", (int)file_size, file);
        // compare
        if(strncmp((const char *)data, (const char *)file, sizeof(data)) == 0)
        {
            (void)printf("Downloaded blob matches uploaded append blob!\r\n");
        }
        else
        {
            (void)printf("Downloaded blob does not match uploaded append blob...\r\n");
        }
    }

    // end sample
    (void)printf("\n********** Sample '%s' FINISH **********\r\n", AZSTORAGE_CONFIG_SAMPLE_NAME);

    return 0;
}
