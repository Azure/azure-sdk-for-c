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

#ifndef _az_IOT_ADU_H
#define _az_IOT_ADU_H

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/* ADU Agent Version */
#define AZ_IOT_ADU_CLIENT_AGENT_VERSION "DU;agent/0.8.0-rc1-public-preview"

/* ADU PnP Component Name */
#define AZ_IOT_ADU_CLIENT_PROPERTIES_COMPONENT_NAME "deviceUpdate"

/* ADU Service Responses */
#define AZ_IOT_ADU_CLIENT_REQUEST_ACCEPTED 200
#define AZ_IOT_ADU_CLIENT_REQUEST_REJECTED 406

/* ADU Service Actions */
#define AZ_IOT_ADU_CLIENT_SERVICE_ACTION_APPLY_DEPLOYMENT 3
#define AZ_IOT_ADU_CLIENT_SERVICE_ACTION_CANCEL 255

/* ADU Agent States */
#define AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE 0
#define AZ_IOT_ADU_CLIENT_AGENT_STATE_DEPLOYMENT_IN_PROGRESS 6
#define AZ_IOT_ADU_CLIENT_AGENT_STATE_FAILED 255

/* Maximum Number of Files Handled by this ADU Agent */
#define AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT 10
#define AZ_IOT_ADU_CLIENT_MAX_INSTRUCTIONS_STEPS 10
#define AZ_IOT_ADU_CLIENT_MAX_FILE_HASH_COUNT 2

/* Maximum Number of Custom Device Properties */
#define AZ_IOT_ADU_CLIENT_MAX_DEVICE_CUSTOM_PROPERTIES 5

/* Default Agent Compatibility Properties */
#define AZ_IOT_ADU_CLIENT_AGENT_DEFAULT_COMPATIBILITY_PROPERTIES "manufacturer,model"

/**
 * @brief     Identity of the update request.
 * @remark    This version refers to the update request itself.
 *            For verifying if an update request is applicable to an
 *            ADU agent, use the update manifest instructions steps "installed criteria".
 */
typedef struct
{
  az_span provider;
  az_span name;
  az_span version;
} az_iot_adu_client_update_id;

/**
 * @brief Holds any user-defined custom properties of the device.
 * @remark Implementer can define other device properties to be used
 *         for the compatibility check while targeting the update deployment.
 */
typedef struct
{
  az_span names[AZ_IOT_ADU_CLIENT_MAX_DEVICE_CUSTOM_PROPERTIES];
  az_span values[AZ_IOT_ADU_CLIENT_MAX_DEVICE_CUSTOM_PROPERTIES];
  int32_t count;
} az_iot_adu_device_custom_properties;

/**
 * @brief      Holds the ADU agent device properties.
 * @remarks    These properties are used by the ADU service for matching
 *             update groups and verifying the current update deployed.
 * @link
 * https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play
 */
typedef struct
{
  /**
   * @brief The device manufacturer of the device, reported through deviceProperties.
   */
  az_span manufacturer;
  /**
   * @brief The device model of the device, reported through deviceProperties.
   */
  az_span model;
  /**
   * @brief Implementer can define other device properties to be used for the
   *        compatibility check while targeting the update deployment.
   */
  az_iot_adu_device_custom_properties* custom_properties;
  /**
   * @brief Version of the Device Update agent running on the device.
   * @remark Must be set to AZ_IOT_ADU_CLIENT_AGENT_VERSION.
   */
  az_span adu_version;
  /**
   * @brief  Version of the Delivery Optimization agent.
   * @remark Please see Azure Device Update documentation on how to use
   *         the delivery optimization agent. If unused, set to #AZ_SPAN_EMPTY.
   * @link
   * https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play#device-properties
   */
  az_span delivery_optimization_agent_version;
  /**
   * @brief An ID of the update that is currently installed.
   */
  az_iot_adu_client_update_id update_id;
} az_iot_adu_client_device_properties;

/**
 * @brief The update step result reported by the agent.
 *
 * This details results for a specific step of the update process.
 *
 */
