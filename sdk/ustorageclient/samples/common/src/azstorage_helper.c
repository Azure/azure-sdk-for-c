#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "azure_c_shared_utility/xlogging.h"

#include "azstorage_blob_api.h"
#include "../inc/azstorage_helper.h"

AZSTORAGE_RESULT azstorage_blob_get_size(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, size_t* blob_size)
{
    AZSTORAGE_RESULT result;

    // allocate memory for headers
    HTTP_HEADERS_HANDLE response_header;

    if ((response_header = HTTPHeaders_Alloc()) == NULL)
    {
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP header.", result);
    }
    // grab blob size from metadata
    else if((result = azstorage_blob_get_metadata(blob_storage_client, blob_path, response_header))
            != AZSTORAGE_SUCCESS)
    {
        HTTPHeaders_Free(response_header);
        LogError("ERROR 0x%x: retrieving blob metadata.", result);
    }
    else
    {
        const char* size_str = HTTPHeaders_FindHeaderValue(response_header, "Content-Length");
        // copy size string into blob_size size_t variable
        if ((size_str == NULL) || (sscanf(size_str, "%zu", blob_size) != 1))
        {
            result = AZSTORAGE_REQUEST_HEADER_ERROR;
        }
        // free internal memory
        HTTPHeaders_Free(response_header);
        LogInfo("azstorage_blob_get_size [%zu] success.", *blob_size);
    }

    return result;
}

AZSTORAGE_RESULT azstorage_blob_get_whole(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, size_t buffer_size, unsigned char* buffer_stream,
                                    unsigned char* file_dest, size_t* file_size)
{
    AZSTORAGE_RESULT result;
    size_t file_dest_size = *file_size;

    // get size of blob
    if ((result = azstorage_blob_get_size(blob_storage_client, blob_path, file_size)) != AZSTORAGE_SUCCESS)
    {
        LogError("ERROR 0x%x: failure getting blob size.", result);
    }
    else
    {
        // compare blob size with file_dest_size (if larger than, abort with file size error)
        if(*file_size > file_dest_size)
        {
            result = AZSTORAGE_MEMORY_ERROR;
            LogError("ERROR 0x%x: blob size larger than file destination size.", result);
        }
        else
        {
            // create temp buffer to fill with blob_get
            size_t remaining_size = *file_size;

            // loop over blob_get, adjusting offset by buffer_size until last chunk
            while((remaining_size > 0) && (result == AZSTORAGE_SUCCESS))
            {
                // grab blob bytes
                // (should only take remaining_size for the last chunk)
                size_t chunk_size = buffer_size < remaining_size ? buffer_size : remaining_size;
                if((result = azstorage_blob_get_bytes(blob_storage_client, blob_path, buffer_stream,
                    chunk_size, *file_size - remaining_size)) != AZSTORAGE_SUCCESS)
                {
                    LogError("ERROR 0x%x: failure getting blob chunk.", result);
                }
                else
                {
                    // append data to file_dest
                    memcpy((file_dest + *file_size - remaining_size), buffer_stream, chunk_size);
                    remaining_size -= chunk_size;
                }
            }
        }
    }

    return result;
}