// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

static az_result verify_storage_appendblob_create(
    az_http_request const* request,
    az_http_response* ref_response)
{
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
    assert_true(az_span_is_content_equal(
        request_url,
        AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                         "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                         "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D")));
  }

  {
    az_span request_body = { 0 };
    assert_true(az_result_succeeded(az_http_request_get_body(request, &request_body)));
    assert_true(az_span_is_content_equal(request_body, AZ_SPAN_EMPTY));
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

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("AppendBlob")));
      }
      else if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Length")))
      {
        assert_false(content_length_header_found);
        content_length_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("0")));
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

    assert_true(blob_type_header_found);
    assert_true(content_length_header_found);
    assert_true(content_type_header_found);
    assert_true(host_header_found);
    assert_true(api_version_header_found);
    assert_true(user_agent_header_found);
  }

  assert_true(az_result_succeeded(az_http_response_init(
      ref_response,
      AZ_SPAN_FROM_STR("HTTP/1.1 201 Created\r\n"
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
                       "\r\n"))));

  return AZ_OK;
}

void test_storage_blobs_appendblob_create(void** state);
void test_storage_blobs_appendblob_create(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client,
      AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                       "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                       "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D"),
      AZ_CREDENTIAL_ANONYMOUS,
      NULL)));

  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response response = { 0 };
  assert_true(
      az_result_succeeded(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buffer))));

  _az_http_client_set_callback(verify_storage_appendblob_create);

  assert_true(az_result_succeeded(az_storage_blobs_appendblob_create(
      &client, NULL, NULL, &response)));

  _az_http_client_set_callback(NULL);
}

static az_result verify_storage_appendblob_append_block(
    az_http_request const* request,
    az_http_response* ref_response)
{
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
    assert_true(az_span_is_content_equal(
        request_url,
        AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                         "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                         "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D&comp=appendblock")));
  }

  {
    az_span request_body = { 0 };
    assert_true(az_result_succeeded(az_http_request_get_body(request, &request_body)));
    assert_true(az_span_is_content_equal(request_body, AZ_SPAN_FROM_STR("BlockContent")));
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

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("AppendBlob")));
      }
      else if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Length")))
      {
        assert_false(content_length_header_found);
        content_length_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("12")));
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

    assert_true(blob_type_header_found);
    assert_true(content_length_header_found);
    assert_true(content_type_header_found);
    assert_true(host_header_found);
    assert_true(api_version_header_found);
    assert_true(user_agent_header_found);
  }

  assert_true(az_result_succeeded(az_http_response_init(
      ref_response,
      AZ_SPAN_FROM_STR("HTTP/1.1 201 Created\r\n"
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
                       "\r\n"))));

  return AZ_OK;
}

void test_storage_blobs_appendblob_append_block(void** state);
void test_storage_blobs_appendblob_append_block(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client,
      AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                       "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                       "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D"),
      AZ_CREDENTIAL_ANONYMOUS,
      NULL)));

  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response response = { 0 };
  assert_true(
      az_result_succeeded(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buffer))));

  _az_http_client_set_callback(verify_storage_appendblob_append_block);

  assert_true(az_result_succeeded(az_storage_blobs_appendblob_append_block(
      &client, NULL, AZ_SPAN_FROM_STR("BlockContent"), NULL, &response)));

  _az_http_client_set_callback(NULL);
}

static az_result no_op_transport(az_http_request const* request, az_http_response* ref_response)
{
  (void)request;

  return az_http_response_init(
      ref_response,
      AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n"
                       "Content-Length: 0\r\n"
                       "\r\n"));
}

void test_storage_blobs_appendblob_append_block_url_too_long(void** state);
void test_storage_blobs_appendblob_append_block_url_too_long(void** state)
{
  (void)state;

  char url_buf[AZ_HTTP_REQUEST_URL_BUFFER_SIZE] = { 0 };
  size_t bytes_to_write = AZ_HTTP_REQUEST_URL_BUFFER_SIZE - strlen("&comp=appendblock");
  memset(url_buf, 'x', bytes_to_write);

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, az_span_create_from_str(url_buf), AZ_CREDENTIAL_ANONYMOUS, NULL)));

  uint8_t response_buffer[1024 * 4] = { 0 };
  az_http_response response = { 0 };
  assert_true(
      az_result_succeeded(az_http_response_init(&response, AZ_SPAN_FROM_BUFFER(response_buffer))));

  _az_http_client_set_callback(no_op_transport);

  assert_true(AZ_ERROR_NOT_ENOUGH_SPACE == az_storage_blobs_appendblob_append_block(
                  &client, NULL, AZ_SPAN_FROM_STR("BlockContent"), NULL, &response));

  _az_http_client_set_callback(NULL);
}

void verify_storage_blobs_appendblob_create_empty_host(void** state);
void verify_storage_blobs_appendblob_create_empty_host(void** state)
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

  assert_true(az_result_succeeded(az_storage_blobs_appendblob_create(
      &client, NULL, NULL, &response)));

  _az_http_client_set_callback(NULL);
}

void verify_storage_blobs_appendblob_append_block_empty_host(void** state);
void verify_storage_blobs_appendblob_append_block_empty_host(void** state)
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

  assert_true(az_result_succeeded(az_storage_blobs_appendblob_append_block(
      &client, NULL, AZ_SPAN_FROM_STR("BlockContent"), NULL, &response)));

  _az_http_client_set_callback(NULL);
}

#ifndef AZ_NO_PRECONDITION_CHECKING

ENABLE_PRECONDITION_CHECK_TESTS()

void verify_storage_blobs_appendblob_create_null_client(void** state);
void verify_storage_blobs_appendblob_create_null_client(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  _az_http_client_set_callback(no_op_transport);

  ASSERT_PRECONDITION_CHECKED(az_storage_blobs_appendblob_create(NULL, NULL, NULL, NULL));

  _az_http_client_set_callback(NULL);

}

void verify_storage_blobs_appendblob_create_null_response(void** state);
void verify_storage_blobs_appendblob_create_null_response(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:///"), AZ_CREDENTIAL_ANONYMOUS, NULL)));

  _az_http_client_set_callback(no_op_transport);

  ASSERT_PRECONDITION_CHECKED(
      az_storage_blobs_appendblob_create(&client, NULL, NULL, NULL));

  _az_http_client_set_callback(NULL);
}

void verify_storage_blobs_appendblob_append_block_null_client(void** state);
void verify_storage_blobs_appendblob_append_block_null_client(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  _az_http_client_set_callback(no_op_transport);

  ASSERT_PRECONDITION_CHECKED(az_storage_blobs_appendblob_append_block(
      NULL, NULL, AZ_SPAN_FROM_STR("BlockContent"), NULL, NULL));

  _az_http_client_set_callback(NULL);

}

void verify_storage_blobs_appendblob_append_block_null_response(void** state);
void verify_storage_blobs_appendblob_append_block_null_response(void** state)
{
  (void)state;
  SETUP_PRECONDITION_CHECK_TESTS();

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:///"), AZ_CREDENTIAL_ANONYMOUS, NULL)));

  _az_http_client_set_callback(no_op_transport);

  ASSERT_PRECONDITION_CHECKED(az_storage_blobs_appendblob_append_block(
      &client, NULL, AZ_SPAN_FROM_STR("BlockContent"), NULL, NULL));

  _az_http_client_set_callback(NULL);

}

#endif // AZ_NO_PRECONDITION_CHECKING
