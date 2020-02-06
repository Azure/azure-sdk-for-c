// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azstorage_blob_api.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/strings.h"
// TODO: once azure-ulib-c is public this can be enabled
//#include "ucontract.h"

#include "azstorage_storage_config.h"

/**
 * @brief header names to be used when adding header value pairs to http request
 */
#define AZSTORAGE_HOST_HEADER_NAME                  "Host"
#define AZSTORAGE_BLOB_TYPE_HEADER_NAME             "x-ms-blob-type"
#define AZSTORAGE_CONTENT_LENGTH_HEADER_NAME        "Content-Length"
#define AZSTORAGE_BYTE_RANGE_HEADER_NAME            "x-ms-range"

/**
 * @brief   internal data structures
 */
char size_str[AZSTORAGE_SIZE_STR_SIZE] = { 0 };

/**
 * @brief   blob type enum to string
 */
static const char* const blob_type_enum_to_string(AZSTORAGE_BLOB_TYPE blob_type)
{
    const char* ret;
    switch(blob_type)
    {
        case AZSTORAGE_BLOB_TYPE_BLOCK_BLOB:
            ret = AZSTORAGE_BLOB_TYPE_BLOCK_BLOB_STRING;
        break;
        case AZSTORAGE_BLOB_TYPE_APPEND_BLOB:
            ret = AZSTORAGE_BLOB_TYPE_APPEND_BLOB_STRING;
        break;
        default:
            ret = NULL;
        break;
    }
    return ret;
}

/**TODO: leave detailed comment for storage team or delete?
 * @brief   Add host header and execute http request.
 *
 * @usage   Used to compose blob.c requests (INTERNAL ONLY).
 *
 * @param   blob_storage_client     The #AZSTORAGE_BLOB_STORAGE_CLIENT* for the connection that is used to access blob.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   request_type            The #HTTPAPI_REQUEST_TYPE (HTTP verb) for the storage account. (GET, POST, PUT, etc.)
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   blob_path               The <tt>const char*</tt> path of the blob with component values (if applicable) and sas_token.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   request_header          The #HTTP_HEADERS_HANDLE containing the headers created by the calling function.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   content                 The <tt>const unsigned char*</tt> payload.
 * @param   content_length          The <tt>size_t</tt> length of the payload.
 * @note                            Only use <tt>(size_t)0</tt> if content_lenght is NULL.
 * @param   response_header         The #HTTP_HEADERS_HANDLE containing the headers of the storage account response.
 * @note                            Cannot be <tt>NULL</tt>.
 * @param   response_content        The #BUFFER_HANDLE response payload.
 * @note                            Cannot be <tt>NULL</tt>.
 *
 * @return  An @link{AZSTORAGE_RESULT} for success or various types of errors
 *          @retval     AZSTORAGE_SUCCESS                    If the request is sent and response is received
 *                                                                          with no errors.
 *          @retval     AZSTORAGE_REQUEST_HEADER_ERROR          If there is a problem adding the "Host" header.
 *          @retval     AZSTORAGE_REQUEST_ERROR              If there is a problem sending the HTTP request.
 *          @retval     AZSTORAGE_SERVICE_ERROR              If there is an error thrown by the server.
 */
