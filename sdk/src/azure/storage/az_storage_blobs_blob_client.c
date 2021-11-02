// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>
#include <azure/core/az_json.h>
#include <azure/core/az_precondition.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_credentials_internal.h>
#include <azure/core/internal/az_http_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/storage/az_storage_blobs.h>

#include <stddef.h>

#include <azure/core/_az_cfg.h>

enum
{
  _az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE = 10 * sizeof(_az_http_request_header),
};

AZ_NODISCARD az_storage_blobs_blob_client_options az_storage_blobs_blob_client_options_default()
{
  az_storage_blobs_blob_client_options options = (az_storage_blobs_blob_client_options){ 0 };
  options = (az_storage_blobs_blob_client_options) {
    ._internal = {
      .telemetry_options = _az_http_policy_telemetry_options_default(),
      .api_version = _az_http_policy_apiversion_options_default(),
    },
    .retry_options = _az_http_policy_retry_options_default(),
  };

  options._internal.telemetry_options.component_name = AZ_SPAN_FROM_STR("storage-blobs");
  options._internal.api_version._internal.name = AZ_SPAN_FROM_STR("x-ms-version");
  options._internal.api_version._internal.version = AZ_SPAN_FROM_STR("2019-02-02");
  options._internal.api_version._internal.option_location
      = _az_http_policy_apiversion_option_location_header;

  // NOLINTNEXTLINE(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
  options.retry_options.max_retries = 5;

  options.retry_options.retry_delay_msec = 1 * _az_TIME_MILLISECONDS_PER_SECOND;

  // NOLINTNEXTLINE(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
  options.retry_options.max_retry_delay_msec = 30 * _az_TIME_MILLISECONDS_PER_SECOND;

  return options;
}

AZ_INLINE az_span _az_get_host_from_url(az_span url)
{
  int32_t const url_length = az_span_size(url);
  uint8_t const* url_ptr = az_span_ptr(url);

  if (url_length >= (int32_t)(sizeof("s://h") - 1))
  {
    int32_t const colon_max = url_length - (int32_t)(sizeof("//h") - 1);
    for (int32_t colon_pos = 0; colon_pos < colon_max; ++colon_pos)
    {
      if (url_ptr[colon_pos] == ':')
      {
        if (url_ptr[colon_pos + 1] == '/' && url_ptr[colon_pos + 2] == '/')
        {
          int32_t const authority_pos = colon_pos + 3;
          int32_t authority_end_pos = authority_pos;
          for (; authority_end_pos < url_length; ++authority_end_pos)
          {
            if (url_ptr[authority_end_pos] == '/')
            {
              break;
            }
          }

          if (authority_end_pos > authority_pos)
          {
            int32_t host_start_pos = authority_pos;
            for (; host_start_pos < authority_end_pos; ++host_start_pos)
            {
              if (url_ptr[host_start_pos] == '@')
              {
                break;
              }
            }

            if (host_start_pos == authority_end_pos)
            {
              host_start_pos = authority_pos;
            }
            else
            {
              ++host_start_pos;
            }

            if (host_start_pos < authority_end_pos)
            {
              int32_t host_end_pos = host_start_pos;
              for (; host_end_pos < authority_end_pos; ++host_end_pos)
              {
                if (url_ptr[host_end_pos] == ':')
                {
                  break;
                }
              }

              return az_span_slice(url, host_start_pos, host_end_pos);
            }
          }
        }

        break;
      }
    }
  }

  return AZ_SPAN_EMPTY;
}

