/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#ifndef _MOSQUITTO_RPC_SERVER_SAMPLE_JSON_PARSER_H
#define _MOSQUITTO_RPC_SERVER_SAMPLE_JSON_PARSER_H

#include <azure/core/az_json.h>

typedef struct
{
  long request_timestamp;
  az_span requested_from;
} unlock_request;

az_result deserialize_unlock_request(az_span request_data, unlock_request* unlock_json_out);

az_result serialize_response_payload(unlock_request req, az_span out_payload);

#endif //_MOSQUITTO_RPC_SERVER_SAMPLE_JSON_PARSER_H
