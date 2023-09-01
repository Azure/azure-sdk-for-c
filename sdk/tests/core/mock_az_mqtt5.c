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
#include <azure/core/az_mqtt5_property_bag.h>
#include <azure/core/az_mqtt5_config.h>
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

AZ_NODISCARD az_result __wrap_az_mqtt5_init(az_mqtt5* mqtt5, az_mqtt5_options const* options);
AZ_NODISCARD az_result __wrap_az_mqtt5_init(az_mqtt5* mqtt5, az_mqtt5_options const* options)
{
  (void)mqtt5;
  (void)options;

  return AZ_OK;
}

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

AZ_NODISCARD az_result __wrap_az_mqtt5_property_bag_append_binary(az_mqtt5_property_bag *property_bag, az_mqtt5_property_type type, az_mqtt5_property_binarydata *prop_bindata);
AZ_NODISCARD az_result __wrap_az_mqtt5_property_bag_append_binary(az_mqtt5_property_bag *property_bag, az_mqtt5_property_type type, az_mqtt5_property_binarydata *prop_bindata)
{
  (void)property_bag;
  (void)type;
  (void)prop_bindata;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt5_property_bag_append_string(az_mqtt5_property_bag *property_bag, az_mqtt5_property_type type, az_mqtt5_property_string *prop_str);
AZ_NODISCARD az_result __wrap_az_mqtt5_property_bag_append_string(az_mqtt5_property_bag *property_bag, az_mqtt5_property_type type, az_mqtt5_property_string *prop_str)
{
  (void)property_bag;
  (void)type;
  (void)prop_str;

  return AZ_OK;
}

AZ_NODISCARD az_result __wrap_az_mqtt5_property_bag_clear(az_mqtt5_property_bag *property_bag);
AZ_NODISCARD az_result __wrap_az_mqtt5_property_bag_clear(az_mqtt5_property_bag *property_bag)
{
  (void)property_bag;

  return AZ_OK;
}


#endif // _az_MOCK_ENABLED
