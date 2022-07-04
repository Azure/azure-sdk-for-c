// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <cmocka.h>

#include <azure/storage/az_storage_blobs.h>

#include <azure/core/az_http_transport.h>
#include <azure/core/az_version.h>

#include <az_test_precondition.h>

#include "_az_test_http_client.h"

#include <azure/core/_az_cfg.h>

#define _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID "azsdk-c-storage-blobs/" AZ_SDK_VERSION_STRING
#define _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID_LENGTH \
  (sizeof(_az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID) - 1)


#define PREPARE_DEFAULT_CLIENT_AND_RESPONSE()                                                      \
      block_count = 0;                                                                             \
      az_storage_blobs_blob_client client = { 0 };                                                 \
      assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(                           \
      &client,                                                                                     \
      AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"                    \
                       "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="  \
                       "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D"),                             \
      AZ_CREDENTIAL_ANONYMOUS,                                                                     \
      NULL)));                                                                                     \
      uint8_t response_buffer[1024 * 4] = { 0 };                                                   \
      az_http_response response = { 0 };                                                           \
      assert_true(                                                                                 \
          az_result_succeeded(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buffer))));

static const char* base64_block_ids_table[110] = {
  "ICAw","ICAx","ICAy","ICAz","ICA0","ICA1","ICA2","ICA3","ICA4","ICA5","IDEw","IDEx","IDEy","IDEz","IDE0",
  "IDE1","IDE2","IDE3","IDE4","IDE5","IDIw","IDIx","IDIy","IDIz","IDI0","IDI1","IDI2","IDI3","IDI4","IDI5",
  "IDMw","IDMx","IDMy","IDMz","IDM0","IDM1","IDM2","IDM3","IDM4","IDM5","IDQw","IDQx","IDQy","IDQz","IDQ0",
  "IDQ1","IDQ2","IDQ3","IDQ4","IDQ5","IDUw","IDUx","IDUy","IDUz","IDU0","IDU1","IDU2","IDU3","IDU4","IDU5",
  "IDYw","IDYx","IDYy","IDYz","IDY0","IDY1","IDY2","IDY3","IDY4","IDY5","IDcw","IDcx","IDcy","IDcz","IDc0",
  "IDc1","IDc2","IDc3","IDc4","IDc5","IDgw","IDgx","IDgy","IDgz","IDg0","IDg1","IDg2","IDg3","IDg4","IDg5",
  "IDkw","IDkx","IDky","IDkz","IDk0","IDk1","IDk2","IDk3","IDk4","IDk5","MTAw","MTAx","MTAy","MTAz","MTA0",
  "MTA1","MTA2","MTA3","MTA4","MTA5"
};

#define BLOCK_LIST_BODY "<?xml version=\"1.0\" encoding=\"utf-8\"?><BlockList>" \
                        "<Latest>ICAw</Latest><Latest>ICAx</Latest><Latest>ICAy</Latest>" \
                        "<Latest>ICAz</Latest><Latest>ICA0</Latest><Latest>ICA1</Latest>" \
                        "<Latest>ICA2</Latest><Latest>ICA3</Latest><Latest>ICA4</Latest>" \
                        "<Latest>ICA5</Latest><Latest>IDEw</Latest><Latest>IDEx</Latest>" \
                        "<Latest>IDEy</Latest><Latest>IDEz</Latest><Latest>IDE0</Latest>" \
                        "<Latest>IDE1</Latest><Latest>IDE2</Latest><Latest>IDE3</Latest>" \
                        "<Latest>IDE4</Latest><Latest>IDE5</Latest><Latest>IDIw</Latest>" \
                        "<Latest>IDIx</Latest><Latest>IDIy</Latest><Latest>IDIz</Latest>" \
                        "<Latest>IDI0</Latest><Latest>IDI1</Latest><Latest>IDI2</Latest>" \
                        "<Latest>IDI3</Latest><Latest>IDI4</Latest><Latest>IDI5</Latest>" \
                        "<Latest>IDMw</Latest><Latest>IDMx</Latest><Latest>IDMy</Latest>" \
                        "<Latest>IDMz</Latest><Latest>IDM0</Latest><Latest>IDM1</Latest>" \
                        "<Latest>IDM2</Latest><Latest>IDM3</Latest><Latest>IDM4</Latest>" \
                        "<Latest>IDM5</Latest><Latest>IDQw</Latest><Latest>IDQx</Latest>" \
                        "<Latest>IDQy</Latest><Latest>IDQz</Latest><Latest>IDQ0</Latest>" \
                        "<Latest>IDQ1</Latest><Latest>IDQ2</Latest><Latest>IDQ3</Latest>" \
                        "<Latest>IDQ4</Latest><Latest>IDQ5</Latest><Latest>IDUw</Latest>" \
                        "<Latest>IDUx</Latest><Latest>IDUy</Latest><Latest>IDUz</Latest>" \
                        "<Latest>IDU0</Latest><Latest>IDU1</Latest><Latest>IDU2</Latest>" \
                        "<Latest>IDU3</Latest><Latest>IDU4</Latest><Latest>IDU5</Latest>" \
                        "<Latest>IDYw</Latest><Latest>IDYx</Latest><Latest>IDYy</Latest>" \
                        "<Latest>IDYz</Latest><Latest>IDY0</Latest><Latest>IDY1</Latest>" \
                        "<Latest>IDY2</Latest><Latest>IDY3</Latest><Latest>IDY4</Latest>" \
                        "<Latest>IDY5</Latest><Latest>IDcw</Latest><Latest>IDcx</Latest>" \
                        "<Latest>IDcy</Latest><Latest>IDcz</Latest><Latest>IDc0</Latest>" \
                        "<Latest>IDc1</Latest><Latest>IDc2</Latest><Latest>IDc3</Latest>" \
                        "<Latest>IDc4</Latest><Latest>IDc5</Latest><Latest>IDgw</Latest>" \
                        "<Latest>IDgx</Latest><Latest>IDgy</Latest><Latest>IDgz</Latest>" \
                        "<Latest>IDg0</Latest><Latest>IDg1</Latest><Latest>IDg2</Latest>" \
                        "<Latest>IDg3</Latest><Latest>IDg4</Latest><Latest>IDg5</Latest>" \
                        "<Latest>IDkw</Latest><Latest>IDkx</Latest><Latest>IDky</Latest>" \
                        "<Latest>IDkz</Latest><Latest>IDk0</Latest><Latest>IDk1</Latest>" \
                        "<Latest>IDk2</Latest><Latest>IDk3</Latest><Latest>IDk4</Latest>" \
                        "<Latest>IDk5</Latest><Latest>MTAw</Latest><Latest>MTAx</Latest>" \
                        "<Latest>MTAy</Latest><Latest>MTAz</Latest><Latest>MTA0</Latest>" \
                        "<Latest>MTA1</Latest><Latest>MTA2</Latest><Latest>MTA3</Latest>" \
                        "<Latest>MTA4</Latest><Latest>MTA5</Latest></BlockList>"


static int  block_count = 0;

static az_result get_data_callback(az_span *data_block)
{
  if (block_count < 110)
  {
    *data_block = AZ_SPAN_FROM_STR("MultiblockBlobContent");
    block_count++;
  }
  else
  {
    *data_block = AZ_SPAN_EMPTY;
    block_count = 0;
  }

  return AZ_OK;
}

static az_result get_data_callback_failed(az_span *data_block)
{
  (void)data_block;
  return AZ_ERROR_NOT_ENOUGH_SPACE;
}

static az_result get_data_callback_failed_second(az_span *data_block)
{
    if (block_count == 0)
    {
        *data_block = AZ_SPAN_FROM_STR("MultiblockBlobContent");
        block_count++;
    }
    else
    {
        return AZ_ERROR_NOT_ENOUGH_SPACE;
    }
    return AZ_OK;
}


static az_result get_data_callback_block_id_too_high(az_span *data_block)
{
  if (block_count < 1001)
  {
    *data_block = AZ_SPAN_FROM_STR("MultiblockBlobContent");
    block_count++;
  }
  else
  {
    *data_block = AZ_SPAN_EMPTY;
    block_count = 0;
  }

  return AZ_OK;
}

static az_result no_op_transport(az_http_request const* request, az_http_response* ref_response)
{
  (void)request;

    az_span_fill(ref_response->_internal.http_response, 0);
    strcpy((char*)az_span_ptr(ref_response->_internal.http_response), "HTTP/1.1 201 Created\r\n"
                                                                      "Content-Length: 0\r\n"
                                                                      "Content-MD5: ZF1nmzmalQE57vKStFOEkw==\r\n"
                                                                      "Last-Modified: Thu, 07 Oct 2021 19:35:26 GMT\r\n"
                                                                      "ETag: \"0x8D989C99D2311EA\"\r\n"
                                                                      "Server: Windows-Azure-Blob/1.0 Microsoft-HTTPAPI/2.0\r\n"
                                                                      "x-ms-request-id: c1602c46-101e-00d7-15b2-bb381b000000\r\n"
                                                                      "x-ms-version: 2019-02-02\r\n"
                                                                      "x-ms-content-crc64: Ezo6E5wD1vI=\r\n"
                                                                      "x-ms-request-server-encrypted: true\r\n"
                                                                      "Date: Thu, 07 Oct 2021 19:35:26 GMT\r\n"
                                                                      "\r\n");
    return AZ_OK;
}

static az_result transport_failed(az_http_request const* request, az_http_response* ref_response)
{
    (void)request;

    az_span_fill(ref_response->_internal.http_response, 0);
    strcpy((char*)az_span_ptr(ref_response->_internal.http_response), "HTTP/1.1 403 Forbidden\r\n"
                                                                      "Content-Length: 0\r\n");
    return AZ_OK;
}

static az_result verify_storage_blobs_upload(
    az_http_request const* request,
    az_http_response* ref_response)
{
  (void)request;
  (void)ref_response;
  (void)base64_block_ids_table;
  assert_non_null(request);
  assert_non_null(ref_response);

  {
    az_span http_method = { 0 };
    assert_true(az_result_succeeded(az_http_request_get_method(request, &http_method)));
    assert_true(az_span_is_content_equal(http_method, az_http_method_put()));
  }

  {
    az_span request_url = { 0 };
    assert_true(az_result_succeeded(az_http_request_get_url(request, &request_url)));


    char url_str[256];
    memset(url_str, 0, sizeof(url_str));

    if (block_count > 0)
    {
      strcat(url_str, "https://storageacct.blob.core.microsoft.com/container/"
                      "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                      "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D&comp=block&blockid=");
      strcat(url_str, base64_block_ids_table[block_count-1]);
    }
    else
    {
      strcat(url_str, "https://storageacct.blob.core.microsoft.com/container/"
                      "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                      "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D&comp=blocklist");
    }

    assert_true(az_span_is_content_equal(
        request_url,
        az_span_create_from_str(url_str)));
  }

  {
    az_span request_body = { 0 };
    assert_true(az_result_succeeded(az_http_request_get_body(request, &request_body)));

    if (block_count > 0) // sending blocks of data
    {
      assert_true(
          az_span_is_content_equal(request_body, AZ_SPAN_FROM_STR("MultiblockBlobContent")));
    }
    else // sending blocklist, as the block_count is being zeroed in the callback function
    {
      assert_true(
          az_span_is_content_equal(request_body, AZ_SPAN_FROM_STR(BLOCK_LIST_BODY)));
    }
  }

  {
    bool blob_type_header_found = false;
    bool content_length_header_found = false;
    bool content_type_header_found = false;
    bool host_header_found = false;
    bool api_version_header_found = false;
    bool user_agent_header_found = false;

    int32_t const headers_count = az_http_request_headers_count(request);
    for (int32_t i = 0; i < headers_count; ++i)
    {
      az_span header_name = { 0 };
      az_span header_value = { 0 };

      assert_true(
          az_result_succeeded(az_http_request_get_header(request, i, &header_name, &header_value)));

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("x-ms-blob-type")))
      {
        assert_false(blob_type_header_found);
        blob_type_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("BlockBlob")));
      }
      else if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Length")))
      {
        assert_false(content_length_header_found);
        content_length_header_found = true;

        if (block_count > 0)
        {
          assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("21")));
        }
        else
        {
          assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("2371")));
        }
      }
      else if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Type")))
      {
        assert_false(content_type_header_found);
        content_type_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("text/plain")));
      }
      else if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Host")))
      {
        assert_false(host_header_found);
        host_header_found = true;

        assert_true(az_span_is_content_equal(
            header_value, AZ_SPAN_FROM_STR("storageacct.blob.core.microsoft.com")));
      }
      else if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("x-ms-version")))
      {
        assert_false(api_version_header_found);
        api_version_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("2019-02-02")));
      }
      else if (az_span_is_content_equal_ignoring_case(header_name, AZ_SPAN_FROM_STR("User-Agent")))
      {
        assert_false(user_agent_header_found);
        user_agent_header_found = true;

        az_span const header_value_fragment
            = az_span_slice(header_value, 0, _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID_LENGTH);

        assert_true(az_span_is_content_equal(
            header_value_fragment, AZ_SPAN_FROM_STR(_az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID)));
      }
    }

    block_count ? assert_true(blob_type_header_found) : assert_false(blob_type_header_found);
    assert_true(content_length_header_found);
    assert_true(content_type_header_found);
    assert_true(host_header_found);
    assert_true(api_version_header_found);
    assert_true(user_agent_header_found);
  }

  az_span_fill(ref_response->_internal.http_response, 0);
  strcpy((char*)az_span_ptr(ref_response->_internal.http_response), "HTTP/1.1 201 Created\r\n"
                                                "Content-Length: 0\r\n"
                                                "Content-MD5: ZF1nmzmalQE57vKStFOEkw==\r\n"
                                                "Last-Modified: Thu, 07 Oct 2021 19:35:26 GMT\r\n"
                                                "ETag: \"0x8D989C99D2311EA\"\r\n"
                                                "Server: Windows-Azure-Blob/1.0 Microsoft-HTTPAPI/2.0\r\n"
                                                "x-ms-request-id: c1602c46-101e-00d7-15b2-bb381b000000\r\n"
                                                "x-ms-version: 2019-02-02\r\n"
                                                "x-ms-content-crc64: Ezo6E5wD1vI=\r\n"
                                                "x-ms-request-server-encrypted: true\r\n"
                                                "Date: Thu, 07 Oct 2021 19:35:26 GMT\r\n"
                                                "\r\n");

  return AZ_OK;
}

