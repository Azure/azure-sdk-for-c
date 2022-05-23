// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_adu_ota.h>
#include <azure/iot/az_iot_hub_client_properties.h>

#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <stdio.h>

/* Define the ADU agent component name.  */
#define AZ_IOT_ADU_OTA_AGENT_COMPONENT_NAME "deviceUpdate"

/* Define the ADU agent interface ID.  */
#define AZ_IOT_ADU_OTA_AGENT_INTERFACE_ID "dtmi:azure:iot:deviceUpdate;1"

/* Define the compatibility.  */
#define AZ_IOT_ADU_OTA_AGENT_COMPATIBILITY "manufacturer,model"

/* Define the ADU agent property name "agent" and sub property names.  */
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_AGENT "agent"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DEVICEPROPERTIES "deviceProperties"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_MANUFACTURER "manufacturer"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_MODEL "model"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INTERFACE_ID "interfaceId"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ADU_VERSION "aduVer"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DO_VERSION "doVer"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES "compatPropertyNames"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INSTALLED_UPDATE_ID "installedUpdateId"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_PROVIDER "provider"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_NAME "name"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_VERSION "version"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_LAST_INSTALL_RESULT "lastInstallResult"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RESULT_CODE "resultCode"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE "extendedResultCode"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RESULT_DETAILS "resultDetails"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_STEP_RESULTS "stepResults"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_STATE "state"

/* Define the ADU agent property name "service" and sub property names.  */
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_SERVICE "service"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_WORKFLOW "workflow"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ACTION "action"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ID "id"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RETRY_TIMESTAMP "retryTimestamp"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_UPDATE_MANIFEST "updateManifest"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_UPDATE_MANIFEST_SIGNATURE "updateManifestSignature"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILEURLS "fileUrls"

#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_MANIFEST_VERSION "manifestVersion"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_UPDATE_ID "updateId"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_COMPATIBILITY "compatibility"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DEVICE_MANUFACTURER "deviceManufacturer"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DEVICE_MODEL "deviceModel"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_GROUP "group"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INSTRUCTIONS "instructions"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_STEPS "steps"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_TYPE "type"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_HANDLER "handler"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_HANDLER_PROPERTIES "handlerProperties"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILES "files"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DETACHED_MANIFEST_FILED "detachedManifestFileId"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INSTALLED_CRITERIA "installedCriteria"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILE_NAME "fileName"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_SIZE_IN_BYTES "sizeInBytes"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_HASHES "hashes"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_SHA256 "sha256"
#define AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_CREATED_DATE_TIME "createdDateTime"

#define NULL_TERM_CHAR_SIZE 1
#define UPDATE_ID_ESCAPING_CHARS_LENGTH 24

#define RETURN_IF_JSON_TOKEN_TYPE_NOT(jr_ptr, json_token_type) \
  if (jr_ptr->token.kind != json_token_type)                   \
  {                                                            \
    return AZ_ERROR_JSON_INVALID_STATE;                        \
  }

#define RETURN_IF_JSON_TOKEN_TEXT_NOT(jr_ptr, literal_text)                         \
  if (!az_json_token_is_text_equal(&jr_ptr->token, AZ_SPAN_FROM_STR(literal_text))) \
  {                                                                                 \
    return AZ_ERROR_JSON_INVALID_STATE;                                             \
  }

static az_span split_az_span(az_span span, int32_t size, az_span* remainder);

AZ_NODISCARD bool az_iot_adu_ota_is_component_device_update(az_span component_name)
{
  return az_span_is_content_equal(
      AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_COMPONENT_NAME), component_name);
}

static az_span generate_update_id_string(
    az_iot_adu_ota_update_id update_id,
    az_span update_id_string)
{
  // TODO: Investigate a way to leverage azure SDK core for this.
  az_span remainder = update_id_string;
  remainder = az_span_copy(remainder, AZ_SPAN_FROM_STR("\"{\\\"provider\\\":\\\""));
  remainder = az_span_copy(remainder, update_id.provider);
  remainder = az_span_copy(remainder, AZ_SPAN_FROM_STR("\\\",\\\"name\\\":\\\""));
  remainder = az_span_copy(remainder, update_id.name);
  remainder = az_span_copy(remainder, AZ_SPAN_FROM_STR("\\\",\\\"version\\\":\\\""));
  remainder = az_span_copy(remainder, update_id.version);
  remainder = az_span_copy(remainder, AZ_SPAN_FROM_STR("\\\"}\""));

  return az_span_slice(
      update_id_string, 0, az_span_size(update_id_string) - az_span_size(remainder));
}

