// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**
 *  @file azstorage_helper.h
 *  @brief Helper functions for samples.
*/

#ifndef AZSTORAGE_HELPER_H
#define AZSTORAGE_HELPER_H

#include "azure_c_shared_utility/xlogging.h"

#include "azstorage_blob_api.h"

/**
 * @brief get blob size using public API @link{azstorage_blob_get_metadata}
*/
AZSTORAGE_RESULT azstorage_blob_get_size(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, size_t* blob_size);

/**
 * @brief get whole blob using local helper function @link{azstorage_get_blob_size} and
 * public API @link{azstorage_blob_get_bytes}
*/
AZSTORAGE_RESULT azstorage_blob_get_whole(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, size_t buffer_size, unsigned char* buffer_stream,
                                    unsigned char* file_dest, size_t* file_size);

#endif /* AZSTORAGE_HELPER_H */