void test_storage_blobs_multiblock_blob_upload(void** state);
void test_storage_blobs_multiblock_blob_upload(void** state)
{
  (void)state;
  PREPARE_DEFAULT_CLIENT_AND_RESPONSE()
  _az_http_client_set_callback(verify_storage_blobs_upload);

  assert_true(az_result_succeeded(az_storage_blobs_multiblock_blob_upload(
      &client, NULL, get_data_callback, NULL, &response)));

  _az_http_client_set_callback(NULL);
}


void test_storage_blobs_multiblock_blob_upload_transport_failed(void** state);
void test_storage_blobs_multiblock_blob_upload_transport_failed(void** state)
{
    (void)state;
    PREPARE_DEFAULT_CLIENT_AND_RESPONSE()
    _az_http_client_set_callback(transport_failed);

    assert_true(az_result_succeeded(az_storage_blobs_multiblock_blob_upload(
            &client, NULL, get_data_callback, NULL, &response)));

    _az_http_client_set_callback(NULL);
}

void test_storage_blobs_multiblock_blob_upload_failed_callback(void** state);
void test_storage_blobs_multiblock_blob_upload_failed_callback(void** state)
{
  (void)state;
  PREPARE_DEFAULT_CLIENT_AND_RESPONSE()
  _az_http_client_set_callback(no_op_transport);

  assert_true(az_result_failed(az_storage_blobs_multiblock_blob_upload(
      &client, NULL, get_data_callback_failed, NULL, &response)));

  _az_http_client_set_callback(NULL);
}

