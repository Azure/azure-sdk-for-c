// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "azure_c_shared_utility/platform.h"

#include "azstorage_blob_api.h"
#include "azstorage_storage_config.h"
#include "blob_sample_config.h"

static const unsigned char file_buffer[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcefghijklmnopqrstuvwxyz0123456789";


void main(void)
{
    // start sample
    (void)printf("********** Sample '%s' START **********\r\n\n", AZSTORAGE_CONFIG_SAMPLE_NAME);

    // WINDOWS: initialize winsock
    // LINUX: initialize ssl
    platform_init();

    AZSTORAGE_RESULT result;
	AZSTORAGE_BLOB blob_storage_client;
    AZSTORAGE_BLOB_TYPE blob_type = AZSTORAGE_BLOB_TYPE_BLOCK_BLOB;

    (void)printf("Begin uploading block blob.\r\n");

    // create blob storage client
    if((result = azstorage_blob_storage_client_create(&blob_storage_client, AZSTORAGE_CONFIG_ACCOUNT_HOST_NAME))
        != AZSTORAGE_SUCCESS)
    {
        (void)printf("ERROR 0x%x: creating storage client.\r\n", result);
    }
    // upload buffer to blob
    else if((result = azstorage_blob_put(&blob_storage_client, blob_type, AZSTORAGE_CONFIG_PUT_BLOB_PATH_WITH_SAS,
        file_buffer, sizeof(file_buffer))) != AZSTORAGE_SUCCESS)
    {
        azstorage_blob_storage_client_destroy(&blob_storage_client);
        (void)printf("ERROR 0x%x: uploading data to blob.\r\n", result);
    }
    // destroy blob storage client
    else
    {
        azstorage_blob_storage_client_destroy(&blob_storage_client);
        (void)printf("Successfully uploaded block blob!\r\n");
    }

    // end sample
    (void)printf("\n********** Sample '%s' COMPLETE **********\r\n", AZSTORAGE_CONFIG_SAMPLE_NAME);

}
