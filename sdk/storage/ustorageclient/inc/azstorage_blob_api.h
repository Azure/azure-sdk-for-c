// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/**
 *  @file azstorage_blobs_api.h
 *  @brief A C SDK for Azure Blob Storage optimized for resource-constrained devices.
 *
 * P1:  Contains core functionality of all planned blob upload/download functions.
*/

#ifndef AZSTORAGE_BLOB_API_H
#define AZSTORAGE_BLOB_API_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
extern "C"
{
#else
#include <stddef.h>
#include <stdint.h>
#endif /* __cplusplus */

#include "umock_c/umock_c_prod.h"
#include "azure_c_shared_utility/httpapi.h"

#include "_internal/_azstorage_internal.h"

/**
 * @brief enumerated type for block blobs (to select string)
 * @note  only block blob supports uploading data with the put function
 *        append blobs require function @link{azstorage_blob_append_block}
 * @note  page blobs currently unsupported
 */
MU_DEFINE_ENUM(
    AZSTORAGE_BLOB_TYPE,
    AZSTORAGE_BLOB_TYPE_BLOCK_BLOB,
    AZSTORAGE_BLOB_TYPE_APPEND_BLOB
)

/**
* @brief Error bit for error values.
*/
#define AZSTORAGE_STORAGE_ERROR_FLAG                   0x80

/**
 * @brief Enumerated result values returned by these APIs.
 */
MU_DEFINE_ENUM(
    AZSTORAGE_RESULT,

    // SUCCESS RESULTS
    AZSTORAGE_SUCCESS                   = 0,                                     /**<Successful return */

    // ERROR RESULTS
    AZSTORAGE_CREATE_ERROR              = (AZSTORAGE_STORAGE_ERROR_FLAG | 1),    /**<Create client error */
    AZSTORAGE_ILLEGAL_ARGUMENT_ERROR    = (AZSTORAGE_STORAGE_ERROR_FLAG | 2),    /**<Invalid argument error */
    AZSTORAGE_MEMORY_ERROR              = (AZSTORAGE_STORAGE_ERROR_FLAG | 3),    /**<Out of memory error */
    AZSTORAGE_NOT_IMPLEMENTED_ERROR     = (AZSTORAGE_STORAGE_ERROR_FLAG | 4),    /**<Client object not implemented error */
    AZSTORAGE_REQUEST_ERROR             = (AZSTORAGE_STORAGE_ERROR_FLAG | 5),    /**<HTTPAPI_ExecuteRequest error */
    AZSTORAGE_REQUEST_HEADER_ERROR      = (AZSTORAGE_STORAGE_ERROR_FLAG | 6),    /**<Adding HTTP header error */
    AZSTORAGE_SERVICE_ERROR             = (AZSTORAGE_STORAGE_ERROR_FLAG | 7)     /**<HTTP status code error */
)

/**
 * @brief dummy wrapper to allow user to allocate correct amount of memory for storage client
 */
typedef struct AZSTORAGE_BLOB_TAG
{
    AZSTORAGE_BLOB_STORAGE_CLIENT az_private;
} AZSTORAGE_BLOB;

/**
 * @brief handle for dummy pointer
 */
typedef AZSTORAGE_BLOB* AZSTORAGE_BLOB_HANDLE;

/*
 * ******************************** Storage Client Functions ********************************
 */

/**
 * @brief   Create blob storage client connection handle.
 *
 * @usage   All parameters shall remain valid until @link{azstorage_blob_storage_client_destroy} is called.
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_HANDLE dummy object. Allows user to allocate memory to and reference
 *                                  an instance to #AZ_BLOB_STORAGE_CLIENT*, the control block used to access the storage
 *                                  account, without giving user access to member variables.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   service_uri             The <tt>const char* const</tt> blob service uri.
 *                                  ex: <account>.blob.core.windows.net. Cannot be <tt>NULL</tt>.
 * @note                            Must be non-volatile or remain valid until @link{az_storage_blob_storage_client_destroy}
 *                                  is called.
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors
 *          @retval     AZSTORAGE_SUCCESS                    If the client is created with no errors.
 *          @retval     AZSTORAGE_ILLEGAL_ARGUMENT_ERROR     If one or more of the arguments is invalid (only
 *                                                           thrown if #AZSTORAGE_BLOB_VALIDATE_ARGUMENT
 *                                                           is defined).
 *          @retval     AZSTORAGE_CREATE_ERROR               If the client cannot create a logical
 *                                                           connection to host.
 */
MOCKABLE_FUNCTION(, AZSTORAGE_RESULT, azstorage_blob_storage_client_create,
    AZSTORAGE_BLOB_HANDLE, blob_storage_client, const char* const, service_uri);

/**
 * @brief   Cleanup blob service client resources.
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_HANDLE dummy object. Allows user to allocate memory to and reference
 *                                  #AZ_BLOB_STORAGE_CLIENT*, the control block used to access the storage account.
 * @note                            Cannot be <tt>NULL</tt>.
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors.
 *          @retval     AZSTORAGE_SUCCESS                    If the client is destroyed with no errors.
 *          @retval     AZSTORAGE_ILLEGAL_ARGUMENT_ERROR     If one or more of the arguments is invalid (only
 *          @retval     AZSTORAGE_NOT_IMPLEMENTED_ERROR      If the HTTP portion of the client is not implemented.
 */
MOCKABLE_FUNCTION(, void, azstorage_blob_storage_client_destroy, AZSTORAGE_BLOB_HANDLE, blob_storage_client);

/*
 * ******************************** Blob Functions ********************************
 */

/**
 * @brief   Creates a new blob or replaces an existing block blob within a container.
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_HANDLE dummy object. Allows user to allocate memory to and reference
 *                                  #AZ_BLOB_STORAGE_CLIENT*, the control block used to access the storage account.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   blob_type               The #AZSTORAGE_BLOB_TYPE (type of blob) you would like to create.
 * @note                            Cannot be <tt>NULL</tt>.
 *                                  Use #AZSTORAGE_BLOB_TYPE AZSTORAGE_BLOB_TYPE_BLOCK_BLOB_STRING if you are using this method
 *                                  to upload data. Other methods (below) must be used to upload to append and block blobs.
 * @param   blob_path               The <tt>const char* const</tt> path (container name and blob name) of the new blob
 *                                  with the storage account sas token and component values.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   buffer                  The <tt>const unsigned char*</tt> buffer to upload to blob.
 * @note                            Use <tt>NULL</tt> if you are creating a new blob.
 * @param   buffer_len              The <tt>size_t</tt> length of the buffer.
 * @note                            Will be ignored if buffer is <tt>NULL</tt>.
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors.
 *          @retval     AZSTORAGE_SUCCESS                    If the blob is successfully created.
 *          @retval     AZSTORAGE_ILLEGAL_ARGUMENT_ERROR     If one or more of the arguments is invalid (only
 *                                                           thrown if #AZSTORAGE_BLOB_VALIDATE_ARGUMENT
 *                                                           is defined).
 *          @retval     AZSTORAGE_MEMORY_ERROR               If memory cannot be allocated for http headers.
 *          @retval     AZSTORAGE_REQUEST_HEADER_ERROR       If there is a problem adding the "Host" header.
 *          @retval     AZSTORAGE_REQUEST_ERROR              If there is a problem sending the HTTP request.
 *          @retval     AZSTORAGE_SERVICE_ERROR              If there is an error thrown by the server.
 */
MOCKABLE_FUNCTION(, AZSTORAGE_RESULT, azstorage_blob_put,
    AZSTORAGE_BLOB_HANDLE, blob_storage_client, AZSTORAGE_BLOB_TYPE, blob_type, const char* const, blob_path,
    const unsigned char*, buffer, size_t, buffer_len);

/**
 * @brief   Appends data to the end of an append blob.
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_HANDLE dummy object. Allows user to allocate memory to and reference
 *                                  #AZ_BLOB_STORAGE_CLIENT*, the control block used to access the storage account.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   blob_path               The <tt>const char* const</tt> path (container name and blob name) of the new blob
 *                                  with the storage account sas token and component values.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   buffer                  The <tt>const unsigned char*</tt> buffer to upload to blob.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   buffer_len              The <tt>size_t</tt> length of the buffer.
 * @note                            Cannot be <tt>(size_t)0</tt>.
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors.
 *          @retval     AZSTORAGE_SUCCESS                    If the blob is successfully created.
 *          @retval     AZSTORAGE_ILLEGAL_ARGUMENT_ERROR     If one or more of the arguments is invalid (only
 *                                                           thrown if #AZSTORAGE_BLOB_VALIDATE_ARGUMENT
 *                                                           is defined).
 *          @retval     AZSTORAGE_MEMORY_ERROR               If memory cannot be allocated for http headers.
 *          @retval     AZSTORAGE_REQUEST_HEADER_ERROR       If there is a problem adding the "Host" header.
 *          @retval     AZSTORAGE_REQUEST_ERROR              If there is a problem sending the HTTP request.
 *          @retval     AZSTORAGE_SERVICE_ERROR              If there is an error thrown by the server.
 */
MOCKABLE_FUNCTION(, AZSTORAGE_RESULT, azstorage_blob_append_block,
    AZSTORAGE_BLOB_HANDLE, blob_storage_client, const char* const, blob_path, const unsigned char*, buffer,
    size_t, buffer_len);

/**
 * @brief   Retrieves blob metadata in the form of a #HTTP_HEADERS_HANDLE response_header.
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_HANDLE dummy object. Allows user to allocate memory to and reference
 *                                  #AZ_BLOB_STORAGE_CLIENT*, the control block used to access the storage account.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   blob_path               The <tt>const char* const</tt> path (container name and blob name) of the new blob
 *                                  with the storage account sas token and component values.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   response_header         The #HTTP_HEADERS_HANDLE containing the headers of the storage account response.
 *                                  Where all metadata values can be found.
 * @note                            Cannot be <tt>NULL</tt>.
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors.
 *          @retval     AZSTORAGE_SUCCESS                    If the blob is successfully created.
 *          @retval     AZSTORAGE_ILLEGAL_ARGUMENT_ERROR     If one or more of the arguments is invalid (only
 *                                                           thrown if #AZSTORAGE_BLOB_VALIDATE_ARGUMENT
 *                                                           is defined).
 *          @retval     AZSTORAGE_MEMORY_ERROR               If memory cannot be allocated for http headers.
 *          @retval     AZSTORAGE_REQUEST_HEADER_ERROR       If there is a problem adding the "Host" header.
 *          @retval     AZSTORAGE_REQUEST_ERROR              If there is a problem sending the HTTP request.
 *          @retval     AZSTORAGE_SERVICE_ERROR              If there is an error thrown by the server.
 */
MOCKABLE_FUNCTION(, AZSTORAGE_RESULT, azstorage_blob_get_metadata,
    AZSTORAGE_BLOB_HANDLE, blob_storage_client, const char* const, blob_path, HTTP_HEADERS_HANDLE,
    response_header);

/**
 * @brief   Downloads bytes of a blob in the specified range [start_offset, start_offset + buffer_len)
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_HANDLE dummy object. Allows user to allocate memory to and reference
 *                                  #AZ_BLOB_STORAGE_CLIENT*, the control block used to access the storage account.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   blob_path               The <tt>const char* const</tt> path (container name and blob name) of the new blob
 *                                  with the storage account sas token and component values.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   buffer                  The <tt>unsigned char*</tt> buffer of size <tt>buffer_len</tt>
 *                                  to store blob data. Use <tt>NULL</tt> to download the whole blob
 *                                  (make sure you know the size of the blob beforehand).
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   buffer_len              The <tt>size_t</tt> Buffer length (end byte offset is <tt>start_offset + buffer_len</tt>)
 *                                  (exclusive).
 * @note                            Cannot be <tt>(size_t)0</tt>.
 * @param   start_offset            The <tt>offset_t</tt> start byte offset in blob (inclusive).
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors.
 *          @retval     AZSTORAGE_SUCCESS                    If the blob is successfully created.
 *          @retval     AZSTORAGE_ILLEGAL_ARGUMENT_ERROR     If one or more of the arguments is invalid (only
 *                                                           thrown if #AZSTORAGE_BLOB_VALIDATE_ARGUMENT
 *                                                           is defined).
 *          @retval     AZSTORAGE_MEMORY_ERROR               If memory cannot be allocated for byte range
 *                                                           #STRING_handle.
 *          @retval     AZSTORAGE_MEMORY_ERROR               If memory cannot be allocated for http headers.
 *          @retval     AZSTORAGE_MEMORY_ERROR               If memory cannot be allocated for response buffer
 *                                                           #BUFFER_handle.
 *          @retval     AZSTORAGE_REQUEST_HEADER_ERROR       If there is a problem adding the "Host" header.
 *          @retval     AZSTORAGE_REQUEST_ERROR              If there is a problem sending the HTTP request.
 *          @retval     AZSTORAGE_SERVICE_ERROR              If there is an error thrown by the server.
 */
MOCKABLE_FUNCTION(, AZSTORAGE_RESULT, azstorage_blob_get_bytes,
    AZSTORAGE_BLOB_HANDLE, blob_storage_client, const char* const, blob_path, unsigned char*, buffer, size_t,
    buffer_len, offset_t, start_offset);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* AZSTORAGE_BLOB_API_H */