void test_storage_blobs_multiblock_blob_upload_failed_callback_on_second_execution(void** state);
void test_storage_blobs_multiblock_blob_upload_failed_callback_on_second_execution(void** state)
{
    (void)state;
    PREPARE_DEFAULT_CLIENT_AND_RESPONSE()
    _az_http_client_set_callback(no_op_transport);

    assert_true(az_result_failed(az_storage_blobs_multiblock_blob_upload(
            &client, NULL, get_data_callback_failed_second, NULL, &response)));

    _az_http_client_set_callback(NULL);
}

void test_storage_blobs_multiblock_blob_upload_block_id_too_high(void** state);
void test_storage_blobs_multiblock_blob_upload_block_id_too_high(void** state)
{
  (void)state;
  PREPARE_DEFAULT_CLIENT_AND_RESPONSE()
  _az_http_client_set_callback(no_op_transport);

  assert_true(az_result_failed(az_storage_blobs_multiblock_blob_upload(
      &client, NULL, get_data_callback_block_id_too_high, NULL, &response)));

  _az_http_client_set_callback(NULL);
}

#define EXAMPLE_URL_WITH_SAS   "https://storageacct.blob.core.microsoft.com/container/" \
                               "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr=https&s" \
                               "v=2020-08-04&sr=b&sig=PLACEHOLDER%3D"

