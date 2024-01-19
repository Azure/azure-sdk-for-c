// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Contains the az_mqtt5.h mock implementation.
 *
 */

#ifdef _az_MOCK_ENABLED

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_config.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>

#if defined(TRANSPORT_MOSQUITTO)
#include <mosquitto.h>
#endif

#include <pthread.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

#if defined(TRANSPORT_MOSQUITTO)
AZ_NODISCARD az_result __wrap_az_mqtt5_init(
    az_mqtt5* mqtt5,
    struct mosquitto** mosquitto_handle,
    az_mqtt5_options const* options);
AZ_NODISCARD az_result __wrap_az_mqtt5_init(
    az_mqtt5* mqtt5,
    struct mosquitto** mosquitto_handle,
    az_mqtt5_options const* options)
{
  (void)mosquitto_handle;
  (void)options;

  mqtt5->_internal.platform_mqtt5.pipeline = NULL;

  return AZ_OK;
}
#else
AZ_NODISCARD az_result
__wrap_az_mqtt5_init(az_mqtt5* mqtt5, void* notransport_handle, az_mqtt5_options const* options);
AZ_NODISCARD az_result
__wrap_az_mqtt5_init(az_mqtt5* mqtt5, void* notransport_handle, az_mqtt5_options const* options)
{
  (void)notransport_handle;
  (void)options;

  mqtt5->_internal.platform_mqtt5.pipeline = NULL;

  return AZ_OK;
}
#endif

AZ_NODISCARD az_result
__wrap_az_mqtt5_outbound_connect(az_mqtt5* mqtt5, az_mqtt5_connect_data* connect_data);
AZ_NODISCARD az_result
__wrap_az_mqtt5_outbound_connect(az_mqtt5* mqtt5, az_mqtt5_connect_data* connect_data)
{
  (void)mqtt5;
  (void)connect_data;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data);
AZ_NODISCARD az_result __wrap_az_mqtt5_outbound_sub(az_mqtt5* mqtt5, az_mqtt5_sub_data* sub_data)
{
  (void)mqtt5;
  (void)sub_data;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data);
AZ_NODISCARD az_result __wrap_az_mqtt5_outbound_pub(az_mqtt5* mqtt5, az_mqtt5_pub_data* pub_data)
{
  (void)mqtt5;
  (void)pub_data;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5);
AZ_NODISCARD az_result __wrap_az_mqtt5_outbound_disconnect(az_mqtt5* mqtt5)
{
  (void)mqtt5;

  return AZ_OK;
}

#endif // _az_MOCK_ENABLED