static AZSTORAGE_RESULT send_request(AZSTORAGE_BLOB_STORAGE_CLIENT* blob_storage_client,
        HTTPAPI_REQUEST_TYPE request_type, const char* blob_path, HTTP_HEADERS_HANDLE request_header,
        const unsigned char* content, size_t content_length, HTTP_HEADERS_HANDLE response_headers,
        BUFFER_HANDLE response_content)
{
    AZSTORAGE_RESULT result;
    HTTP_HEADERS_RESULT headers_result;
    HTTPAPI_RESULT http_result;
    unsigned int http_status_code = 0;

    /*[azstorage_blob_put_http_add_header_host_FAIL]*/
    /*[azstorage_blob_append_block_http_add_header_host_FAIL]*/
    /*[azstorage_blob_get_bytes_http_add_header_host_FAIL]*/
    /*[azstorage_blob_get_metadata_http_add_header_host_FAIL]*/
    if ((headers_result = HTTPHeaders_AddHeaderNameValuePair(request_header, AZSTORAGE_HOST_HEADER_NAME,
        blob_storage_client->service_uri)) != HTTP_HEADERS_OK)
    {
        result = AZSTORAGE_REQUEST_HEADER_ERROR;
        LogError("ERROR 0x%x: failed to add service_uri 'Host' header.", headers_result);
    }
    /*[azstorage_blob_put_http_request_FAIL]*/
    /*[azstorage_blob_append_block_http_request_FAIL]*/
    /*[azstorage_blob_get_bytes_http_request_FAIL]*/
    /*[aazstorage_blob_get_metadata_http_request_FAIL]*/
    else if ((http_result = HTTPAPI_ExecuteRequest(blob_storage_client->http_handle, request_type,
            blob_path, request_header, content, content_length,
            &http_status_code, response_headers, response_content)) != HTTPAPI_OK)
    {
        result = AZSTORAGE_REQUEST_ERROR;
        LogError("ERROR 0x%x: failure on request\n", http_result);
    }
    /*[azstorage_blob_put_http_status_code_FAIL]*/
    /*[azstorage_blob_append_block_http_status_code_FAIL]*/
    /*[azstorage_blob_get_bytes_http_status_code_FAIL]*/
    /*[azstorage_blob_get_metadata_http_status_code_FAIL]*/
    else if ((http_status_code < 200) || (http_status_code >= 300))
    {
        result = AZSTORAGE_SERVICE_ERROR;
        LogError("ERROR 0x%x: failed with http status code %u.\n", result, http_status_code);
    }
    /*[azstorage_blob_put_SUCCESS]*/
    /*[azstorage_blob_append_block_SUCCESS]*/
    /*[azstorage_blob_get_bytes_SUCCESS]*/
    /*[azstorage_blob_get_metadata_SUCCESS]*/
    else
    {
        result = AZSTORAGE_SUCCESS;
    }

    return result;
}


