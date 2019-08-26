
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#ifdef __cplusplus
#include <cstdbool>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umock_c_negative_tests.h"

static TEST_MUTEX_HANDLE g_test_by_test;
static TEST_MUTEX_HANDLE g_dll_by_dll;

#define ENABLE_MOCKS

#undef GB_USE_CUSTOM_HEAP

#ifndef GB_DEBUG_ALLOC
#define GB_DEBUG_ALLOC
#endif

#include "azure_c_shared_utility/gballoc.h"
#include "azure_c_shared_utility/buffer_.h"
#include "azure_c_shared_utility/httpheaders.h"
#include "azure_c_shared_utility/httpapi.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/strings.h"
#define GB_USE_CUSTOM_HEAP
#undef GB_DEBUG_ALLOC

#undef ENABLE_MOCKS

// the functions to test
#include "azstorage_blob_api.h"

// #define IGNORED_PTR_ARG (NULL)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

#define TEST_CONNECTION_HANDLER         (HTTP_HANDLE) 0x50
#define TEST_HTTP_HEADER_HANDLE         (HTTP_HEADERS_HANDLE)0x51
#define TEST_STRING_HANDLE              (STRING_HANDLE)0x52
#define TEST_BUFFER_HANDLE              (BUFFER_HANDLE)0x53


IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE_VALUES);

HTTP_HEADERS_HANDLE my_HTTPHeaders_Alloc(void)
{
    return TEST_HTTP_HEADER_HANDLE;
}
void my_HTTPHeaders_Free(HTTP_HEADERS_HANDLE handle)
{
}

static bool httpapi_execute_request_io_error;
static unsigned int httpapi_execute_request_http_status;
static char* httpapi_execute_request_http_content = "OK";
HTTPAPI_RESULT my_HTTPAPI_ExecuteRequest(HTTP_HANDLE handle, HTTPAPI_REQUEST_TYPE requestType, const char* relativePath,
    HTTP_HEADERS_HANDLE httpHeadersHandle, const unsigned char* content,
    size_t contentLength, unsigned int* statusCode,
    HTTP_HEADERS_HANDLE responseHeadersHandle, BUFFER_HANDLE responseContent)
{
    HTTPAPI_RESULT result;

    if (httpapi_execute_request_io_error)
    {
        result = HTTPAPI_ERROR;
    }
    else
    {
        *statusCode = httpapi_execute_request_http_status;
        result = HTTPAPI_OK;
    }

    return result;
}

static AZSTORAGE_BLOB test_blob_storage_client;
static const char* test_service_uri = "unitteststorageaccount.blob.core.windows.net";
static const char* test_blob_path_with_sas = "/test/fake_blob.txt?unit_test_sas";
static AZSTORAGE_BLOB_TYPE test_blob_type = AZSTORAGE_BLOB_TYPE_APPEND_BLOB;
#define TEST_BUFFER_SIZE 12
static const unsigned char test_buffer[TEST_BUFFER_SIZE] = "0123456789\n";
#define TEST_OFFSET 0
#define TEST_BYTE_RANGE_BUFFER_SIZE 33
static char test_byte_range_buffer[TEST_BYTE_RANGE_BUFFER_SIZE] = "bytes=100000000000-199999999999";
#define TEST_RESPONSE_BUFFER_SIZE 20
static unsigned char test_response_buffer[TEST_RESPONSE_BUFFER_SIZE] = "my downloaded file\n";
static unsigned char test_response_buffer_destination[TEST_RESPONSE_BUFFER_SIZE] = { 0 };

/**
 * Beginning of the UT for azstorage_blob.c module.
 */
BEGIN_TEST_SUITE(azstorage_blob_ut)

TEST_SUITE_INITIALIZE(suite_init)
{
    g_test_by_test = TEST_MUTEX_CREATE();
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    REGISTER_TYPE(HTTP_HEADERS_RESULT, HTTP_HEADERS_RESULT);
    REGISTER_TYPE(HTTPAPI_REQUEST_TYPE, HTTPAPI_REQUEST_TYPE);
    REGISTER_TYPE(HTTPAPI_RESULT, HTTPAPI_RESULT);

    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(HTTP_HEADERS_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(BUFFER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(STRING_HANDLE, void*);

    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Alloc, my_HTTPHeaders_Alloc);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPHeaders_Free, my_HTTPHeaders_Free);
    REGISTER_GLOBAL_MOCK_HOOK(HTTPAPI_ExecuteRequest, my_HTTPAPI_ExecuteRequest);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_test_by_test);
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
    if (TEST_MUTEX_ACQUIRE(g_test_by_test))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }

    memset(&test_blob_storage_client, 0, sizeof(test_blob_storage_client));
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
    TEST_MUTEX_RELEASE(g_test_by_test);
}