AZ_NODISCARD az_result az_iot_adu_ota_get_properties_payload(
    az_iot_hub_client const* iot_hub_client,
    az_iot_adu_ota_device_information* device_information,
    int32_t agent_state,
    az_iot_adu_ota_workflow* workflow,
    az_iot_adu_ota_install_result* last_install_result,
    az_span payload,
    az_span* out_payload)
{
  _az_PRECONDITION_NOT_NULL(iot_hub_client);
  _az_PRECONDITION_NOT_NULL(device_information);
  _az_PRECONDITION_VALID_SPAN(device_information->manufacturer, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_information->model, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_information->update_id.provider, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_information->update_id.name, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_information->update_id.version, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_information->adu_version, 1, false);
  _az_PRECONDITION_VALID_SPAN(payload, 1, false);
  _az_PRECONDITION_NOT_NULL(out_payload);

  az_json_writer jw;

  /* Update reported property */
  _az_RETURN_IF_FAILED(az_json_writer_init(&jw, payload, NULL));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

  /* Fill the ADU agent component name.  */
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_begin_component(
      iot_hub_client, &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_COMPONENT_NAME)));

  /* Fill the agent property name.  */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_AGENT)));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

  /* Fill the deviceProperties.  */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DEVICEPROPERTIES)));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_MANUFACTURER)));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, device_information->manufacturer));

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_MODEL)));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, device_information->model));

  // TODO: verify if this needs to be exposed as an option.
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INTERFACE_ID)));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_INTERFACE_ID)));

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ADU_VERSION)));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, device_information->adu_version));

  if (!az_span_is_content_equal(device_information->do_version, AZ_SPAN_EMPTY))
  {
    // TODO: verify if 'doVer' is required.
    //       Ref:
    //       https://docs.microsoft.com/en-us/azure/iot-hub-device-update/device-update-plug-and-play
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DO_VERSION)));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, device_information->do_version));
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  /* Fill the compatible property names. */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES)));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_COMPATIBILITY)));

  /* Add last installed update information */
  if (last_install_result != NULL)
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_LAST_INSTALL_RESULT)));
    _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RESULT_CODE)));
    _az_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, last_install_result->result_code));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE)));
    _az_RETURN_IF_FAILED(
        az_json_writer_append_int32(&jw, last_install_result->extended_result_code));

    if (!az_span_is_content_equal(last_install_result->result_details, AZ_SPAN_EMPTY))
    {
      // TODO: Add quotes if result_details is not enclosed by quotes.
      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RESULT_DETAILS)));
      _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, last_install_result->result_details));
    }

    for (int32_t i = 0; i < last_install_result->step_results_count; i++)
    {
      _az_RETURN_IF_FAILED(
          az_json_writer_append_property_name(&jw, last_install_result->step_results[i].step_id));
      _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RESULT_CODE)));
      _az_RETURN_IF_FAILED(
          az_json_writer_append_int32(&jw, last_install_result->step_results[i].result_code));

      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE)));
      _az_RETURN_IF_FAILED(az_json_writer_append_int32(
          &jw, last_install_result->step_results[i].extended_result_code));

      if (!az_span_is_content_equal(
              last_install_result->step_results[i].result_details, AZ_SPAN_EMPTY))
      {
        // TODO: Add quotes if result_details is not enclosed by quotes.
        _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
            &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RESULT_DETAILS)));
        _az_RETURN_IF_FAILED(
            az_json_writer_append_string(&jw, last_install_result->step_results[i].result_details));
      }

      _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
    }

    _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  }

  /* Fill the agent state.   */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_STATE)));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, agent_state));

  /* Fill the workflow.  */
  if (workflow != NULL && (az_span_ptr(workflow->id) != NULL && az_span_size(workflow->id) > 0))
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_WORKFLOW)));
    _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ACTION)));
    _az_RETURN_IF_FAILED(az_json_writer_append_int32(&jw, workflow->action));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ID)));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, workflow->id));

    /* Append retry timestamp in workflow if existed.  */
    if (!az_span_is_content_equal(workflow->retry_timestamp, AZ_SPAN_EMPTY))
    {
      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_RETRY_TIMESTAMP)));
      _az_RETURN_IF_FAILED(az_json_writer_append_string(&jw, workflow->retry_timestamp));
    }
    _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));
  }

  /* Fill installed update id.  */
  // TODO: move last_installed_update_id out of this device_information structure.
  // TODO: rename device_information var and struct to device_properties to match json prop name.

  az_span update_id_string = az_span_slice_to_end(
      payload,
      az_span_size(az_json_writer_get_bytes_used_in_destination(&jw))
          + UPDATE_ID_ESCAPING_CHARS_LENGTH);

  if (az_span_is_content_equal(update_id_string, AZ_SPAN_EMPTY))
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INSTALLED_UPDATE_ID)));
  _az_RETURN_IF_FAILED(az_json_writer_append_json_text(
      &jw, generate_update_id_string(device_information->update_id, update_id_string)));

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_end_component(iot_hub_client, &jw));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

