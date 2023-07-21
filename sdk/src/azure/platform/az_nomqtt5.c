// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_mqtt5_init(az_mqtt5* mqtt5, az_mqtt5_options const* options)
{
  (void)mqtt5;
  (void)options;
  _az_PRECONDITION_NOT_NULL(mqtt5);
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result
az_mqtt5_outbound_connect(az_mqtt5* mqtt5, az_mqtt5_connect_data* connect_data)
{
  (void)mqtt5;
  (void)connect_data;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data)
{
  (void)mqtt5;
  (void)sub_data;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data)
{
  (void)mqtt5;
  (void)pub_data;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5)
{
  (void)mqtt5;
  return AZ_ERROR_NOT_IMPLEMENTED;
}
