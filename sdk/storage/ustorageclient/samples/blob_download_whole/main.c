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

static unsigned char buffer_download[AZSTORAGE_BUFFER_SIZE];
static unsigned char file[AZSTORAGE_AVAILABLE_STORAGE];
static size_t file_size = AZSTORAGE_AVAILABLE_STORAGE;

void main(void)
{
    // start sample
    (void)printf("********** Sample '%s' START **********\r\n\n", AZSTORAGE_CONFIG_SAMPLE_NAME);

    // WINDOWS: initialize winsock
    // LINUX: initialize ssl
    platform_init();
    AZSTORAGE_RESULT result;

    // create blob storage client
    AZSTORAGE_BLOB blob_storage_client;
    (void)printf("Begin downloading blob\n");
    if((result = azstorage_blob_storage_client_create(&blob_storage_client, AZSTORAGE_CONFIG_ACCOUNT_HOST_NAME))
        != AZSTORAGE_SUCCESS)
    {
        (void)printf("ERROR 0x%x: creating storage client.\r\n", result);
    }
    // retrieve blob data from page blob
    else if((result = azstorage_blob_get_whole(&blob_storage_client, AZSTORAGE_CONFIG_DOWNLOAD_BLOB_PATH_WITH_SAS,
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
        (void)printf("\nBlob Data:\n%.*s\n", (int)file_size, file);
        (void)printf("Successfully downloaded blob!\r\n");
    }

    // end sample
    (void)printf("\n********** Sample '%s' COMPLETE **********\r\n", AZSTORAGE_CONFIG_SAMPLE_NAME);
}