AZSTORAGE_RESULT azstorage_blob_storage_client_create(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                                    const char* const service_uri)
{
    #ifdef AZSTORAGE_BLOB_VALIDATE_ARGUMENTS
    UCONTRACT
    (
        /*[azstorage_blob_service_client_create_blob_storage_client_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_storage_client, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_service_client_create_service_uri_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(service_uri, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR)
    );
    #else
        if (blob_storage_client == NULL || service_uri == NULL)
        {
            return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR;
        }
    #endif

    AZSTORAGE_RESULT result;
    AZSTORAGE_BLOB_STORAGE_CLIENT* cb = (AZSTORAGE_BLOB_STORAGE_CLIENT*)blob_storage_client;
    cb->service_uri = service_uri;

    /*[azstorage_blob_service_client_create_http_connection_NULL_FAIL]*/
    if ((cb->http_handle = HTTPAPI_CreateConnection(service_uri)) == NULL)
    {
        result = AZSTORAGE_CREATE_ERROR;
        LogError("ERROR 0x%x: Create Connection returned NULL.", result);
    }
    /*[azstorage_blob_service_client_create_SUCCESS]*/
    else
    {
        result = AZSTORAGE_SUCCESS;
        LogInfo("azstorage_blob_storage_client_create success.");
    }

    return result;
}

void azstorage_blob_storage_client_destroy(AZSTORAGE_BLOB_HANDLE blob_storage_client)
{
    #ifdef AZSTORAGE_BLOB_VALIDATE_ARGUMENTS
    UCONTRACT
    (
        /*[azstorage_blob_storage_client_destroy_blob_storage_client_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_storage_client,);
    );
    #else
        if (blob_storage_client == NULL)
        {
            return;
        }
    #endif

    AZSTORAGE_BLOB_STORAGE_CLIENT* cb = (AZSTORAGE_BLOB_STORAGE_CLIENT*)blob_storage_client;

    /*[azstorage_blob_storage_client_destroy_client_not_implemented_FAIL]*/
    if(cb->http_handle == NULL)
    {
        LogError("WARNING: Client not implemented. Nothing to destroy.");
    }
    /*[azstorage_blob_storage_client_destroy_SUCCESS]*/
    else
    {
        HTTPAPI_CloseConnection(cb->http_handle);
        cb->http_handle = NULL;
        LogInfo("azstorage_blob_storage_client_destroy success.");
    }
}

AZSTORAGE_RESULT azstorage_blob_put(AZSTORAGE_BLOB_HANDLE blob_storage_client, AZSTORAGE_BLOB_TYPE blob_type,
                                    const char* blob_path, const unsigned char* buffer, size_t buffer_len)
{
    #ifdef AZSTORAGE_BLOB_VALIDATE_ARGUMENTS
    UCONTRACT
    (
        /*[azstorage_blob_put_blob_storage_client_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_storage_client, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_put_blob_type_NULL_FAIL]*/
        UCONTRACT_REQUIRE(blob_type <= AZSTORAGE_BLOB_TYPE_APPEND_BLOB,
        AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, "Please use a valid blob type.\n"),
        /*[azstorage_blob_put_blob_path_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_path, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_put_buffer_NULL_buffer_len_not_ZERO_FAIL]*/
        /*[azstorage_blob_put_buffer_not_NULL_buffer_len_ZERO_FAIL]*/
        UCONTRACT_REQUIRE((buffer == NULL && buffer_len == 0) || (buffer_len > 0 && buffer != NULL ),
            AZSTORAGE_ILLEGAL_ARGUMENT_ERROR,
            "If buffer is NULL, buffer_len shall be 0.\nIf buffer is NOT NULL, buffer_len shall be greater than 0.\n")
    );
    #else
        if (blob_storage_client == NULL || blob_path == NULL || blob_type > AZSTORAGE_BLOB_TYPE_APPEND_BLOB ||
            ((buffer != NULL && buffer_len == 0) || (buffer_len > 0 && buffer == NULL )))
        {
            return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR;
        }
    #endif

    AZSTORAGE_RESULT result;
    HTTP_HEADERS_RESULT headers_result;
    AZSTORAGE_BLOB_STORAGE_CLIENT* cb = (AZSTORAGE_BLOB_STORAGE_CLIENT*)blob_storage_client;
    HTTP_HEADERS_HANDLE request_headers;
    HTTP_HEADERS_HANDLE response_headers;
    // TODO: error check snprintf() and write corresponding unit test
    snprintf(size_str, AZSTORAGE_SIZE_STR_SIZE, "%zu", buffer_len);

    /*[azstorage_blob_put_http_request_headers_OOM_FAIL]*/
    if ((request_headers = HTTPHeaders_Alloc()) == NULL)
    {
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP request header.", result);
    }
    /*[azstorage_blob_put_http_response_headers_OOM_FAIL]*/
    else if ((response_headers = HTTPHeaders_Alloc()) == NULL)
    {
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP response header.", result);
    }
    /*[azstorage_blob_put_http_add_header_blob_type_FAIL]*/
    else if ((headers_result = HTTPHeaders_AddHeaderNameValuePair(request_headers, AZSTORAGE_BLOB_TYPE_HEADER_NAME,
        blob_type_enum_to_string(blob_type)) != HTTP_HEADERS_OK))
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_REQUEST_HEADER_ERROR;
        LogError("ERROR 0x%x: failure adding blob_type to HTTP header.", headers_result);
    }
    /*[azstorage_blob_put_http_add_header_content_length_FAIL]*/
    else if ((headers_result = HTTPHeaders_AddHeaderNameValuePair(request_headers, AZSTORAGE_CONTENT_LENGTH_HEADER_NAME,
        size_str) != HTTP_HEADERS_OK))
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_REQUEST_HEADER_ERROR;
        LogError("ERROR 0x%x: failure adding buffer_len to HTTP header.", headers_result);
    }
    /*[azstorage_blob_put_http_add_header_host_FAIL]*/
    /*[azstorage_blob_put_http_request_FAIL]*/
    /*[azstorage_blob_put_http_status_code_FAIL]*/
    else if((result = send_request(cb, HTTPAPI_REQUEST_PUT, blob_path, request_headers, buffer, buffer_len,
        response_headers, NULL)) != AZSTORAGE_SUCCESS)
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        LogError("ERROR 0x%x: failure sending HTTP request.", result);
    }
    /*[azstorage_blob_put_SUCCESS]*/
    else
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        LogInfo("azstorage_blob_put success.");
    }

    return result;
}

