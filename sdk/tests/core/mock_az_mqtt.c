// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Contains the az_mqtt.h mock implementation.
 *
 */

#ifdef _az_MOCK_ENABLED

#include <azure/core/az_mqtt.h>
#include <azure/core/az_mqtt_config.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <mosquitto.h>
#include <pthread.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result __wrap_az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options);
AZ_NODISCARD az_result __wrap_az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options)
{
  (void)mqtt;
  (void)options;

  return AZ_OK;
}

AZ_NODISCARD az_result
__wrap_az_mqtt_outbound_connect(az_mqtt* mqtt, az_mqtt_connect_data* connect_data);
AZ_NODISCARD az_result
__wrap_az_mqtt_outbound_connect(az_mqtt* mqtt, az_mqtt_connect_data* connect_data)
{
  (void)mqtt;
  (void)connect_data;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt_outbound_sub(az_mqtt* mqtt, az_mqtt_sub_data* sub_data);
AZ_NODISCARD az_result __wrap_az_mqtt_outbound_sub(az_mqtt* mqtt, az_mqtt_sub_data* sub_data)
{
  (void)mqtt;
  (void)sub_data;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt_outbound_pub(az_mqtt* mqtt, az_mqtt_pub_data* pub_data);
AZ_NODISCARD az_result __wrap_az_mqtt_outbound_pub(az_mqtt* mqtt, az_mqtt_pub_data* pub_data)
{
  (void)mqtt;
  (void)pub_data;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt_outbound_disconnect(az_mqtt* mqtt);
AZ_NODISCARD az_result __wrap_az_mqtt_outbound_disconnect(az_mqtt* mqtt)
{
  (void)mqtt;

  return AZ_OK;
}

#endif // _az_MOCK_ENABLED