typedef struct
{
  /**
   * @brief A code that contains information about the result of the last update action.
   * @example 700
   */
  int32_t result_code;
  /**
   * @brief A code that contains additional information about the result.
   * @example 0x80004005
   */
  int32_t extended_result_code;
  /**
   * @brief Customer-defined free form string to provide additional result details.
   */
  az_span result_details;
} az_iot_adu_client_step_result;

/**
 * @brief The update result reported by the agent.
 *
 *   This details the result for the overall update.
 */
typedef struct
{
  /**
   * @brief A code that contains information about the result of the last update action.
   * @example 700
   */
  int32_t result_code;
  /**
   * @brief A code that contains additional information about the result.
   * @example 0x80004005
   */
  int32_t extended_result_code;
  /**
   * @brief Customer-defined free form string to provide additional result details.
   */
  az_span result_details;
  /**
   * @brief Number of items in \p step_results.
   */
  int32_t step_results_count;
  /**
   * @brief The results for each step in the update manifest instructions.
   * @remark The number of steps MUST match the number of steps in the
   *         update manifest for the resulting state to be property generated.
   */
  az_iot_adu_client_step_result step_results[AZ_IOT_ADU_CLIENT_MAX_INSTRUCTIONS_STEPS];
} az_iot_adu_client_install_result;

/**
 * @brief A set of values that indicate which deployment the agent is currently working on.
 *
 */
typedef struct
{
  /**
   * @brief An integer that corresponds to an action the agent should perform.
   * @remark Refer to the following defines for the expected values:
   *         AZ_IOT_ADU_CLIENT_AGENT_STATE_IDLE
   *         AZ_IOT_ADU_CLIENT_AGENT_STATE_DEPLOYMENT_IN_PROGRESS
   *         AZ_IOT_ADU_CLIENT_AGENT_STATE_FAILED
   */
  int32_t action;
  /**
   * @brief ID of current deployment.
   */
  az_span id;
  /**
   * @brief Time of last deployment retry.
   */
  az_span retry_timestamp;
} az_iot_adu_client_workflow;

/**
 * @brief A map of file ID to download url.
 */
typedef struct
{
  /**
   * @brief File ID, mapped in the updated manifest.
   */
  az_span id;
  /**
   * @brief Complete url to a file.
   */
  az_span url;
} az_iot_adu_client_file_url;

/**
 * @brief Structure that holds the parsed contents of the ADU
 *        request in the Plug and Play writable properties sent
 *        by the ADU service.
 */
typedef struct
{
  /**
   * @brief A set of values that indicate which deployment the agent is currently working on.
   */
  az_iot_adu_client_workflow workflow;
  /**
   * @brief Description of the content of an update.
   */
  az_span update_manifest;
  /**
   * @brief A JSON Web Signature (JWS) with JSON Web Keys used for source verification.
   */
  az_span update_manifest_signature;
  /**
   * @brief Tells the agent which files to download and the hash to use to verify that the files
   * were downloaded correctly.
   */
  az_iot_adu_client_file_url file_urls[AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT];
  /**
   * @brief Number of items in \p file_urls.
   */
  uint32_t file_urls_count;
} az_iot_adu_client_update_request;

/**
 * @brief User-defined properties for handling an update request.
 *
 */
typedef struct
{
  az_span installed_criteria;
} az_iot_adu_client_update_manifest_instructions_step_handler_properties;

/**
 * @brief Step in the instructions of an update manifest.
 *
 */
typedef struct
{
  /**
   * @brief Name of the component that is expected to handle the step.
   */
  az_span handler;
  /**
   * @brief Files related to this update step.
   */
  az_span files[AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT];
  /**
   * @brief Number of items in \p files.
   */
  uint32_t files_count;
  /**
   * @brief Additional user-defined properties for the update step handler.
   */
  az_iot_adu_client_update_manifest_instructions_step_handler_properties handler_properties;
} az_iot_adu_client_update_manifest_instructions_step;

/**
 * @brief Instructions in the update manifest.
 */
