// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <iot_sample_common.h>

#include "pnp_device_info_component.h"
#include "pnp_mqtt_message.h"

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

/* Reported property keys and values */
static const az_span pnp_device_info_software_version_property_name
    = AZ_SPAN_LITERAL_FROM_STR("swVersion");
static const az_span pnp_device_info_software_version_property_value
    = AZ_SPAN_LITERAL_FROM_STR("1.0.0.0");
static const az_span pnp_device_info_manufacturer_property_name
    = AZ_SPAN_LITERAL_FROM_STR("manufacturer");
static const az_span pnp_device_info_manufacturer_property_value
    = AZ_SPAN_LITERAL_FROM_STR("Sample-Manufacturer");
static const az_span pnp_device_info_model_property_name = AZ_SPAN_LITERAL_FROM_STR("model");
static const az_span pnp_device_info_model_property_value
    = AZ_SPAN_LITERAL_FROM_STR("pnp-sample-Model-123");
static const az_span pnp_device_info_os_name_property_name = AZ_SPAN_LITERAL_FROM_STR("osName");
static const az_span pnp_device_info_os_name_property_value = AZ_SPAN_LITERAL_FROM_STR("Contoso");
static const az_span pnp_device_info_processor_architecture_property_name
    = AZ_SPAN_LITERAL_FROM_STR("processorArchitecture");
static const az_span pnp_device_info_processor_architecture_property_value
    = AZ_SPAN_LITERAL_FROM_STR("Contoso-Arch-64bit");
static const az_span pnp_device_info_processor_manufacturer_property_name
    = AZ_SPAN_LITERAL_FROM_STR("processorManufacturer");
static const az_span pnp_device_info_processor_manufacturer_property_value
    = AZ_SPAN_LITERAL_FROM_STR("Processor Manufacturer(TM)");
static const az_span pnp_device_info_total_storage_property_name
    = AZ_SPAN_LITERAL_FROM_STR("totalStorage");
static const double pnp_device_info_total_storage_property_value = 1024.0;
static const az_span pnp_device_info_total_memory_property_name
    = AZ_SPAN_LITERAL_FROM_STR("totalMemory");
static const double pnp_device_info_total_memory_property_value = 128;

az_result pnp_device_info_get_report_data(
    az_span payload_span,
    az_span* out_payload_span)
{
  az_json_writer jw;
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_init(&jw, payload_span, NULL));

  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, pnp_device_info_manufacturer_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, pnp_device_info_manufacturer_property_value));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, pnp_device_info_model_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, pnp_device_info_model_property_value));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, pnp_device_info_software_version_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, pnp_device_info_software_version_property_value));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, pnp_device_info_os_name_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, pnp_device_info_os_name_property_value));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, pnp_device_info_processor_architecture_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, pnp_device_info_processor_architecture_property_value));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, pnp_device_info_processor_manufacturer_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, pnp_device_info_processor_manufacturer_property_value));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, pnp_device_info_total_storage_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, pnp_device_info_total_storage_property_value, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(
      az_json_writer_append_property_name(&jw, pnp_device_info_total_memory_property_name));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_double(
      &jw, pnp_device_info_total_memory_property_value, DOUBLE_DECIMAL_PLACE_DIGITS));
  IOT_SAMPLE_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  out_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}
