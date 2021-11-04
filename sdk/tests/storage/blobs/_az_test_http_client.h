// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_http_transport.h>

typedef az_result (*_az_http_client_send_request_fn)(
    az_http_request const* request,
    az_http_response* ref_response);

void _az_http_client_set_callback(_az_http_client_send_request_fn http_client_callback);