typedef struct
{
  /**
   * @brief Steps of the instructions in an update request.
   */
  az_iot_adu_client_update_manifest_instructions_step
      steps[AZ_IOT_ADU_CLIENT_MAX_INSTRUCTIONS_STEPS];
  /**
   * @brief Number of items in \p steps.
   */
  uint32_t steps_count;
} az_iot_adu_client_update_manifest_instructions;

/**
 * @brief Hash value for a given file.
 *
 */
typedef struct
{
  az_span hash_type;
  az_span hash_value;
} az_iot_adu_client_update_manifest_file_hash;

/**
 * @brief Details of a file referenced in the update request.
 *
 */
typedef struct
{
  /**
   * @brief Identity of a file, referenced in the update request.
   *
   */
  az_span id;
  /**
   * @brief Name of the file.
   *
   */
  az_span file_name;
  /**
   * @brief Size of a file, in bytes.
   *
   */
  uint32_t size_in_bytes;
  /**
   * @brief Hashes provided for a given file in the update request.
   *
   */
  az_iot_adu_client_update_manifest_file_hash hashes[AZ_IOT_ADU_CLIENT_MAX_FILE_HASH_COUNT];
  /**
   * @brief Number of items in \p hashes.
   *
   */
  uint32_t hashes_count;
} az_iot_adu_client_update_manifest_file;

/**
 * @brief Structure that holds the parsed contents of the update manifest
 *        sent by the ADU service.
 */
typedef struct
{
  /**
   * @brief Version of the update manifest schema.
   *
   */
  az_span manifest_version;
  /**
   * @brief User-defined identity of the update manifest.
   *
   */
  az_iot_adu_client_update_id update_id;
  /**
   * @brief Instructions of the update manifest.
   *
   */
  az_iot_adu_client_update_manifest_instructions instructions;
  /**
   * @brief Download urls for the files referenced in the update manifest instructions.
   *
   */
  az_iot_adu_client_update_manifest_file files[AZ_IOT_ADU_CLIENT_MAX_FILE_URL_COUNT];
  /**
   * @brief Number of items in \p files.
   *
   */
  uint32_t files_count;
  az_span create_date_time;
} az_iot_adu_client_update_manifest;

/**
 * @brief User-defined options for the Azure IoT ADU client.
 *
 */
typedef struct
{
  az_span device_compatibility_properties;
} az_iot_adu_client_options;

/**
 * @brief Structure that holds the state of the Azure IoT ADU client.
 *
 */
typedef struct
{
  struct
  {
    az_iot_adu_client_options options;
  } _internal;
} az_iot_adu_client;

/**
 * @brief Gets the default Azure IoT ADU Client options.
 * @details Call this to obtain an initialized #az_iot_adu_client_options structure that can be
 * afterwards modified and passed to #az_iot_adu_client_init.
 *
 * @return #az_iot_adu_client_options.
 */
AZ_NODISCARD az_iot_adu_client_options az_iot_adu_client_options_default();

/**
 * @brief Initializes an Azure IoT ADU Client.
 *
 * @param client  The #az_iot_adu_client to use for this call.
 * @param options A reference to an #az_iot_adu_client_options structure. If `NULL` is passed,
 * the adu client will use the default options. If using custom options, please initialize first by
 * calling az_iot_adu_client_options_default() and then populating relevant options with your own
 * values.
 * @pre \p client must not be `NULL`.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_iot_adu_client_init(az_iot_adu_client* client, az_iot_adu_client_options* options);

/**
 * @brief Verifies if the Azure Plug-and-Play writable properties component
 *        is for ADU device update.
 *
 * @param[in] client            The #az_iot_adu_client to use for this call.
 * @param[in] component_name    #az_span pointing to the component name in the
 *                              writable properties.
 * @return A boolean indicating if the component name is for ADU device update.
 */
AZ_NODISCARD bool az_iot_adu_client_is_component_device_update(
    az_iot_adu_client* client,
    az_span component_name);

