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

#include "_az_test_http_client.h"

#include <azure/core/_az_cfg.h>

void test_storage_blobs_init(void** state);
void test_storage_blobs_init(void** state)
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
}

void test_storage_blobs_init_nonnull_options(void** state);
void test_storage_blobs_init_nonnull_options(void** state)
{
  (void)state;

  az_storage_blobs_blob_client_options options = az_storage_blobs_blob_client_options_default();

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client,
      AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                       "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                       "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D"),
      AZ_CREDENTIAL_ANONYMOUS,
      &options)));
}

#define _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID "azsdk-c-storage-blobs/" AZ_SDK_VERSION_STRING
#define _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID_LENGTH \
  (sizeof(_az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID) - 1)

static az_result verify_storage_blobs_upload(
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
    assert_true(az_span_is_content_equal(request_body, AZ_SPAN_FROM_STR("BlobContent")));
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

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("11")));
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

void test_storage_blobs_upload(void** state);
void test_storage_blobs_upload(void** state)
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

  _az_http_client_set_callback(verify_storage_blobs_upload);

  assert_true(az_result_succeeded(az_storage_blobs_blob_upload(
      &client, NULL, AZ_SPAN_FROM_STR("BlobContent"), NULL, &response)));

  _az_http_client_set_callback(NULL);
}

static az_result verify_storage_blobs_download(
    az_http_request const* request,
    az_http_response* ref_response)
{
  assert_non_null(request);
  assert_non_null(ref_response);

  {
    az_span http_method = { 0 };
    assert_true(az_result_succeeded(az_http_request_get_method(request, &http_method)));
    assert_true(az_span_is_content_equal(http_method, az_http_method_get()));
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
    assert_true(az_span_size(request_body) == 0);
  }

  {
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

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Host")))
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

    assert_true(host_header_found);
    assert_true(api_version_header_found);
    assert_true(user_agent_header_found);
  }

  assert_true(az_result_succeeded(az_http_response_init(
      ref_response,
      AZ_SPAN_FROM_STR("HTTP/1.1 200 OK\r\n"
                       "Content-Length: 11\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-MD5: ZF1nmzmalQE57vKStFOEkw==\r\n"
                       "Last-Modified: Thu, 07 Oct 2021 19:35:26 GMT\r\n"
                       "Accept-Ranges: bytes\r\n"
                       "ETag: \"0x8D98F37CBAD882E\"\r\n"
                       "Server: Windows-Azure-Blob/1.0 Microsoft-HTTPAPI/2.0\r\n"
                       "x-ms-request-id: c1602c46-101e-00d7-15b2-bb381b000000\r\n"
                       "x-ms-version: 2019-02-02\r\n"
                       "x-ms-creation-time: Thu, 07 Oct 2021 19:01:58 GMT\r\n"
                       "x-ms-lease-status: unlocked\r\n"
                       "x-ms-lease-state: available\r\n"
                       "x-ms-blob-type: BlockBlob\r\n"
                       "x-ms-request-server-encrypted: true\r\n"
                       "Date: Thu, 07 Oct 2021 19:35:26 GMT\r\n"
                       "\r\n"
                       "BlobContent"))));

  return AZ_OK;
}

#undef _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID
#undef _az_STORAGE_BLOBS_TEST_EXPECTED_TELEMETRY_ID_LENGTH

void test_storage_blobs_download(void** state);
void test_storage_blobs_download(void** state)
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

  _az_http_client_set_callback(verify_storage_blobs_download);

  assert_true(az_result_succeeded(az_storage_blobs_blob_download(&client, NULL, NULL, &response)));

  _az_http_client_set_callback(NULL);
}

void test_storage_blobs_init_url_no_colon(void** state);
void test_storage_blobs_init_url_no_colon(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("xxxxx"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_no_slash1(void** state);
void test_storage_blobs_init_url_no_slash1(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:xxx"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_no_slash2(void** state);
void test_storage_blobs_init_url_no_slash2(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:/xx"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_empty_host_slash(void** state);
void test_storage_blobs_init_url_empty_host_slash(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x:///"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_empty_host_username(void** state);
void test_storage_blobs_init_url_empty_host_username(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x://@z"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_host_username(void** state);
void test_storage_blobs_init_url_host_username(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x://y@z"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_host_empty_username_with_slash(void** state);
void test_storage_blobs_init_url_host_empty_username_with_slash(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x://y@/"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

void test_storage_blobs_init_url_host_port(void** state);
void test_storage_blobs_init_url_host_port(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };
  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client, AZ_SPAN_FROM_STR("x://y:1"), AZ_CREDENTIAL_ANONYMOUS, NULL)));
}

#define URL_START "x://"
#define URL_START_LEN (sizeof(URL_START) - 1)

void test_storage_blobs_init_url_too_long(void** state);
void test_storage_blobs_init_url_too_long(void** state)
{
  (void)state;

  az_storage_blobs_blob_client client = { 0 };

  uint8_t url_buf[sizeof(client._internal.blob_url_buffer) + 1] = URL_START;
  memset(url_buf + URL_START_LEN, 'y', sizeof(url_buf) - URL_START_LEN);

  assert_int_equal(
      AZ_ERROR_NOT_ENOUGH_SPACE,
      az_storage_blobs_blob_client_init(
          &client, AZ_SPAN_FROM_BUFFER(url_buf), AZ_CREDENTIAL_ANONYMOUS, NULL));
}

#undef URL_START
#undef URL_START_LEN

static AZ_NODISCARD az_result test_credential_fn(void* ref_credential, az_span scopes)
{
  (void)ref_credential;
  (void)scopes;

  return AZ_ERROR_UNEXPECTED_CHAR;
}

void test_storage_blobs_init_credential_error(void** state);
void test_storage_blobs_init_credential_error(void** state)
{
  (void)state;

  _az_credential cred = (_az_credential){ ._internal = { .set_scopes = test_credential_fn } };

  az_storage_blobs_blob_client client = { 0 };
  assert_int_equal(
      AZ_ERROR_UNEXPECTED_CHAR,
      az_storage_blobs_blob_client_init(&client, AZ_SPAN_FROM_STR("x://y"), &cred, NULL));
}