// Reference: AzureRTOS/AZ_IOT_ADU_OTA_agent_service_properties_get(...)
AZ_NODISCARD az_result az_iot_adu_ota_parse_service_properties(
    az_iot_hub_client const* iot_hub_client,
    az_json_reader* jr,
    az_span buffer,
    az_iot_adu_ota_update_request* update_request,
    az_span* buffer_remainder)
{
  _az_PRECONDITION_NOT_NULL(iot_hub_client);
  _az_PRECONDITION_NOT_NULL(jr);
  _az_PRECONDITION_VALID_SPAN(buffer, 1, false);
  _az_PRECONDITION_NOT_NULL(update_request);

  int32_t required_size;
  int32_t out_length;

  RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_PROPERTY_NAME);
  RETURN_IF_JSON_TOKEN_TEXT_NOT(jr, AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_SERVICE);

  _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
  RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_BEGIN_OBJECT);
  _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

  update_request->workflow.action = 0;
  update_request->workflow.id = AZ_SPAN_EMPTY;
  update_request->workflow.retry_timestamp = AZ_SPAN_EMPTY;
  update_request->update_manifest = AZ_SPAN_EMPTY;
  update_request->update_manifest_signature = AZ_SPAN_EMPTY;
  update_request->file_urls_count = 0;

  while (jr->token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_PROPERTY_NAME);

    if (az_json_token_is_text_equal(
            &jr->token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_WORKFLOW)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

      while (jr->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_PROPERTY_NAME);

        if (az_json_token_is_text_equal(
                &jr->token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ACTION)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
          _az_RETURN_IF_FAILED(
              az_json_token_get_int32(&jr->token, &update_request->workflow.action));
        }
        else if (az_json_token_is_text_equal(
                     &jr->token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_ID)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

          required_size = jr->token.size + NULL_TERM_CHAR_SIZE;

          _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

          update_request->workflow.id = split_az_span(buffer, required_size, &buffer);

          _az_RETURN_IF_FAILED(az_json_token_get_string(
              &jr->token,
              (char*)az_span_ptr(update_request->workflow.id),
              az_span_size(update_request->workflow.id),
              &out_length));

          // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the
          // end!!!!!!)
          //       Preferably have a function that does not copy anything.
          update_request->workflow.id = az_span_slice(update_request->workflow.id, 0, out_length);
        }
        else
        {
          // TODO: log unexpected property.
          return AZ_ERROR_JSON_INVALID_STATE;
        }

        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      }
    }
    else if (az_json_token_is_text_equal(
                 &jr->token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_UPDATE_MANIFEST)))
    {
      int32_t update_manifest_length;

      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

      _az_RETURN_IF_FAILED(az_json_token_get_string(
          &jr->token, (char*)az_span_ptr(buffer), az_span_size(buffer), &update_manifest_length));

      // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the end!!!!!!)
      //       Preferably have a function that does not copy anything.
      // TODO: optmize the memory usage for update_manifest:
      //       Here we are copying the entire update manifest [originally escaped] json into
      //       update_request->update_manifest. Later az_iot_adu_ota_parse_update_manifest
      //       parses that json into a az_iot_adu_ota_update_manifest structure, by simply mapping
      //       the values of update_request->update_manifest.
      //       Option 1: there seems to be no workaround for update_request->update_manifest for
      //       copying with
      //                 az_json_token_get_string, since the original update manifest comes as an
      //                 escaped json. What can be done is to make it temporary, and parse the
      //                 update manifest within az_iot_adu_ota_parse_service_request, saving only
      //                 the update manifest values in the (then) provided buffer.
      //       Option 2: Have a function in azure SDK core that can parse an escaped json, allowing
      //       us to
      //                 avoid copying the update manifest at all.
      update_request->update_manifest = split_az_span(buffer, update_manifest_length, &buffer);
    }
    else if (az_json_token_is_text_equal(
                 &jr->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_UPDATE_MANIFEST_SIGNATURE)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

      required_size = jr->token.size + NULL_TERM_CHAR_SIZE;

      _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

      update_request->update_manifest_signature = split_az_span(buffer, required_size, &buffer);

      _az_RETURN_IF_FAILED(az_json_token_get_string(
          &jr->token,
          (char*)az_span_ptr(update_request->update_manifest_signature),
          az_span_size(update_request->update_manifest_signature),
          &out_length));

      // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the end!!!!!!)
      //       Preferably have a function that does not copy anything.
      update_request->update_manifest_signature
          = az_span_slice(update_request->update_manifest_signature, 0, out_length);
    }
    else if (az_json_token_is_text_equal(
                 &jr->token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILEURLS)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));

      while (jr->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_PROPERTY_NAME);

        required_size = jr->token.size + NULL_TERM_CHAR_SIZE;

        _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

        update_request->file_urls[update_request->file_urls_count].id
            = split_az_span(buffer, required_size, &buffer);

        _az_RETURN_IF_FAILED(az_json_token_get_string(
            &jr->token,
            (char*)az_span_ptr(update_request->file_urls[update_request->file_urls_count].id),
            az_span_size(update_request->file_urls[update_request->file_urls_count].id),
            &out_length));

        // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the
        // end!!!!!!)
        //       Preferably have a function that does not copy anything.
        update_request->file_urls[update_request->file_urls_count].id = az_span_slice(
            update_request->file_urls[update_request->file_urls_count].id, 0, out_length);

        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
        RETURN_IF_JSON_TOKEN_TYPE_NOT(jr, AZ_JSON_TOKEN_STRING);

        required_size = jr->token.size + NULL_TERM_CHAR_SIZE;

        _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

        update_request->file_urls[update_request->file_urls_count].url
            = split_az_span(buffer, required_size, &buffer);

        _az_RETURN_IF_FAILED(az_json_token_get_string(
            &jr->token,
            (char*)az_span_ptr(update_request->file_urls[update_request->file_urls_count].url),
            az_span_size(update_request->file_urls[update_request->file_urls_count].url),
            &out_length));

        // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the
        // end!!!!!!)
        //       Preferably have a function that does not copy anything.
        update_request->file_urls[update_request->file_urls_count].url = az_span_slice(
            update_request->file_urls[update_request->file_urls_count].url, 0, out_length);

        update_request->file_urls_count++;

        _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
      }
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(jr));
  }

  if (buffer_remainder != NULL)
  {
    *buffer_remainder = buffer;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_adu_ota_get_service_properties_response(
    az_iot_hub_client const* iot_hub_client,
    int32_t version,
    int32_t status,
    az_span payload,
    az_span* out_payload)
{
  _az_PRECONDITION_NOT_NULL(iot_hub_client);
  _az_PRECONDITION_VALID_SPAN(payload, 1, false);
  _az_PRECONDITION_NOT_NULL(out_payload);

  az_json_writer jw;

  // Component and response status
  _az_RETURN_IF_FAILED(az_json_writer_init(&jw, payload, NULL));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_begin_component(
      iot_hub_client, &jw, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_COMPONENT_NAME)));
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_begin_response_status(
      iot_hub_client,
      &jw,
      AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_SERVICE),
      status,
      version,
      AZ_SPAN_EMPTY));

  // It is not necessary to send the properties back in the acknowledgement.
  // We opt not to send them to reduce the size of the payload.
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(&jw));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  _az_RETURN_IF_FAILED(
      az_iot_hub_client_properties_writer_end_response_status(iot_hub_client, &jw));
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_end_component(iot_hub_client, &jw));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(&jw));

  *out_payload = az_json_writer_get_bytes_used_in_destination(&jw);

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_adu_ota_parse_update_manifest(
    az_span payload,
    az_iot_adu_ota_update_manifest* update_manifest)
{
  _az_PRECONDITION_VALID_SPAN(payload, 1, false);
  _az_PRECONDITION_NOT_NULL(update_manifest);

  az_json_reader jr;

  // Initialize the update_manifest with empty values.
  update_manifest->manifest_version = AZ_SPAN_EMPTY;
  update_manifest->update_id.name = AZ_SPAN_EMPTY;
  update_manifest->update_id.provider = AZ_SPAN_EMPTY;
  update_manifest->update_id.version = AZ_SPAN_EMPTY;
  update_manifest->compatibility.device_manufacturer = AZ_SPAN_EMPTY;
  update_manifest->compatibility.device_model = AZ_SPAN_EMPTY;
  update_manifest->instructions.steps_count = 0;
  update_manifest->files_count = 0;
  update_manifest->create_date_time = AZ_SPAN_EMPTY;

  _az_RETURN_IF_FAILED(az_json_reader_init(&jr, payload, NULL));

  _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
  _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

  while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

    bool property_parsed = true;

    if (az_json_token_is_text_equal(
            &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_MANIFEST_VERSION)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
      update_manifest->manifest_version = jr.token.slice;
    }
    else if (az_json_token_is_text_equal(
                 &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INSTRUCTIONS)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

      if (az_json_token_is_text_equal(
              &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_STEPS)))
      {
        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_ARRAY);
        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

        update_manifest->instructions.steps_count = 0;

        while (jr.token.kind != AZ_JSON_TOKEN_END_ARRAY)
        {
          uint32_t step_index = update_manifest->instructions.steps_count;

          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

          while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
          {
            RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

            if (az_json_token_is_text_equal(
                    &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_HANDLER)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
              RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);

              update_manifest->instructions.steps[step_index].handler = jr.token.slice;
            }
            else if (az_json_token_is_text_equal(
                         &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILES)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
              RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_ARRAY);
              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

              update_manifest->instructions.steps[step_index].files_count = 0;

              while (jr.token.kind != AZ_JSON_TOKEN_END_ARRAY)
              {
                uint32_t file_index = update_manifest->instructions.steps[step_index].files_count;

                RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);

                update_manifest->instructions.steps[step_index].files[file_index] = jr.token.slice;
                update_manifest->instructions.steps[step_index].files_count++;

                _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
              }
            }
            else if (az_json_token_is_text_equal(
                         &jr.token,
                         AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_HANDLER_PROPERTIES)))
            {
              // TODO: properly save installed criteria as a map instead of fixed field.
              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
              RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
              RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

              if (az_json_token_is_text_equal(
                      &jr.token,
                      AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_INSTALLED_CRITERIA)))
              {
                _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
                RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
                update_manifest->instructions.steps[step_index]
                    .handler_properties.installed_criteria
                    = jr.token.slice;
                _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
                RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_END_OBJECT);
              }
              else
              {
                return AZ_ERROR_JSON_INVALID_STATE;
              }
            }
            else
            {
              return AZ_ERROR_JSON_INVALID_STATE;
            }

            _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
          }

          update_manifest->instructions.steps_count++;

          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        }

        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_END_OBJECT);
      }
      else
      {
        // TODO: log unexpected property.
        return AZ_ERROR_JSON_INVALID_STATE;
      }
    }
    else if (az_json_token_is_text_equal(
                 &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_UPDATE_ID)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

      while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

        if (az_json_token_is_text_equal(
                &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_PROVIDER)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
          update_manifest->update_id.provider = jr.token.slice;
        }
        else if (az_json_token_is_text_equal(
                     &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_NAME)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
          update_manifest->update_id.name = jr.token.slice;
        }
        else if (az_json_token_is_text_equal(
                     &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_VERSION)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
          update_manifest->update_id.version = jr.token.slice;
        }
        else
        {
          // TODO: log unexpected property.
          return AZ_ERROR_JSON_INVALID_STATE;
        }

        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      }
    }
    else if (az_json_token_is_text_equal(
                 &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_COMPATIBILITY)))
    {
      // TODO: parse this as a map (dictionary) instead.
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_ARRAY);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

      while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

        if (az_json_token_is_text_equal(
                &jr.token,
                AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DEVICE_MANUFACTURER)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
          update_manifest->compatibility.device_manufacturer = jr.token.slice;
        }
        else if (az_json_token_is_text_equal(
                     &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_DEVICE_MODEL)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
          update_manifest->compatibility.device_model = jr.token.slice;
        }
        else
        {
          // TODO: parse compat as map, and do not return this failure.
          return AZ_ERROR_JSON_INVALID_STATE;
        }

        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      }

      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_END_ARRAY);
    }
    else if (az_json_token_is_text_equal(
                 &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILES)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

      while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        uint32_t files_index = update_manifest->files_count;

        RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

        update_manifest->files[files_index].id = jr.token.slice;

        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

        while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
        {
          RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);

          if (az_json_token_is_text_equal(
                  &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_FILE_NAME)))
          {
            _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
            RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
            update_manifest->files[files_index].file_name = jr.token.slice;
          }
          else if (az_json_token_is_text_equal(
                       &jr.token,
                       AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_SIZE_IN_BYTES)))
          {
            _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
            RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_NUMBER);

            _az_RETURN_IF_FAILED(az_json_token_get_uint32(
                &jr.token, &update_manifest->files[files_index].size_in_bytes));
          }
          else if (az_json_token_is_text_equal(
                       &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_HASHES)))
          {
            _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
            RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_BEGIN_OBJECT);
            _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));

            update_manifest->files[files_index].hashes_count = 0;

            while (jr.token.kind != AZ_JSON_TOKEN_END_OBJECT)
            {
              uint32_t hashes_count = update_manifest->files[files_index].hashes_count;

              RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_PROPERTY_NAME);
              update_manifest->files[files_index].hashes[hashes_count].id = jr.token.slice;
              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
              RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
              update_manifest->files[files_index].hashes[hashes_count].hash = jr.token.slice;

              update_manifest->files[files_index].hashes_count++;

              _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
            }
          }
          else
          {
            return AZ_ERROR_JSON_INVALID_STATE;
          }

          _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
        }

        update_manifest->files_count++;

        _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      }
    }
    else if (az_json_token_is_text_equal(
                 &jr.token, AZ_SPAN_FROM_STR(AZ_IOT_ADU_OTA_AGENT_PROPERTY_NAME_CREATED_DATE_TIME)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      RETURN_IF_JSON_TOKEN_TYPE_NOT((&jr), AZ_JSON_TOKEN_STRING);
      update_manifest->create_date_time = jr.token.slice;
    }
    else
    {
      property_parsed = false;
    }

    if (!property_parsed)
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
      _az_RETURN_IF_FAILED(az_json_reader_skip_children(&jr));
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(&jr));
  }

  return AZ_OK;
}

/* --- az_core extensions --- */
static az_span split_az_span(az_span span, int32_t size, az_span* remainder)
{
  az_span result = az_span_slice(span, 0, size);

  if (remainder != NULL)
  {
    if (az_span_is_content_equal(AZ_SPAN_EMPTY, result))
    {
      *remainder = AZ_SPAN_EMPTY;
    }
    else
    {
      *remainder = az_span_slice(span, size, az_span_size(span));
    }
  }

  return result;
}
