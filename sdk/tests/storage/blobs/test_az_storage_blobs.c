// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include <azure/storage/az_storage_blobs.h>

#include <azure/core/az_http_transport.h>

#include "_az_test_http_client.h"

#include <azure/core/_az_cfg.h>

void test_storage_blobs_init(void** state);
void test_storage_blobs_init(void** state)
{
  (void)state;
  az_storage_blobs_blob_client client = { 0 };
  az_storage_blobs_blob_client_options client_options
      = az_storage_blobs_blob_client_options_default();

  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client,
      AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                       "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                       "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D"),
      AZ_CREDENTIAL_ANONYMOUS,
      &client_options)));
}

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

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Length")))
      {
        assert_false(content_length_header_found);
        content_length_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("11")));
      }

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Type")))
      {
        assert_false(content_type_header_found);
        content_type_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("text/plain")));
      }

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Content-Type")))
      {
        assert_false(content_type_header_found);
        content_type_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("text/plain")));
      }

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("Host")))
      {
        assert_false(host_header_found);
        host_header_found = true;

        assert_true(az_span_is_content_equal(
            header_value, AZ_SPAN_FROM_STR("storageacct.blob.core.microsoft.com")));
      }

      if (az_span_is_content_equal(header_name, AZ_SPAN_FROM_STR("x-ms-version")))
      {
        assert_false(api_version_header_found);
        api_version_header_found = true;

        assert_true(az_span_is_content_equal(header_value, AZ_SPAN_FROM_STR("2019-02-02")));
      }
    }

    assert_true(blob_type_header_found);
    assert_true(content_length_header_found);
    assert_true(content_type_header_found);
    assert_true(host_header_found);
    assert_true(api_version_header_found);
  }

  return AZ_OK;
}

void test_storage_blobs_upload(void** state);
void test_storage_blobs_upload(void** state)
{
  (void)state;
  az_storage_blobs_blob_client client = { 0 };
  az_storage_blobs_blob_client_options client_options
      = az_storage_blobs_blob_client_options_default();

  assert_true(az_result_succeeded(az_storage_blobs_blob_client_init(
      &client,
      AZ_SPAN_FROM_STR("https://storageacct.blob.core.microsoft.com/container/"
                       "blob.txt?sp=racwdyt&st=2021-10-07T19:03:00Z&se=2021-10-08T03:03:00Z&spr="
                       "https&sv=2020-08-04&sr=b&sig=PLACEHOLDER%3D"),
      AZ_CREDENTIAL_ANONYMOUS,
      &client_options)));

  az_storage_blobs_blob_upload_options upload_options
      = az_storage_blobs_blob_upload_options_default();

  az_http_response response = { 0 };

  _az_http_client_set_callback(verify_storage_blobs_upload);

  assert_true(az_result_succeeded(az_storage_blobs_blob_upload(
      &client, AZ_SPAN_FROM_STR("BlobContent"), &upload_options, &response)));

  _az_http_client_set_callback(NULL);
}
