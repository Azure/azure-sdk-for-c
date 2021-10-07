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
  az_storage_blobs_blob_client_options options = (az_storage_blobs_blob_client_options) {
    ._internal = {
      .api_version = { 
        ._internal = { 
          .option_location = _az_http_policy_apiversion_option_location_header,
          .name = AZ_SPAN_FROM_STR("x-ms-version"),
          .version = AZ_SPAN_LITERAL_FROM_STR("2019-02-02"),
        },
      },
      .telemetry_options = _az_http_policy_telemetry_options_default(),
    },
    .retry_options = _az_http_policy_retry_options_default(),
  };

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

  if (url_length < (sizeof("s://h") - 1))
  {
    int32_t const colon_max = url_length - (sizeof("//h") - 1);
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
    az_span endpoint,
    void* credential,
    az_storage_blobs_blob_client_options const* options)
{
  _az_PRECONDITION_NOT_NULL(out_client);
  _az_PRECONDITION_NOT_NULL(options);

  _az_credential* const cred = (_az_credential*)credential;

  *out_client = (az_storage_blobs_blob_client) {
    ._internal = {
      .endpoint = AZ_SPAN_FROM_BUFFER(out_client->_internal.endpoint_buffer),
      .host = AZ_SPAN_EMPTY,
      .options = *options,
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
      }
    }
  };

  // Copy url to client buffer so customer can re-use buffer on his/her side
  int32_t const uri_size = az_span_size(endpoint);
  _az_RETURN_IF_NOT_ENOUGH_SIZE(out_client->_internal.endpoint, uri_size);
  az_span_copy(out_client->_internal.endpoint, endpoint);
  out_client->_internal.endpoint = az_span_slice(out_client->_internal.endpoint, 0, uri_size);

  out_client->_internal.host = _az_get_host_from_url(out_client->_internal.endpoint);

  _az_RETURN_IF_FAILED(
      _az_credential_set_scopes(cred, AZ_SPAN_FROM_STR("https://storage.azure.com/.default")));

  return AZ_OK;
}

AZ_NODISCARD az_result az_storage_blobs_blob_upload(
    az_storage_blobs_blob_client* ref_client,
    az_span content, /* Buffer of content*/
    az_storage_blobs_blob_upload_options const* options,
    az_http_response* ref_response)
{

  az_storage_blobs_blob_upload_options opt;
  if (options == NULL)
  {
    opt = az_storage_blobs_blob_upload_options_default();
  }
  else
  {
    opt = *options;
  }

  // Request buffer
  // create request buffer TODO: define size for a blob upload
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE];
  az_span request_url_span = AZ_SPAN_FROM_BUFFER(url_buffer);
  // copy url from client
  int32_t uri_size = az_span_size(ref_client->_internal.endpoint);
  _az_RETURN_IF_NOT_ENOUGH_SIZE(request_url_span, uri_size);
  az_span_copy(request_url_span, ref_client->_internal.endpoint);

  uint8_t headers_buffer[_az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE];
  az_span request_headers_span = AZ_SPAN_FROM_BUFFER(headers_buffer);

  // create request
  az_http_request request;
  _az_RETURN_IF_FAILED(az_http_request_init(
      &request,
      opt.context,
      az_http_method_put(),
      request_url_span,
      uri_size,
      request_headers_span,
      content));

  // add blob type to request
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("x-ms-blob-type"), AZ_SPAN_FROM_STR("BlockBlob")));

  uint8_t content_length[_az_INT64_AS_STR_BUFFER_SIZE] = { 0 };
  az_span content_length_span = AZ_SPAN_FROM_BUFFER(content_length);
  az_span remainder;
  _az_RETURN_IF_FAILED(az_span_i64toa(content_length_span, az_span_size(content), &remainder));
  content_length_span
      = az_span_slice(content_length_span, 0, _az_span_diff(remainder, content_length_span));

  // add Content-Length to request
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("Content-Length"), content_length_span));

  // add blob type to request
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("Content-Type"), AZ_SPAN_FROM_STR("text/plain")));

  // add host header
  if (az_span_size(ref_client->_internal.host) > 0)
  {
    _az_RETURN_IF_FAILED(az_http_request_append_header(
        &request, AZ_SPAN_FROM_STR("Host"), ref_client->_internal.host));
  }

  // start pipeline
  return az_http_pipeline_process(&ref_client->_internal.pipeline, &request, ref_response);
}
