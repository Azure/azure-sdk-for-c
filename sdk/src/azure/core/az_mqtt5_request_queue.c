// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdlib.h>
#include <azure/core/az_mqtt5_pub_queue.h>
#include <azure/core/az_platform.h>

#include <azure/core/_az_cfg.h>

bool _az_mqtt5_request_mid_equal(az_span key, void* value, void* user_data);

az_platform_hash_table* az_mqtt5_init_request_hash_table()
{
  return az_platform_hash_table_create(MAX_PENDING_REQUESTS);
}

az_result az_mqtt5_add_pending_request(az_mqtt5_request* out_request,
      az_mqtt5_connection* connection,
      az_platform_hash_table* hash_table,
      _az_event_policy_collection* request_policy_collection,
      az_span correlation_id,
      int32_t publish_timeout_s,
      int32_t timeout_s,
      void* request)
{
  // copy data

  az_result ret = AZ_OK;

  out_request = malloc(sizeof(az_mqtt5_request));
  ret = az_mqtt5_request_init(out_request, connection, request_policy_collection, correlation_id, publish_timeout_s, timeout_s, request);

  ret = az_platform_hash_table_add(hash_table, correlation_id, out_request, sizeof(az_mqtt5_request*));

  return ret;
}

bool _az_mqtt5_request_mid_equal(az_span key, void* value, void* user_data)
{
  (void)key;
  az_mqtt5_request* request = (az_mqtt5_request*)value;
  int32_t* mid = (int32_t*)user_data;
  return request != NULL && mid != NULL && request->_internal.pending_pub_id == *mid;
}

az_mqtt5_request* az_mqtt5_get_request_by_mid(az_platform_hash_table* hash_table, int32_t mid)
{
  return (az_mqtt5_request*)az_platform_hash_table_find(hash_table, _az_mqtt5_request_mid_equal, (void*)&mid);
}
// NULL if not found
az_mqtt5_request* az_mqtt5_get_request_by_correlation_id(az_platform_hash_table* hash_table, az_span correlation_id)
{
  return (az_mqtt5_request*)az_platform_hash_table_get_by_key(hash_table, correlation_id);
}
az_result az_mqtt5_remove_request(az_platform_hash_table* hash_table, az_mqtt5_request* request)
{
  az_result ret =  az_platform_hash_table_remove(hash_table, request->_internal.correlation_id);
  free(request);
  return ret;
  // TODO: free data
}

az_mqtt5_request* az_mqtt5_get_first_expired_request(az_platform_hash_table* hash_table)
{
  (void)hash_table;
  // TODO: Implement
  return NULL;
}