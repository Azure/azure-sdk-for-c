// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample_pnp_device_info_component.h"
#include "sample_pnp_mqtt_component.h"

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

/* Reported property keys and values */
static const az_span sample_pnp_device_info_software_version_property_name = AZ_SPAN_LITERAL_FROM_STR("swVersion");
static const az_span sample_pnp_device_info_software_version_property_value = AZ_SPAN_LITERAL_FROM_STR("1.0.0.0");
static const az_span sample_pnp_device_info_manufacturer_property_name = AZ_SPAN_LITERAL_FROM_STR("manufacturer");
static const az_span sample_pnp_device_info_manufacturer_property_value = AZ_SPAN_LITERAL_FROM_STR("Sample-Manufacturer");
static const az_span sample_pnp_device_info_model_property_name = AZ_SPAN_LITERAL_FROM_STR("model");
static const az_span sample_pnp_device_info_model_property_value = AZ_SPAN_LITERAL_FROM_STR("pnp-sample-Model-123");
static const az_span sample_pnp_device_info_os_name_property_name = AZ_SPAN_LITERAL_FROM_STR("osName");
static const az_span sample_pnp_device_info_os_name_property_value = AZ_SPAN_LITERAL_FROM_STR("Contoso");
static const az_span sample_pnp_device_info_processor_architecture_property_name = AZ_SPAN_LITERAL_FROM_STR("processorArchitecture");
static const az_span sample_pnp_device_info_processor_architecture_property_value = AZ_SPAN_LITERAL_FROM_STR("Contoso-Arch-64bit");
static const az_span sample_pnp_device_info_processor_manufacturer_property_name = AZ_SPAN_LITERAL_FROM_STR("processorManufacturer");
static const az_span sample_pnp_device_info_processor_manufacturer_property_value = AZ_SPAN_LITERAL_FROM_STR("Processor Manufacturer(TM)");
static const az_span sample_pnp_device_info_total_storage_property_name
    = AZ_SPAN_LITERAL_FROM_STR("totalStorage");
static const double sample_pnp_device_info_total_storage_property_value = 1024.0;
static const az_span sample_pnp_device_info_total_memory_property_name
    = AZ_SPAN_LITERAL_FROM_STR("totalMemory");
static const double sample_pnp_device_info_total_memory_property_value = 128;

az_result sample_pnp_device_info_get_report_data(
    const az_iot_hub_client* client,
    sample_pnp_mqtt_message* mqtt_message)
{
  az_json_writer jw;
  AZ_RETURN_IF_FAILED(az_json_writer_init(&jw, mqtt_message->payload_span, NULL));

  AZ_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_manufacturer_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, sample_pnp_device_info_manufacturer_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_model_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, sample_pnp_device_info_model_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_software_version_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, sample_pnp_device_info_software_version_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_os_name_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, sample_pnp_device_info_os_name_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_processor_architecture_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, sample_pnp_device_info_processor_architecture_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_processor_manufacturer_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_string(&jw, sample_pnp_device_info_processor_manufacturer_property_value));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_total_storage_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, sample_pnp_device_info_total_storage_property_value, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_property_name(&jw, sample_pnp_device_info_total_memory_property_name));
  AZ_RETURN_IF_FAILED(az_json_writer_append_double(&jw, sample_pnp_device_info_total_memory_property_value, DOUBLE_DECIMAL_PLACE_DIGITS));
  AZ_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  mqtt_message->out_payload_span = az_json_writer_get_bytes_used_in_destination(&jw);

  AZ_RETURN_IF_FAILED(az_iot_hub_client_twin_patch_get_publish_topic(
      client,
      get_request_id(),
      mqtt_message->topic,
      mqtt_message->topic_length,
      mqtt_message->out_topic_length));

  return AZ_OK;
}