#define MULTIBLOCK_BLOB_SUFFIX "&comp=block&blockid=ABCD"

void test_storage_blobs_multiblock_blob_upload_url_too_long(void** state);
void test_storage_blobs_multiblock_blob_upload_url_too_long(void** state)
{
    (void)state;
    block_count = 0;
    az_storage_blobs_blob_client client = { 0 };

    uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE];
    memset(url_buffer, 0, sizeof(url_buffer));
    strcat((char*)url_buffer, EXAMPLE_URL_WITH_SAS);

    memset(url_buffer + strlen(EXAMPLE_URL_WITH_SAS), 'x', 
           sizeof(url_buffer) - strlen(EXAMPLE_URL_WITH_SAS) - strlen(MULTIBLOCK_BLOB_SUFFIX));
    
    assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
            &client,
            az_span_create(url_buffer, sizeof(url_buffer)-1),
            AZ_CREDENTIAL_ANONYMOUS,
            NULL)));

    uint8_t response_buffer[1024 * 4] = { 0 };
    az_http_response response = { 0 };
    assert_true(
            az_result_succeeded(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buffer))));
    _az_http_client_set_callback(verify_storage_blobs_upload);

    assert_true(az_result_failed(az_storage_blobs_multiblock_blob_upload(
            &client, NULL, get_data_callback, NULL, &response)));

    _az_http_client_set_callback(NULL);
}

