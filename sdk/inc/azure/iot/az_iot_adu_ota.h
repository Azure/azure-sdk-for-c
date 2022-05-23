// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief
 * @note
 *
 * @note
 */

#ifndef _az_IOT_ADU_OTA_H
#define _az_IOT_ADU_OTA_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/* ADU Agent Version */
#define AZ_IOT_ADU_AGENT_VERSION "DU;agent/0.8.0-rc1-public-preview"

/* ADU PnP Component Name */
#define AZ_IOT_ADU_PROPERTIES_COMPONENT_NAME "deviceUpdate"

/* ADU Service Actions */
#define AZ_IOT_ADU_OTA_SERVICE_ACTION_APPLY_DEPLOYMENT 3
#define AZ_IOT_ADU_OTA_SERVICE_ACTION_CANCEL 255

/* ADU Agent States */
#define AZ_IOT_ADU_OTA_AGENT_STATE_IDLE 0
#define AZ_IOT_ADU_OTA_AGENT_STATE_DEPLOYMENT_IN_PROGRESS 6
#define AZ_IOT_ADU_OTA_AGENT_STATE_FAILED 255

/* Maximum Number of Files Handled by this ADU Agent */
#define AZ_IOT_ADU_OTA_FILE_URL_MAX_COUNT 10
#define MAX_INSTRUCTIONS_STEPS 10
#define MAX_FILE_HASH_COUNT 2

typedef struct
{
  az_span provider;
  az_span name;
  az_span version;
} az_iot_adu_ota_update_id;

/*
 * For reference:
 * https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play
 */
typedef struct
{
  az_span manufacturer;
  az_span model;
  az_span adu_version;
  az_span do_version;
  az_iot_adu_ota_update_id update_id;
} az_iot_adu_ota_device_information;

typedef struct
{
  az_span step_id;
  int32_t result_code;
  int32_t extended_result_code;
  az_span result_details;
} az_iot_adu_ota_step_result;

typedef struct
{
  int32_t result_code;
  int32_t extended_result_code;
  az_span result_details;
  int32_t step_results_count;
  az_iot_adu_ota_step_result step_results[MAX_INSTRUCTIONS_STEPS];
} az_iot_adu_ota_install_result;

typedef struct
{
  int32_t action;
  az_span id;
  az_span retry_timestamp;
} az_iot_adu_ota_workflow;

typedef struct
{
  az_span id;
  az_span url;
} az_iot_adu_ota_file_url;

/**
 * @brief Structure that holds the parsed contents of the OTA
 *        request in the Plug and Play writable properties sent
 *        by the ADU service.
 */
typedef struct
{
  az_iot_adu_ota_workflow workflow;
  az_span update_manifest;
  az_span update_manifest_signature;
  az_iot_adu_ota_file_url file_urls[AZ_IOT_ADU_OTA_FILE_URL_MAX_COUNT];
  uint32_t file_urls_count;
} az_iot_adu_ota_update_request;

typedef struct
{
  az_span device_manufacturer;
  az_span device_model;
} az_iot_adu_ota_update_manifest_compatibility;

typedef struct
{
  az_span installed_criteria;
} az_iot_adu_ota_update_manifest_instructions_step_handler_properties;

typedef struct
{
  az_span handler;
  az_span files[AZ_IOT_ADU_OTA_FILE_URL_MAX_COUNT];
  uint32_t files_count;
  az_iot_adu_ota_update_manifest_instructions_step_handler_properties handler_properties;
} az_iot_adu_ota_update_manifest_instructions_step;

typedef struct
{
  az_iot_adu_ota_update_manifest_instructions_step steps[MAX_INSTRUCTIONS_STEPS];
  uint32_t steps_count;
} az_iot_adu_ota_update_manifest_instructions;

typedef struct
{
  az_span id;
  az_span hash;
} az_iot_adu_ota_update_manifest_file_hash;

typedef struct
{
  az_span id;
  az_span file_name;
  uint32_t size_in_bytes;
  az_iot_adu_ota_update_manifest_file_hash hashes[MAX_FILE_HASH_COUNT];
  uint32_t hashes_count;
} az_iot_adu_ota_update_manifest_file;

/**
 * @brief Structure that holds the parsed contents of the update manifest
 *        sent by the ADU service.
 */
typedef struct
{
  az_span manifest_version;
  az_iot_adu_ota_update_id update_id;
  // TODO: confirm compat is always through manufacturer and model.
  //       It might not be, so this needs to be a generic property bag instead.
  az_iot_adu_ota_update_manifest_compatibility compatibility;
  az_iot_adu_ota_update_manifest_instructions instructions;
  az_iot_adu_ota_update_manifest_file files[AZ_IOT_ADU_OTA_FILE_URL_MAX_COUNT];
  uint32_t files_count;
  az_span create_date_time;
} az_iot_adu_ota_update_manifest;