AZ_NODISCARD az_result az_storage_blobs_blob_client_init(
    az_storage_blobs_blob_client* out_client,
    az_span blob_url,
    void* credential,
    az_storage_blobs_blob_client_options const* options)
{
  _az_PRECONDITION_NOT_NULL(out_client);
  _az_PRECONDITION_VALID_SPAN(blob_url, sizeof("s://h") - 1, false);

  _az_credential* const cred = (_az_credential*)credential;

  *out_client = (az_storage_blobs_blob_client){ 0 };
  *out_client = (az_storage_blobs_blob_client) {
    ._internal = {
      .credential = cred,
      .pipeline = (_az_http_pipeline){
        ._internal = {
          .policies = {
            {
              ._internal = {
                .process = az_http_pipeline_policy_apiversion,
                .options= &out_client->_internal.options._internal.api_version,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_telemetry,
                .options = &out_client->_internal.options._internal.telemetry_options,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_retry,
                .options = &out_client->_internal.options.retry_options,
              },
            },
            {
              ._internal = {
                .process = az_http_pipeline_policy_credential,
                .options = cred,
              },
            },
#ifndef AZ_NO_LOGGING
            {
              ._internal = {
                .process = az_http_pipeline_policy_logging,
                .options = NULL,
              },
            },
#endif // AZ_NO_LOGGING
            {
              ._internal = {
                .process = az_http_pipeline_policy_transport,
                .options = NULL,
              },
            },
          },
        }
      },
      .blob_url = AZ_SPAN_FROM_BUFFER(out_client->_internal.blob_url_buffer),
      .host = AZ_SPAN_EMPTY,
      .options = (options != NULL) ? *options : az_storage_blobs_blob_client_options_default(),
    }
  };

  // Copy url to client buffer so customer can re-use buffer on his/her side
  int32_t const blob_url_size = az_span_size(blob_url);
  _az_RETURN_IF_NOT_ENOUGH_SIZE(out_client->_internal.blob_url, blob_url_size);
  az_span_copy(out_client->_internal.blob_url, blob_url);
  out_client->_internal.blob_url = az_span_slice(out_client->_internal.blob_url, 0, blob_url_size);

  out_client->_internal.host = _az_get_host_from_url(out_client->_internal.blob_url);

  _az_RETURN_IF_FAILED(
      _az_credential_set_scopes(cred, AZ_SPAN_FROM_STR("https://storage.azure.com/.default")));

  return AZ_OK;
}

static AZ_NODISCARD az_result _az_init_blob_client_http_request(
    az_http_request* out_request,
    az_context* context,
    az_storage_blobs_blob_client const* client,
    az_span request_url_span,
    az_span request_headers_span,
    az_http_method http_method,
    az_span body)
{
  // URL buffer
  az_span const blob_url = client->_internal.blob_url;
  int32_t const url_size = az_span_size(blob_url);
  _az_RETURN_IF_NOT_ENOUGH_SIZE(request_url_span, url_size);
  az_span_copy(request_url_span, blob_url);

  // Request
  _az_RETURN_IF_FAILED(az_http_request_init(
      out_request,
      (context != NULL) ? context : &az_context_application,
      http_method,
      request_url_span,
      url_size,
      request_headers_span,
      body));

  // Host header
  az_span const host_val = client->_internal.host;
  if (az_span_size(host_val) > 0)
  {
    _az_RETURN_IF_FAILED(
        az_http_request_append_header(out_request, AZ_SPAN_FROM_STR("Host"), host_val));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_span content,
    az_storage_blobs_blob_upload_options const* options,
    az_http_response* ref_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_response);

  (void)options;

  // HTTP request buffers
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE] = { 0 };
  uint8_t headers_buffer[_az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE] = { 0 };

  // Initialize the HTTP request
  az_http_request request = { 0 };
  _az_RETURN_IF_FAILED(_az_init_blob_client_http_request(
      &request,
      context,
      client,
      AZ_SPAN_FROM_BUFFER(url_buffer),
      AZ_SPAN_FROM_BUFFER(headers_buffer),
      az_http_method_put(),
      content));

  // Blob Type header
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("x-ms-blob-type"), AZ_SPAN_FROM_STR("BlockBlob")));

  // Content Length header
  uint8_t content_length[_az_INT64_AS_STR_BUFFER_SIZE] = { 0 };
  {
    // Form the value
    az_span content_length_span = AZ_SPAN_FROM_BUFFER(content_length);
    az_span remainder = { 0 };
    _az_RETURN_IF_FAILED(az_span_i64toa(content_length_span, az_span_size(content), &remainder));
    content_length_span
        = az_span_slice(content_length_span, 0, _az_span_diff(remainder, content_length_span));

    // Append the header
    _az_RETURN_IF_FAILED(az_http_request_append_header(
        &request, AZ_SPAN_FROM_STR("Content-Length"), content_length_span));
  }

  // Content Type header
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("Content-Type"), AZ_SPAN_FROM_STR("text/plain")));

  // Run the pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &request, ref_response);
}

AZ_NODISCARD az_result az_storage_blobs_blob_download(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_storage_blobs_blob_download_options const* options,
    az_http_response* ref_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_response);

  (void)options;

  // HTTP request buffers
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE] = { 0 };
  uint8_t headers_buffer[_az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE] = { 0 };

  // Initialize the HTTP request
  az_http_request request = { 0 };
  _az_RETURN_IF_FAILED(_az_init_blob_client_http_request(
      &request,
      context,
      client,
      AZ_SPAN_FROM_BUFFER(url_buffer),
      AZ_SPAN_FROM_BUFFER(headers_buffer),
      az_http_method_get(),
      AZ_SPAN_EMPTY));

  // Run the pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &request, ref_response);
}
