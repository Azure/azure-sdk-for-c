// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt5_pub_queue. You use this to manage pending publishes.
 *
 * @note The state diagram is in 
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_PUB_QUEUE_H
#define _az_MQTT5_PUB_QUEUE_H

#include <stdio.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_mqtt5_request.h>

#include <azure/core/_az_cfg_prefix.h>

// Application can define this value to control how much memory is used to track pending requests
// (and how many can be in flight at once)
#ifndef MAX_PENDING_REQUESTS
#define MAX_PENDING_REQUESTS 5
#endif

// request hash table api
az_platform_hash_table* az_mqtt5_init_request_hash_table();

az_result az_mqtt5_add_pending_request(az_mqtt5_request* out_request,
    az_mqtt5_connection* connection,
    az_platform_hash_table* hash_table,
    _az_event_policy_collection* request_policy_collection,
    az_span correlation_id,
    int32_t publish_timeout_s,
    int32_t timeout_s,
    void* request);
az_result az_mqtt5_remove_request(az_platform_hash_table* hash_table, az_mqtt5_request* request);
// az_mqtt5_request* az_mqtt5_get_first_expired_request(az_platform_hash_table* hash_table);
// az_result az_mqtt5_remove_expired_requests(void* hash_table);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_PUB_QUEUE_H
