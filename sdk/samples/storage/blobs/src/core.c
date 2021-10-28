// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

void http_send_request();

void core_transport_policy() { http_send_request(); }

void core_span_create() {}
