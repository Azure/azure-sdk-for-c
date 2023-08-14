// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_property_bag.h>
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

AZ_NODISCARD az_result
az_mqtt5_property_bag_init(az_mqtt5_property_bag* property_bag, az_mqtt5* mqtt5, void* options)
{
  (void)property_bag;
  (void)mqtt5;
  (void)options;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_empty(az_mqtt5_property_bag* property_bag)
{
  (void)property_bag;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_string_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_string* prop_str)
{
  (void)property_bag;
  (void)type;
  (void)prop_str;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_stringpair_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_stringpair* prop_strpair)
{
  (void)property_bag;
  (void)type;
  (void)prop_strpair;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_byte_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t prop_byte)
{
  (void)property_bag;
  (void)type;
  (void)prop_byte;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_int_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t prop_int)
{
  (void)property_bag;
  (void)type;
  (void)prop_int;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_binary_append(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_binarydata* prop_bindata)
{
  (void)property_bag;
  (void)type;
  (void)prop_bindata;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_string_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_string* out_prop_str)
{
  (void)property_bag;
  (void)type;
  (void)out_prop_str;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_stringpair_find(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span key,
    az_mqtt5_property_stringpair* out_prop_strpair)
{
  (void)property_bag;
  (void)type;
  (void)key;
  (void)out_prop_strpair;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_byte_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t* out_prop_byte)
{
  (void)property_bag;
  (void)type;
  (void)out_prop_byte;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_int_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t* out_prop_int)
{
  (void)property_bag;
  (void)type;
  (void)out_prop_int;
  return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_mqtt5_property_bag_binarydata_read(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_binarydata* out_prop_bindata)
{
  (void)property_bag;
  (void)type;
  (void)out_prop_bindata;
  return AZ_ERROR_NOT_IMPLEMENTED;
}
