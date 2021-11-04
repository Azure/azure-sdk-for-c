// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "_az_test_http_client.h"

static _az_http_client_send_request_fn volatile _az_http_client_callback = NULL;

void _az_http_client_set_callback(_az_http_client_send_request_fn http_client_callback)
{
  _az_http_client_callback = http_client_callback;
}

AZ_NODISCARD az_result
az_http_client_send_request(az_http_request const* request, az_http_response* ref_response)
{
  return (_az_http_client_callback == NULL) ? AZ_ERROR_DEPENDENCY_NOT_PROVIDED
                                            : _az_http_client_callback(request, ref_response);
}