void verify_storage_blobs_multiblock_upload_empty_host(void** state);
void verify_storage_blobs_multiblock_upload_empty_host(void** state)
{
    (void)state;

    az_storage_blobs_blob_client client = { 0 };
    assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
            &client, AZ_SPAN_FROM_STR("x:///"), AZ_CREDENTIAL_ANONYMOUS, NULL)));

    uint8_t response_buffer[1024 * 4] = { 0 };
    az_http_response response = { 0 };
    assert_true(
            az_result_succeeded(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buffer))));

    _az_http_client_set_callback(no_op_transport);

    assert_true(az_result_succeeded(az_storage_blobs_multiblock_blob_upload(
            &client, NULL, get_data_callback, NULL, &response)));

    _az_http_client_set_callback(NULL);


}

#ifndef AZ_NO_PRECONDITION_CHECKING

ENABLE_PRECONDITION_CHECK_TESTS()

void verify_storage_blobs_multiblock_blob_upload_null_client(void** state);
void verify_storage_blobs_multiblock_blob_upload_null_client(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  assert_true(
      az_result_succeeded(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))));

  ASSERT_PRECONDITION_CHECKED(az_storage_blobs_multiblock_blob_upload(
            NULL, NULL, get_data_callback, NULL, &http_response));

  _az_http_client_set_callback(NULL);

}

void verify_storage_blobs_multiblock_blob_upload_null_response(void** state);
void verify_storage_blobs_multiblock_blob_upload_null_response(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:///"), AZ_CREDENTIAL_ANONYMOUS, NULL)));

  _az_http_client_set_callback(no_op_transport);

  ASSERT_PRECONDITION_CHECKED(az_storage_blobs_multiblock_blob_upload(
            &client, NULL, get_data_callback, NULL, NULL));

  _az_http_client_set_callback(NULL);
}

void verify_storage_blobs_multiblock_blob_upload_null_get_data_callback(void** state);
void verify_storage_blobs_multiblock_blob_upload_null_get_data_callback(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:///"), AZ_CREDENTIAL_ANONYMOUS, NULL)));

  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response http_response = { 0 };
  assert_true(
      az_result_succeeded(az_http_response_init(&http_response, AZ_SPAN_FROM_BUFFER(response_buffer))));

  _az_http_client_set_callback(no_op_transport);

  ASSERT_PRECONDITION_CHECKED(az_storage_blobs_multiblock_blob_upload
            (&client, NULL, NULL, NULL, &http_response));

  _az_http_client_set_callback(NULL);
}

#endif // AZ_NO_PRECONDITION_CHECKING