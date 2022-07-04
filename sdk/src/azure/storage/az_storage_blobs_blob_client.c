// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_http.h>
#include <azure/core/az_http_transport.h>
#include <azure/core/az_json.h>
#include <azure/core/az_base64.h>
#include <azure/core/az_precondition.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_credentials_internal.h>
#include <azure/core/internal/az_http_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/storage/az_storage_blobs.h>

#include <stddef.h>
#include <stdio.h>

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
      .telemetry_options = _az_http_policy_telemetry_options_create(AZ_SPAN_FROM_STR("storage-blobs")),
      .api_version = _az_http_policy_apiversion_options_default(),
    },
    .retry_options = _az_http_policy_retry_options_default(),
  };

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

static AZ_NODISCARD az_result _az_init_multiblock_blob_client_http_request(
    az_http_request* out_request,
    az_context* context,
    az_storage_blobs_blob_client const* client,
    az_span request_url_span,
    az_span request_headers_span,
    az_http_method http_method,
    az_span body,
    uint32_t block_id,
    char *block_id_list,
    size_t block_id_list_size,
    uint32_t *block_id_list_cursor)
{
  // URL buffer
  int32_t url_strlen = 0;
  char suffix[32] = "&comp=block&blockid=";
  char block_id_str[8] = {0};
  char block_id_with_spaces[8] = "   ";
  int32_t block_id_base64_len = 0;

  /* Maximum number of blocks is limited to 999 to reduce the size of block ID list
     as the base64 encoded block_id will always have only four letters */
  if (block_id > 999)
  {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  /* write block_id to format left aligned to 4 digits with spaces */
  sprintf(block_id_str, "%u", block_id);
  size_t block_id_len =  strlen(block_id_str);
  memcpy(block_id_with_spaces + 3 - block_id_len, block_id_str, block_id_len);

  /* write left aligned block_id to base64 format */
  _az_RETURN_IF_FAILED(az_base64_encode(
    az_span_create((uint8_t*)block_id_str, sizeof(block_id_str)),
    az_span_create_from_str(block_id_with_spaces), 
    &block_id_base64_len));
  strcat(suffix, block_id_str);

  /* append suffix to the request URL span */
  az_span_fill(request_url_span, 0);
  az_span_to_str((char *)az_span_ptr(request_url_span), az_span_size(request_url_span), client->_internal.blob_url);

  if (strlen((char *)az_span_ptr(request_url_span)) + strlen(suffix) < (size_t)az_span_size(request_url_span))
  {
    strcat((char *)az_span_ptr(request_url_span), suffix);
    url_strlen = (int32_t)strlen((char *)az_span_ptr(request_url_span));
    request_url_span = az_span_slice(request_url_span, 0, url_strlen);
  }
  else
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  if (*block_id_list_cursor + 21 < block_id_list_size - 13) // strlen("<Latest>ABCD</Latest>") = 21, strlen("</BlockList>\0") = 13
  {
    strcat(block_id_list, "<Latest>");
    strcat(block_id_list, block_id_str);
    strcat(block_id_list, "</Latest>");
    *block_id_list_cursor += 21;
  }
  else 
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  int32_t const url_size = az_span_size(request_url_span);

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

static AZ_NODISCARD az_result _az_init_multiblock_blob_block_list_client_http_request(
    az_http_request* out_request,
    az_context* context,
    az_storage_blobs_blob_client const* client,
    az_span request_url_span,
    az_span request_headers_span,
    az_http_method http_method,
    az_span body)
{
  // URL buffer
  int32_t url_strlen = 0;
  const char *suffix = "&comp=blocklist";
  az_span_fill(request_url_span, 0);
  az_span_to_str((char *)az_span_ptr(request_url_span), az_span_size(request_url_span), client->_internal.blob_url);

   // no need to check url length, as the block list suffix is shorter than previous requests suffixes
   strcat((char *)az_span_ptr(request_url_span), suffix);
   url_strlen = (int32_t)strlen((char *)az_span_ptr(request_url_span));
   request_url_span = az_span_slice(request_url_span, 0, url_strlen);

  int32_t const url_size = az_span_size(request_url_span);

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

static AZ_NODISCARD az_result _az_init_appendblob_client_http_request(
    az_http_request* out_request,
    az_context* context,
    az_storage_blobs_blob_client const* client,
    az_span request_url_span,
    az_span request_headers_span,
    az_http_method http_method,
    az_span body)
{
  // URL buffer
  int32_t url_strlen = 0;
  const char *suffix = "&comp=appendblock";
  az_span_fill(request_url_span, 0);
  az_span_to_str((char *)az_span_ptr(request_url_span), az_span_size(request_url_span), client->_internal.blob_url);

  if (strlen((char *)az_span_ptr(request_url_span)) + strlen(suffix) < (size_t)az_span_size(request_url_span))
  {
    strcat((char *)az_span_ptr(request_url_span), suffix);
    url_strlen = (int32_t)strlen((char *)az_span_ptr(request_url_span));
    request_url_span = az_span_slice(request_url_span, 0, url_strlen);
  }
  else
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  int32_t const url_size = az_span_size(request_url_span);

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

AZ_NODISCARD az_result az_storage_blobs_multiblock_blob_upload(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_result (*get_data_callback)(az_span *data_block),
    az_storage_blobs_blob_upload_options const* options,
    az_http_response* ref_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_response);
  _az_PRECONDITION_NOT_NULL(get_data_callback);

  (void)options;

  // HTTP request buffers
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE] = { 0 };
  uint8_t headers_buffer[_az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE] = { 0 };
  az_http_request request = { 0 };
  az_result result;

  uint32_t block_id = 0;
  char block_id_list[AZ_HTTP_REQUEST_BLOCK_ID_LIST_SIZE] = {0};
  uint32_t block_id_list_cursor = 0;
  az_span content = AZ_SPAN_EMPTY;
  _az_RETURN_IF_FAILED(get_data_callback(&content));

  const char *block_id_header = "<?xml version=\"1.0\" encoding=\"utf-8\"?><BlockList>";
  strcat(block_id_list, block_id_header);
  block_id_list_cursor += (uint32_t)strlen(block_id_header);

  while (az_span_ptr(content) != NULL && az_span_size(content) != 0)
  {
    // Initialize the HTTP request
    _az_RETURN_IF_FAILED(_az_init_multiblock_blob_client_http_request(
        &request,
        context,
        client,
        AZ_SPAN_FROM_BUFFER(url_buffer),
        AZ_SPAN_FROM_BUFFER(headers_buffer),
        az_http_method_put(),
        content,
        block_id,
        block_id_list,
        sizeof(block_id_list),
        &block_id_list_cursor));

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

    // Send the HTTP request
    az_span_fill(ref_response->_internal.http_response, 0);
    _az_RETURN_IF_FAILED(az_http_response_init(
    		ref_response, ref_response->_internal.http_response));

    // Run the pipeline
    result = az_http_pipeline_process(
        &client->_internal.pipeline, &request, ref_response);

    if (az_http_response_get_status_code(ref_response) != AZ_HTTP_STATUS_CODE_CREATED)
    {
      return result;
    }

    _az_RETURN_IF_FAILED(get_data_callback(&content));
    block_id++;
  }

  // Put Block List
  if (block_id_list_cursor + strlen("</BlockList>\0") < sizeof(block_id_list))
  {
    strcat(block_id_list, "</BlockList>");
  }
  else
  {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  // Initialize the HTTP request
  _az_RETURN_IF_FAILED(_az_init_multiblock_blob_block_list_client_http_request(
      &request,
      context,
      client,
      AZ_SPAN_FROM_BUFFER(url_buffer),
      AZ_SPAN_FROM_BUFFER(headers_buffer),
      az_http_method_put(),
      az_span_create_from_str(block_id_list)));

  // Content Length header
  uint8_t content_length[_az_INT64_AS_STR_BUFFER_SIZE] = { 0 };
  {
    // Form the value
    az_span content_length_span = AZ_SPAN_FROM_BUFFER(content_length);
    az_span remainder = { 0 };
    _az_RETURN_IF_FAILED(az_span_i64toa(content_length_span, (uint32_t)strlen(block_id_list), &remainder));
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
  result = az_http_pipeline_process(
      &client->_internal.pipeline, &request, ref_response);

  return result;
}

AZ_NODISCARD az_result az_storage_blobs_appendblob_create(
    az_storage_blobs_blob_client* client,
    az_context* context,
    az_storage_blobs_blob_upload_options const* options,
    az_http_response* ref_response)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_response);

  (void)options;

  // HTTP request buffers
  uint8_t url_buffer[AZ_HTTP_REQUEST_URL_BUFFER_SIZE] = { 0 };
  uint8_t headers_buffer[_az_STORAGE_HTTP_REQUEST_HEADER_BUFFER_SIZE] = { 0 };

  az_span content = AZ_SPAN_FROM_STR("");

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
      &request, AZ_SPAN_FROM_STR("x-ms-blob-type"), AZ_SPAN_FROM_STR("AppendBlob")));

  // Content Length header
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("Content-Length"), AZ_SPAN_FROM_STR("0")));

  // Content Type header
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("Content-Type"), AZ_SPAN_FROM_STR("text/plain")));

  // Run the pipeline
  return az_http_pipeline_process(&client->_internal.pipeline, &request, ref_response);
}

AZ_NODISCARD az_result az_storage_blobs_appendblob_append_block(
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
  _az_RETURN_IF_FAILED(_az_init_appendblob_client_http_request(
      &request,
      context,
      client,
      AZ_SPAN_FROM_BUFFER(url_buffer),
      AZ_SPAN_FROM_BUFFER(headers_buffer),
      az_http_method_put(),
      content));

  // Blob Type header
  _az_RETURN_IF_FAILED(az_http_request_append_header(
      &request, AZ_SPAN_FROM_STR("x-ms-blob-type"), AZ_SPAN_FROM_STR("AppendBlob")));

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
