// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stddef.h>

#include <azure/az_core.h>

#include <iot_sample_common.h>

#include "pnp_device_info_component.h"

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

// Reported property keys and values
static az_span const software_version_property_name = AZ_SPAN_LITERAL_FROM_STR("swVersion");
static az_span const software_version_property_value = AZ_SPAN_LITERAL_FROM_STR("1.0.0.0");
static az_span const manufacturer_property_name = AZ_SPAN_LITERAL_FROM_STR("manufacturer");
static az_span const manufacturer_property_value = AZ_SPAN_LITERAL_FROM_STR("Sample-Manufacturer");
static az_span const model_property_name = AZ_SPAN_LITERAL_FROM_STR("model");
static az_span const model_property_value = AZ_SPAN_LITERAL_FROM_STR("pnp-sample-Model-123");
static az_span const os_name_property_name = AZ_SPAN_LITERAL_FROM_STR("osName");
static az_span const os_name_property_value = AZ_SPAN_LITERAL_FROM_STR("Contoso");
static az_span const processor_architecture_property_name
    = AZ_SPAN_LITERAL_FROM_STR("processorArchitecture");
static az_span const processor_architecture_property_value
    = AZ_SPAN_LITERAL_FROM_STR("Contoso-Arch-64bit");
static az_span const processor_manufacturer_property_name
    = AZ_SPAN_LITERAL_FROM_STR("processorManufacturer");
static az_span const processor_manufacturer_property_value
    = AZ_SPAN_LITERAL_FROM_STR("Processor Manufacturer(TM)");
static az_span const total_storage_property_name = AZ_SPAN_LITERAL_FROM_STR("totalStorage");
static double const total_storage_property_value = 1024.0;
static az_span const total_memory_property_name = AZ_SPAN_LITERAL_FROM_STR("totalMemory");
static double const total_memory_property_value = 128;

void pnp_device_info_build_reported_property(az_span payload, az_span* out_payload)
{
  char const* const log = "Failed to build reported property payload for device info";

  az_json_writer jw;
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_init(&jw, payload, NULL), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_begin_object(&jw), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, manufacturer_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, manufacturer_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_property_name(&jw, model_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, model_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, software_version_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_string(&jw, software_version_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, os_name_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_string(&jw, os_name_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, processor_architecture_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_string(&jw, processor_architecture_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, processor_manufacturer_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_string(&jw, processor_manufacturer_property_value), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, total_storage_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(&jw, total_storage_property_value, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_property_name(&jw, total_memory_property_name), log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(
      az_json_writer_append_double(&jw, total_memory_property_value, DOUBLE_DECIMAL_PLACE_DIGITS),
      log);
  IOT_SAMPLE_EXIT_IF_AZ_FAILED(az_json_writer_append_end_object(&jw), log);

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);
}