/**
 * @brief Generates the Azure Plug-and-Play (reported) properties payload
 *        with the state of the ADU agent.
 *
 * @param[in] client                The #az_iot_adu_client to use for this call.
 * @param[in] device_properties     A pointer to a #az_iot_adu_client_device_properties
 *                                  structure with all the details of the device,
 *                                  as required by the ADU service.
 * @param[in] agent_state           An integer value indicating the current state of
 *                                  the ADU agent. Use the values defined by the
 *                                  #AZ_IOT_ADU_CLIENT_AGENT_STATE macros in this header.
 *                                  Please see the ADU online documentation for more
 *                                  details.
 * @param[in] workflow              A pointer to a #az_iot_adu_client_workflow instance
 *                                  indicating the current ADU workflow being processed,
 *                                  if an ADU service workflow was received. Use NULL
 *                                  if no device update is in progress.
 * @param[in] last_install_result   A pointer to a #az_iot_adu_client_install_result
 *                                  instance with the results of the current or past
 *                                  device update workflow, if available. Use NULL
 *                                  if no results are available.
 * @param[in,out] ref_json_writer   An #az_json_writer initialized with the memory where
 *                                  to write the property payload.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_client_get_agent_state_payload(
    az_iot_adu_client* client,
    az_iot_adu_client_device_properties* device_properties,
    int32_t agent_state,
    az_iot_adu_client_workflow* workflow,
    az_iot_adu_client_install_result* last_install_result,
    az_json_writer* ref_json_writer);

/**
 * @brief Parses the json content from the ADU service writable properties into
 *        a pre-defined structure.
 *
 * @param[in] client               The #az_iot_adu_client to use for this call.
 * @param[in] ref_json_reader      A #az_json_reader initialized with the ADU
 *                                 service writable properties json, set to the
 *                                 beginning of the json object that is the value
 *                                 of the ADU component.
 * @param[in] buffer               An #az_span buffer where to write the parsed
 *                                 values read from the json content.
 * @param[out] update_request      A pointer to the #az_iot_adu_client_update_request
 *                                 structure where to store the parsed contents
 *                                 read from the `ref_json_reader` json reader.
 *                                 In summary, this structure holds #az_span
 *                                 instances that point to the actual data
 *                                 parsed from `ref_json_reader` and copied to `buffer`.
 * @param[out] buffer_remainder    A pointer to an #az_span where to store the
 *                                 remaining available space of `buffer`.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_client_parse_service_properties(
    az_iot_adu_client* client,
    az_json_reader* ref_json_reader,
    az_span buffer,
    az_iot_adu_client_update_request* update_request,
    az_span* buffer_remainder);

/**
 * @brief    Generates the payload necessary to respond to the service
             after receiving incoming properties.
 *
 * @param[in] client            The #az_iot_adu_client to use for this call.
 * @param[in] version           Version of the writable properties.
 * @param[in] status            Azure Plug-and-Play status code for the
 *                              writable properties acknowledgement.
 * @param[in] ref_json_writer   An #az_json_writer pointing to the memory buffer where to
 *                              write the resulting Azure Plug-and-Play properties.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_client_get_service_properties_response(
    az_iot_adu_client* client,
    int32_t version,
    int32_t status,
    az_json_writer* ref_json_writer);

/**
 * @brief Parses the json content from the ADU service update manifest into
 *        a pre-defined structure.
 *
 * @param[in] client              The #az_iot_adu_client to use for this call.
 * @param[in] ref_json_reader     ADU update manifest, as initialized json reader.
 * @param[out] update_manifest    The structure where the parsed values of the
 *                                manifest are stored. Values are not copied from
 *                                `payload`, the fields of the structure just
 *                                point to the positions in `payload` where the
 *                                data is present, except for numeric and boolean
 *                                values (which are parsed into the respective
 *                                data types).
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_adu_client_parse_update_manifest(
    az_iot_adu_client* client,
    az_json_reader* ref_json_reader,
    az_iot_adu_client_update_manifest* update_manifest);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_ADU_H