/*
 * ********************* azstorage_blob_storage_client_create() unit tests *********************
 */
/* azstorage_blob_storage_client_create shall return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR
if AZSTORAGE_BLOB_HANDLE blob_storage_client argument is NULL */
TEST_FUNCTION(azstorage_blob_service_client_create_blob_storage_client_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_storage_client_create(NULL, test_service_uri);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_storage_client_create shall return AZSTORAGE_ILLEGAL_ARGUMENT_ERROR
if const char* const service_uri argument is NULL */
TEST_FUNCTION(azstorage_blob_service_client_create_service_uri_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_storage_client_create(&test_blob_storage_client, NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_storage_client_create shall return AZSTORAGE_CREATE_ERROR
if HTTPAPI_CreateConnection returns NULL */
TEST_FUNCTION(azstorage_blob_service_client_create_http_connection_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPAPI_CreateConnection(test_service_uri)).SetReturn(NULL);

    ///act
    test_result = azstorage_blob_storage_client_create(&test_blob_storage_client, test_service_uri);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_CREATE_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_storage_client_create shall return AZSTORAGE_SUCCESS
if no errors occur */
TEST_FUNCTION(azstorage_blob_service_client_create_SUCCESS)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPAPI_CreateConnection(test_service_uri)).SetReturn(TEST_CONNECTION_HANDLER);

    ///act
    test_result = azstorage_blob_storage_client_create(&test_blob_storage_client, test_service_uri);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SUCCESS, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_service_client_destroy() unit tests *********************
 */

/* azstorage_blob_storage_client_destroy shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided client handle is NULL*/
TEST_FUNCTION(azstorage_blob_storage_client_destroy_blob_storage_client_NULL_FAIL)
{
    ///act
    azstorage_blob_storage_client_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_storage_client_destroy shall return AZSTORAGE_NOT_IMPLEMENTED_ERROR
if the provided client handle does not have an initialized http_handle */
TEST_FUNCTION(azstorage_blob_storage_client_destroy_client_not_implemented_FAIL)
{
    ///arrange
    STRICT_EXPECTED_CALL(HTTPAPI_CreateConnection(test_service_uri)).SetReturn(NULL);
    (void)azstorage_blob_storage_client_create(&test_blob_storage_client, test_service_uri);
    umock_c_reset_all_calls();

    ///act
    azstorage_blob_storage_client_destroy(&test_blob_storage_client);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_storage_client_destroy shall destroy the http connection and return AZ_STORAGE_CLIENT_SUCCESS
if no errors occur */
TEST_FUNCTION(azstorage_blob_storage_client_destroy_SUCCESS)
{
    ///arrange
    STRICT_EXPECTED_CALL(HTTPAPI_CreateConnection(test_service_uri)).SetReturn(TEST_CONNECTION_HANDLER);
    (void)azstorage_blob_storage_client_create(&test_blob_storage_client, test_service_uri);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(HTTPAPI_CloseConnection(TEST_CONNECTION_HANDLER));

    ///act
    azstorage_blob_storage_client_destroy(&test_blob_storage_client);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/*
 * ********************* azstorage_blob_put() unit tests *********************
 */
/* azstorage_blob_put interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob storage client is NULL. */
TEST_FUNCTION(azstorage_blob_put_blob_storage_client_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_put(
                    NULL,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob type >= (int)AZSTORAGE_BLOB_TYPE_APPEND_BLOB. */
TEST_FUNCTION(azstorage_blob_put_blob_type_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    (AZSTORAGE_BLOB_TYPE)2,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob path is NULL. */
TEST_FUNCTION(azstorage_blob_put_blob_path_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    NULL,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob buffer is NULL and the corresponding buffer_len is not 0. */
TEST_FUNCTION(azstorage_blob_put_buffer_NULL_buffer_len_not_ZERO_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    NULL,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob buffer is not NULL and the corresponding buffer_len is 0. */
TEST_FUNCTION(azstorage_blob_put_buffer_not_NULL_buffer_len_ZERO_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    0);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http request headers */
TEST_FUNCTION(azstorage_blob_put_http_request_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}
/* azstorage_blob_put shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http response header */
TEST_FUNCTION(azstorage_blob_put_http_response_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the blob type header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_put_http_add_header_blob_type_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the content length header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_put_http_add_header_content_length_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_put() -> send_request() unit tests *********************
 */

/* azstorage_blob_put shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the host header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_put_http_add_header_host_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put shall return AZSTORAGE_REQUEST_ERROR
if the HTTPAPI_ExecuteRequest() produces an error with the request */
TEST_FUNCTION(azstorage_blob_put_http_request_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = true;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            test_buffer,
            TEST_BUFFER_SIZE,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put shall return AZSTORAGE_SERVICE_ERROR
if the http status code is outside bounds (200,300] */
TEST_FUNCTION(azstorage_blob_put_http_status_code_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 404;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            test_buffer,
            TEST_BUFFER_SIZE,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SERVICE_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_put shall return AZSTORAGE_SUCCESS
if no errors occur*/
TEST_FUNCTION(azstorage_blob_put_SUCCESS)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 201;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            test_buffer,
            TEST_BUFFER_SIZE,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_put(
                    &test_blob_storage_client,
                    test_blob_type,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SUCCESS, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_append_block() unit tests *********************
 */

/* azstorage_blob_append_block interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob storage client is NULL. */
TEST_FUNCTION(azstorage_blob_append_block_blob_storage_client_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_append_block(
                    NULL,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob path is NULL. */
TEST_FUNCTION(azstorage_blob_append_block_blob_path_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    NULL,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided buffer is NULL. */
TEST_FUNCTION(azstorage_blob_append_block_buffer_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    NULL,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided buffer length is zero. */
TEST_FUNCTION(azstorage_blob_append_block_buffer_len_zero_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    0);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http request headers */
TEST_FUNCTION(azstorage_blob_append_block_http_request_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http response headers */
TEST_FUNCTION(azstorage_blob_append_block_http_response_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


/* azstorage_blob_append_block shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the content length header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_append_block_http_add_header_content_length_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_append_block() -> send_request() unit tests *********************
 */

/* azstorage_blob_append_block shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the host header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_append_block_http_add_header_host_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block shall return AZSTORAGE_REQUEST_ERROR
if the HTTPAPI_ExecuteRequest() produces an error with the request */
TEST_FUNCTION(azstorage_blob_append_block_http_request_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = true;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            test_buffer,
            TEST_BUFFER_SIZE,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block shall return AZSTORAGE_SERVICE_ERROR
if the http status code is outside bounds (200,300] */
TEST_FUNCTION(azstorage_blob_append_block_http_status_code_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 404;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            test_buffer,
            TEST_BUFFER_SIZE,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SERVICE_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_append_block shall return AZSTORAGE_SUCCESS
if no errors occur*/
TEST_FUNCTION(azstorage_blob_append_block_SUCCESS)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 201;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_PUT,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            test_buffer,
            TEST_BUFFER_SIZE,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_append_block(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_buffer,
                    TEST_BUFFER_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SUCCESS, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_get_bytes() unit tests *********************
 */

/* azstorage_blob_get_bytes interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob storage client is NULL. */
TEST_FUNCTION(azstorage_blob_get_bytes_blob_storage_client_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_bytes(
                    NULL,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob path is NULL. */
TEST_FUNCTION(azstorage_blob_get_bytes_blob_path_NULL_FAIL)
{
    //arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    NULL,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided buffer is NULL. */
TEST_FUNCTION(azstorage_blob_get_bytes_buffer_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    NULL,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided buffer length is zero. */
TEST_FUNCTION(azstorage_blob_get_bytes_buffer_len_zero_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    0,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http request headers */
TEST_FUNCTION(azstorage_blob_get_bytes_http_request_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http response headers */
TEST_FUNCTION(azstorage_blob_get_bytes_http_response_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_MEMORY_ERROR
if the response buffer handle cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_get_bytes_response_buffer_handle_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new()).SetReturn(NULL);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the byte range header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_get_bytes_http_add_header_byte_range_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new()).SetReturn(TEST_BUFFER_HANDLE);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(BUFFER_delete(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_get_bytes() -> send_request() unit tests *********************
 */

/* azstorage_blob_get_bytes shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the host header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_get_bytes_http_add_header_host_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new()).SetReturn(TEST_BUFFER_HANDLE);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(BUFFER_delete(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_REQUEST_ERROR
if the HTTPAPI_ExecuteRequest() produces an error with the request */
TEST_FUNCTION(azstorage_blob_get_bytes_http_request_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new()).SetReturn(TEST_BUFFER_HANDLE);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = true;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_GET,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            NULL,
            0,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            TEST_BUFFER_HANDLE))
            .IgnoreArgument(1)
            .IgnoreArgument(6);
    STRICT_EXPECTED_CALL(BUFFER_delete(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_SERVICE_ERROR
if the http status code is outside bounds (200,300] */
TEST_FUNCTION(azstorage_blob_get_bytes_http_status_code_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new()).SetReturn(TEST_BUFFER_HANDLE);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 404;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_GET,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            NULL,
            0,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            TEST_BUFFER_HANDLE))
            .IgnoreArgument(1)
            .IgnoreArgument(6);
    STRICT_EXPECTED_CALL(BUFFER_delete(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SERVICE_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_bytes shall return AZSTORAGE_SUCCESS
if no errors occur*/
TEST_FUNCTION(azstorage_blob_get_bytes_SUCCESS)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(BUFFER_new()).SetReturn(TEST_BUFFER_HANDLE);
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 201;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_GET,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            NULL,
            0,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            TEST_BUFFER_HANDLE))
            .IgnoreArgument(1)
            .IgnoreArgument(6);
    STRICT_EXPECTED_CALL(BUFFER_length(IGNORED_PTR_ARG))
                        .IgnoreAllArguments()
                        .SetReturn(TEST_RESPONSE_BUFFER_SIZE);
    STRICT_EXPECTED_CALL(BUFFER_u_char(IGNORED_PTR_ARG))
                        .IgnoreAllArguments()
                        .SetReturn(test_response_buffer);
    STRICT_EXPECTED_CALL(BUFFER_delete(TEST_BUFFER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_bytes(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    test_response_buffer_destination,
                    TEST_BUFFER_SIZE,
                    TEST_OFFSET);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SUCCESS, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_get_metadata() unit tests *********************
 */
/* azstorage_blob_get_metadata interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob storage client is NULL. */
TEST_FUNCTION(azstorage_blob_get_metadata_blob_storage_client_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_metadata(
                    NULL,
                    test_blob_path_with_sas,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_metadata interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided blob path is NULL. */
TEST_FUNCTION(azstorage_blob_get_metadata_blob_path_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    NULL,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_metadata interface shall return AZ_STORAGE_CLIENT_INVALID_ARG_ERROR
if the provided response header is NULL. */
TEST_FUNCTION(azstorage_blob_get_metadata_response_header_NULL_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    NULL);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_ILLEGAL_ARGUMENT_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_metadata shall return AZSTORAGE_MEMORY_ERROR
if memory cannot be allocated for http headers */
TEST_FUNCTION(azstorage_blob_get_metadata_http_headers_OOM_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc()).SetReturn(NULL);

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_MEMORY_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*
 * ********************* azstorage_blob_get_metadata() -> send_request() unit tests *********************
 */

/* azstorage_blob_get_metadata shall return AZSTORAGE_REQUEST_HEADER_ERROR
if the host header cannot be added to the http request */
TEST_FUNCTION(azstorage_blob_get_metadata_http_add_header_host_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_ERROR).IgnoreAllArguments();
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_HEADER_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_metadata shall return AZSTORAGE_REQUEST_ERROR
if the HTTPAPI_ExecuteRequest() produces an error with the request */
TEST_FUNCTION(aazstorage_blob_get_metadata_http_request_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = true;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_HEAD,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            NULL,
            0,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_REQUEST_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_metadata shall return AZSTORAGE_SERVICE_ERROR
if the http status code is outside bounds (200,300] */
TEST_FUNCTION(azstorage_blob_get_metadata_http_status_code_FAIL)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 404;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_HEAD,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            NULL,
            0,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SERVICE_ERROR, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* azstorage_blob_get_metadata shall return AZSTORAGE_SUCCESS
if no errors occur*/
TEST_FUNCTION(azstorage_blob_get_metadata_SUCCESS)
{
    ///arrange
    AZSTORAGE_RESULT test_result;
    STRICT_EXPECTED_CALL(HTTPHeaders_Alloc());
    STRICT_EXPECTED_CALL(HTTPHeaders_AddHeaderNameValuePair(IGNORED_PTR_ARG, "", "")
                        ).SetReturn(HTTP_HEADERS_OK).IgnoreAllArguments();
    httpapi_execute_request_io_error = false;
    httpapi_execute_request_http_status = 201;
    STRICT_EXPECTED_CALL(HTTPAPI_ExecuteRequest(
            IGNORED_PTR_ARG,
            HTTPAPI_REQUEST_HEAD,
            test_blob_path_with_sas,
            TEST_HTTP_HEADER_HANDLE,
            NULL,
            0,
            IGNORED_PTR_ARG,
            TEST_HTTP_HEADER_HANDLE,
            IGNORED_PTR_ARG))
            .IgnoreArgument(1)
            .IgnoreArgument(6)
            .IgnoreArgument(8);
    STRICT_EXPECTED_CALL(HTTPHeaders_Free(TEST_HTTP_HEADER_HANDLE));

    ///act
    test_result = azstorage_blob_get_metadata(
                    &test_blob_storage_client,
                    test_blob_path_with_sas,
                    TEST_HTTP_HEADER_HANDLE);

    ///assert
    ASSERT_ARE_EQUAL(int, AZSTORAGE_SUCCESS, test_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(azstorage_blob_ut)