/**
 * @brief Verifies if the Azure Plug-and-Play writable properties component
 *        is for ADU device update.
 *
 * @param[in] component_name    #az_span pointing to the component name in the
 *                              writable properties.
 * @return A boolean indicating if the component name is for ADU device update.
 */
AZ_NODISCARD bool az_iot_adu_ota_is_component_device_update(az_span component_name);

/**
 * @brief Generates the Azure Plug-and-Play (reported) properties payload
 *        with the state of the ADU agent.
 *
 * @param[in] iot_hub_client        A pointer to the #az_iot_hub_client instance.
 * @param[in] device_information    A pointer to a #az_iot_adu_ota_device_information
 *                                  structure with all the details of the device,
 *                                  as required by the ADU service.
 * @param[in] agent_state           An integer value indicating the current state of
 *                                  the ADU agent. Use the values defined by the
 *                                  #AZ_IOT_ADU_OTA_AGENT_STATE macros in this header.
 *                                  Please see the ADU online documentation for more
 *                                  details.
 * @param[in] workflow              A pointer to a #az_iot_adu_ota_workflow instance
 *                                  indicating the current ADU workflow being processed,
 *                                  if an ADU service workflow was received. Use NULL
 *                                  if no device update is in progress.
 * @param[in] last_install_result   A pointer to a #az_iot_adu_ota_install_result
 *                                  instance with the results of the current or past
 *                                  device update workflow, if available. Use NULL
 *                                  if no results are available.
 * @param[in] payload               An #az_span pointing to the memory buffer where to
 *                                  write the resulting Azure Plug-and-Play properties.
 * @param[out] out_payload          A pointer to hold the resulting #az_span slice of
 *                                  `payload`, fitted to the content generated by this
 *                                  function.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_ota_get_properties_payload(
    az_iot_hub_client const* iot_hub_client,
    az_iot_adu_ota_device_information* device_information,
    int32_t agent_state,
    az_iot_adu_ota_workflow* workflow,
    az_iot_adu_ota_install_result* last_install_result,
    az_span payload,
    az_span* out_payload);

/**
 * @brief Parses the json content from the ADU service writable properties into
 *        a pre-defined structure.
 *
 * @param[in] iot_hub_client       A pointer to the #az_iot_hub_client instance.
 * @param[in] jr                   A #az_json_reader initialized with the ADU
 *                                 service writable properties json, set to the
 *                                 beginning of the json object that is the value
 *                                 of the ADU component.
 * @param[in] buffer               An #az_span buffer where to write the parsed
 *                                 values read from the json content.
 * @param[out] update_request      A pointer to the #az_iot_adu_ota_update_request
 *                                 structure where to store the parsed contents
 *                                 read from the `jr` json reader.
 *                                 In summary, this structure holds #az_span
 *                                 instances that point to the actual data
 *                                 parsed from `jr` and copied to `buffer`.
 * @param[out] buffer_remainder    A pointer to an #az_span where to store the
 *                                 remaining available space of `buffer`.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_ota_parse_service_properties(
    az_iot_hub_client const* iot_hub_client,
    az_json_reader* jr,
    az_span buffer,
    az_iot_adu_ota_update_request* update_request,
    az_span* buffer_remainder);

/**
 * @brief    Generates the payload necessary to respond to the service
             after receiving incoming properties.
 *
 * @param[in] iot_hub_client    A pointer to the #az_iot_hub_client instance.
 * @param[in] version           Version of the writable properties.
 * @param[in] status            Azure Plug-and-Play status code for the
 *                              writable properties acknowledgement.
 * @param[in] payload           An #az_span pointing to the memory buffer where to
 *                              write the resulting Azure Plug-and-Play properties.
 * @param[out] out_payload      A pointer to hold the resulting #az_span slice of
 *                              `payload`, fitted to the content generated by this
 *                              function.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_ota_get_service_properties_response(
    az_iot_hub_client const* iot_hub_client,
    int32_t version,
    int32_t status,
    az_span payload,
    az_span* out_payload);

/**
 * @brief Parses the json content from the ADU service update manifest into
 *        a pre-defined structure.
 *
 * @param[in] payload             ADU update manifest, as unescaped json content.
 * @param[out] update_manifest    The structure where the parsed values of the
 *                                manifest are stored. Values are not copied from
 *                                `payload`, the fields of the structure just
 *                                point to the positions in `payload` where the
 *                                data is present, except for numeric and boolean
 *                                values (which are parsed into the respective
 *                                data types).
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_ota_parse_update_manifest(
    az_span payload,
    az_iot_adu_ota_update_manifest* update_manifest);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_ADU_OTA_H