AZSTORAGE_RESULT azstorage_blob_append_block(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, const unsigned char* buffer, size_t buffer_len)
{
    #ifdef AZSTORAGE_BLOB_VALIDATE_ARGUMENTS
    UCONTRACT
    (
        UCONTRACT_REQUIRE_NOT_NULL(blob_storage_client, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        UCONTRACT_REQUIRE_NOT_NULL(blob_path, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        UCONTRACT_REQUIRE_NOT_NULL(buffer, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        UCONTRACT_REQUIRE_NOT_EQUALS(buffer_len, 0, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR)
    );
    #else
        if (blob_storage_client == NULL || blob_path == NULL || buffer == NULL || buffer_len == 0)
        {
            return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR;
        }
    #endif

    AZSTORAGE_RESULT result;
    HTTP_HEADERS_RESULT headers_result;
    AZSTORAGE_BLOB_STORAGE_CLIENT* cb = (AZSTORAGE_BLOB_STORAGE_CLIENT*)blob_storage_client;
    HTTP_HEADERS_HANDLE request_headers;
    HTTP_HEADERS_HANDLE response_headers;
    // TODO: error check snprintf() and write corresponding unit test
    snprintf(size_str, AZSTORAGE_SIZE_STR_SIZE, "%zu", buffer_len);

    /*[azstorage_blob_append_block_http_request_headers_OOM_FAIL]*/
    if ((request_headers = HTTPHeaders_Alloc()) == NULL)
    {
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP header.", result);
    }
    /*[azstorage_blob_append_block_http_response_headers_OOM_FAIL]*/
    else if ((response_headers = HTTPHeaders_Alloc()) == NULL)
    {
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP response header.", result);
    }
    /*[azstorage_blob_append_block_http_add_header_content_length_FAIL]*/
    else if ((headers_result = HTTPHeaders_AddHeaderNameValuePair(request_headers, AZSTORAGE_CONTENT_LENGTH_HEADER_NAME,
        size_str)) != HTTP_HEADERS_OK)
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_REQUEST_HEADER_ERROR;
        LogError("ERROR 0x%x: failure adding buffer_len to HTTP header.", headers_result);
    }
    /*[azstorage_blob_append_block_http_add_header_host_FAIL]*/
    /*[azstorage_blob_append_block_http_request_FAIL]*/
    /*[azstorage_blob_append_block_http_status_code_FAIL]*/
    else if((result = send_request(cb, HTTPAPI_REQUEST_PUT, blob_path, request_headers, buffer, buffer_len, response_headers,
        NULL)) != AZSTORAGE_SUCCESS)
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        LogError("ERROR 0x%x: failure sending HTTP request.", result);
    }
    /*[azstorage_blob_append_block_SUCCESS]*/
    else
    {
        memset(size_str, 0, AZSTORAGE_SIZE_STR_SIZE);
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        LogInfo("azstorage_blob_append_block success.");
    }

    return result;
}

AZSTORAGE_RESULT azstorage_blob_get_metadata(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, HTTP_HEADERS_HANDLE response_headers)
{
    #ifdef AZSTORAGE_BLOB_VALIDATE_ARGUMENTS
    UCONTRACT
    (
        /*[azstorage_blob_get_metadata_blob_storage_client_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_storage_client, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_get_metadata_blob_path_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_path, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_get_metadata_response_header_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(response_headers, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR)
    );
    #else
        if (blob_storage_client == NULL || blob_path == NULL || response_headers == NULL)
        {
            return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR;
        }
    #endif

    AZSTORAGE_RESULT result;
    AZSTORAGE_BLOB_STORAGE_CLIENT* cb = (AZSTORAGE_BLOB_STORAGE_CLIENT*)blob_storage_client;
    HTTP_HEADERS_HANDLE request_header;

    /*[azstorage_blob_get_metadata_http_headers_OOM_FAIL]*/
    if ((request_header = HTTPHeaders_Alloc()) == NULL)
    {
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP header.", result);
    }
    /*[azstorage_blob_get_metadata_http_add_header_host_FAIL]*/
    /*[aazstorage_blob_get_metadata_http_request_FAIL]*/
    /*[azstorage_blob_get_metadata_http_status_code_FAIL]*/
    else if((result = send_request(cb, HTTPAPI_REQUEST_HEAD, blob_path, request_header, NULL, 0, response_headers,
        NULL)) != AZSTORAGE_SUCCESS)
    {
        HTTPHeaders_Free(request_header);
        LogError("ERROR 0x%x: failure sending HTTP request.", result);
    }
    /*[azstorage_blob_get_metadata_SUCCESS]*/
    else
    {
        HTTPHeaders_Free(request_header);
        LogInfo("azstorage_blob_get_metadata success.");
    }

    return result;
}

AZSTORAGE_RESULT azstorage_blob_get_bytes(AZSTORAGE_BLOB_HANDLE blob_storage_client,
                                    const char* blob_path, unsigned char* buffer, size_t buffer_len,
                                    offset_t start_offset)
{
    #ifdef AZSTORAGE_BLOB_VALIDATE_ARGUMENTS
    UCONTRACT
    (
        /*[azstorage_blob_get_bytes_blob_storage_client_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_storage_client, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_get_bytes_blob_path_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(blob_path, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_get_bytes_buffer_NULL_FAIL]*/
        UCONTRACT_REQUIRE_NOT_NULL(buffer, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR),
        /*[azstorage_blob_get_bytes_buffer_len_zero_FAIL]*/
        UCONTRACT_REQUIRE_NOT_EQUALS(buffer_len, 0, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR)
    );
    #else
        if (blob_storage_client == NULL || blob_path == NULL || buffer == NULL || buffer_len == 0)
        {
            return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR;
        }
    #endif

    AZSTORAGE_RESULT result;
    HTTP_HEADERS_RESULT headers_result;
    AZSTORAGE_BLOB_STORAGE_CLIENT* cb = (AZSTORAGE_BLOB_STORAGE_CLIENT*)blob_storage_client;
    HTTP_HEADERS_HANDLE request_headers;
    HTTP_HEADERS_HANDLE response_headers;
    BUFFER_HANDLE response_buffer_ref;
    char byte_range_buffer[AZSTORAGE_BYTERANGE_HEADER_SIZE];
    // TODO: error check snprintf() and write corresponding unit test
    snprintf(byte_range_buffer, AZSTORAGE_BYTERANGE_HEADER_SIZE,
        "bytes=%zd-%zd", start_offset, start_offset + buffer_len - 1);

    /*[azstorage_blob_get_bytes_http_request_headers_OOM_FAIL]*/
    if ((request_headers = HTTPHeaders_Alloc()) == NULL)
    {
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP header.", result);
    }
    /*[azstorage_blob_get_bytes_http_response_headers_OOM_FAIL]*/
    else if ((response_headers = HTTPHeaders_Alloc()) == NULL)
    {
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for HTTP header.", result);
    }
    /*[azstorage_blob_get_bytes_response_buffer_handle_OOM_FAIL]*/
    else if ((response_buffer_ref = BUFFER_new()) == NULL)
    {
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_MEMORY_ERROR;
        LogError("ERROR 0x%x: failure allocating memory for BUFFER_HANDLE.", result);
    }
    /*[azstorage_blob_get_bytes_http_add_header_byte_range_FAIL]*/
    else if ((headers_result = HTTPHeaders_AddHeaderNameValuePair(request_headers, AZSTORAGE_BYTE_RANGE_HEADER_NAME,
        byte_range_buffer)) != HTTP_HEADERS_OK)
    {
        BUFFER_delete(response_buffer_ref);
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        result = AZSTORAGE_REQUEST_HEADER_ERROR;
        LogError("ERROR 0x%x: failure adding byte_range_ref to HTTP header.", headers_result);
    }
    /*[azstorage_blob_get_bytes_http_add_header_host_FAIL]*/
    /*[azstorage_blob_get_bytes_http_request_FAIL]*/
    /*[azstorage_blob_get_bytes_http_status_code_FAIL]*/
    else if((result = send_request(cb, HTTPAPI_REQUEST_GET, blob_path, request_headers, NULL, 0, response_headers,
        response_buffer_ref)) != AZSTORAGE_SUCCESS)
    {
        BUFFER_delete(response_buffer_ref);
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        LogError("ERROR 0x%x: failure sending http request.", result);
    }
    /*[azstorage_blob_get_bytes_SUCCESS]*/
    else
    {
        // if we have successfully completed the request, copy data from response_buffer_ref to our param buffer
        (void)memcpy(buffer, (unsigned char*)BUFFER_u_char(response_buffer_ref), BUFFER_length(response_buffer_ref));

        // free up internal memory
        BUFFER_delete(response_buffer_ref);
        HTTPHeaders_Free(response_headers);
        HTTPHeaders_Free(request_headers);
        LogInfo("azstorage_blob_get_bytes [%zd-%zd] success.", start_offset, start_offset + buffer_len - 1);
    }

    return result;
}
