// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options)
{
  (void)mqtt;
  (void)options;
  _az_PRECONDITION_NOT_NULL(mqtt);
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt_outbound_connect(az_mqtt* mqtt, az_mqtt_connect_data* connect_data)
{
  (void)mqtt;
  (void)context;
  (void)connect_data;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt_outbound_sub(az_mqtt* mqtt, az_mqtt_sub_data* sub_data)
{
  (void)mqtt;
  (void)context;
  (void)sub_data;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt_outbound_pub(az_mqtt* mqtt, az_mqtt_pub_data* pub_data)
{
  (void)mqtt;
  (void)context;
  (void)pub_data;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt_outbound_disconnect(az_mqtt* mqtt)
{
  (void)mqtt;
  (void)context;
  return AZ_ERROR_NOT_IMPLEMENTED;
}
