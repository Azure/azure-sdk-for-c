// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/az_iot_adu_client.h>
#include <azure/iot/az_iot_hub_client_properties.h>

#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <stdio.h>

/* Define the ADU agent component name.  */
#define AZ_IOT_ADU_CLIENT_AGENT_COMPONENT_NAME "deviceUpdate"

/* Define the ADU agent interface ID.  */
#define AZ_IOT_ADU_CLIENT_AGENT_INTERFACE_ID "dtmi:azure:iot:deviceUpdate;1"

/* Define the ADU agent property name "agent" and sub property names.  */
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_AGENT "agent"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DEVICEPROPERTIES "deviceProperties"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_MANUFACTURER "manufacturer"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_MODEL "model"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INTERFACE_ID "interfaceId"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ADU_VERSION "aduVer"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DO_VERSION "doVer"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES "compatPropertyNames"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INSTALLED_UPDATE_ID "installedUpdateId"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_PROVIDER "provider"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_NAME "name"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_VERSION "version"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_LAST_INSTALL_RESULT "lastInstallResult"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RESULT_CODE "resultCode"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE "extendedResultCode"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RESULT_DETAILS "resultDetails"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_STEP_RESULTS "stepResults"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_STATE "state"

/* Define the ADU agent property name "service" and sub property names.  */
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SERVICE "service"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_WORKFLOW "workflow"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ACTION "action"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ID "id"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RETRY_TIMESTAMP "retryTimestamp"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_UPDATE_MANIFEST "updateManifest"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_UPDATE_MANIFEST_SIGNATURE "updateManifestSignature"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILEURLS "fileUrls"

#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_MANIFEST_VERSION "manifestVersion"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_UPDATE_ID "updateId"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_COMPATIBILITY "compatibility"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DEVICE_MANUFACTURER "deviceManufacturer"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DEVICE_MODEL "deviceModel"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_GROUP "group"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INSTRUCTIONS "instructions"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_STEPS "steps"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_TYPE "type"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_HANDLER "handler"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_HANDLER_PROPERTIES "handlerProperties"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILES "files"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DETACHED_MANIFEST_FILED "detachedManifestFileId"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INSTALLED_CRITERIA "installedCriteria"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILE_NAME "fileName"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SIZE_IN_BYTES "sizeInBytes"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_HASHES "hashes"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SHA256 "sha256"
#define AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_CREATED_DATE_TIME "createdDateTime"

#define NULL_TERM_CHAR_SIZE 1
#define UPDATE_ID_ESCAPING_CHARS_LENGTH 24

#define RETURN_IF_JSON_TOKEN_NOT_TYPE(jr_ptr, json_token_type) \
  if (jr_ptr->token.kind != json_token_type)                   \
  {                                                            \
    return AZ_ERROR_JSON_INVALID_STATE;                        \
  }

#define RETURN_IF_JSON_TOKEN_NOT_TEXT(jr_ptr, literal_text)                         \
  if (!az_json_token_is_text_equal(&jr_ptr->token, AZ_SPAN_FROM_STR(literal_text))) \
  {                                                                                 \
    return AZ_ERROR_JSON_INVALID_STATE;                                             \
  }

static az_span split_az_span(az_span span, int32_t size, az_span* remainder);

const az_span default_compatibility_properties
    = AZ_SPAN_LITERAL_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_DEFAULT_COMPATIBILITY_PROPERTIES);

AZ_NODISCARD az_iot_adu_client_options az_iot_adu_client_options_default()
{
  return (az_iot_adu_client_options){ .device_compatibility_properties
                                      = default_compatibility_properties };
}

AZ_NODISCARD az_result
az_iot_adu_client_init(az_iot_adu_client* client, az_iot_adu_client_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);

  client->_internal.options = options == NULL ? az_iot_adu_client_options_default() : *options;

  return AZ_OK;
}

AZ_NODISCARD bool az_iot_adu_client_is_component_device_update(
    az_iot_adu_client* client,
    az_span component_name)
{
  _az_PRECONDITION_NOT_NULL(client);

  (void)client;

  return az_span_is_content_equal(
      AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_COMPONENT_NAME), component_name);
}

static az_span generate_update_id_string(
    az_iot_adu_client_update_id update_id,
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

#define RESULT_STEP_ID_PREFIX "step_"
#define MAX_UINT32_NUMBER_OF_DIGITS 10
#define RESULT_STEP_ID_MAX_SIZE (sizeof(RESULT_STEP_ID_PREFIX) + MAX_UINT32_NUMBER_OF_DIGITS)

static az_span get_json_writer_remaining_buffer(az_json_writer* jw)
{
  return az_span_slice_to_end(
      jw->_internal.destination_buffer,
      az_span_size(az_json_writer_get_bytes_used_in_destination(jw)));
}

static az_result generate_step_id(az_span buffer, uint32_t step_index, az_span* step_id)
{
  az_result result;
  *step_id = buffer;
  buffer = az_span_copy(buffer, AZ_SPAN_FROM_STR(RESULT_STEP_ID_PREFIX));

  result = az_span_u32toa(buffer, step_index, &buffer);
  _az_RETURN_IF_FAILED(result);

  *step_id = az_span_slice(*step_id, 0, az_span_size(*step_id) - az_span_size(buffer));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_adu_client_get_agent_state_payload(
    az_iot_adu_client* client,
    az_iot_adu_client_device_properties* device_properties,
    int32_t agent_state,
    az_iot_adu_client_workflow* workflow,
    az_iot_adu_client_install_result* last_install_result,
    az_json_writer* ref_json_writer)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(device_properties);
  _az_PRECONDITION_VALID_SPAN(device_properties->manufacturer, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_properties->model, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_properties->update_id.provider, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_properties->update_id.name, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_properties->update_id.version, 1, false);
  _az_PRECONDITION_VALID_SPAN(device_properties->adu_version, 1, false);
  _az_PRECONDITION_NOT_NULL(ref_json_writer);

  /* Update reported property */
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));

  /* Fill the ADU agent component name.  */
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_begin_component(
      NULL, ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_COMPONENT_NAME)));

  /* Fill the agent property name.  */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_AGENT)));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));

  /* Fill the deviceProperties.  */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DEVICEPROPERTIES)));
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_MANUFACTURER)));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(ref_json_writer, device_properties->manufacturer));

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_MODEL)));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(ref_json_writer, device_properties->model));

  if (device_properties->custom_properties != NULL)
  {
    for (int32_t custom_property_index = 0;
         custom_property_index < device_properties->custom_properties->count;
         custom_property_index++)
    {
      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          ref_json_writer, device_properties->custom_properties->names[custom_property_index]));
      _az_RETURN_IF_FAILED(az_json_writer_append_string(
          ref_json_writer, device_properties->custom_properties->values[custom_property_index]));
    }
  }

  // TODO: verify if this needs to be exposed as an option.
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INTERFACE_ID)));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_INTERFACE_ID)));

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ADU_VERSION)));
  _az_RETURN_IF_FAILED(
      az_json_writer_append_string(ref_json_writer, device_properties->adu_version));

  if (!az_span_is_content_equal(
          device_properties->delivery_optimization_agent_version, AZ_SPAN_EMPTY))
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_DO_VERSION)));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(
        ref_json_writer, device_properties->delivery_optimization_agent_version));
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));

  /* Fill the compatibility property names. */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer,
      AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_COMPAT_PROPERTY_NAMES)));
  _az_RETURN_IF_FAILED(az_json_writer_append_string(
      ref_json_writer, client->_internal.options.device_compatibility_properties));

  /* Add last installed update information */
  if (last_install_result != NULL)
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer,
        AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_LAST_INSTALL_RESULT)));
    _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RESULT_CODE)));
    _az_RETURN_IF_FAILED(
        az_json_writer_append_int32(ref_json_writer, last_install_result->result_code));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer,
        AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE)));
    _az_RETURN_IF_FAILED(
        az_json_writer_append_int32(ref_json_writer, last_install_result->extended_result_code));

    if (!az_span_is_content_equal(last_install_result->result_details, AZ_SPAN_EMPTY))
    {
      // TODO: Add quotes if result_details is not enclosed by quotes.
      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RESULT_DETAILS)));
      _az_RETURN_IF_FAILED(
          az_json_writer_append_string(ref_json_writer, last_install_result->result_details));
    }

    for (int32_t i = 0; i < last_install_result->step_results_count; i++)
    {
      az_span remaining_buffer = get_json_writer_remaining_buffer(ref_json_writer);
      // Taking from the end of the remaining buffer to avoid az_json_writer overlapping
      // with the data we will generate in that buffer.
      az_span step_id = az_span_slice_to_end(
          remaining_buffer, az_span_size(remaining_buffer) - (int32_t)RESULT_STEP_ID_MAX_SIZE);

      _az_RETURN_IF_FAILED(generate_step_id(step_id, (uint32_t)i, &step_id));

      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(ref_json_writer, step_id));
      _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));

      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RESULT_CODE)));
      _az_RETURN_IF_FAILED(az_json_writer_append_int32(
          ref_json_writer, last_install_result->step_results[i].result_code));

      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          ref_json_writer,
          AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_EXTENDED_RESULT_CODE)));
      _az_RETURN_IF_FAILED(az_json_writer_append_int32(
          ref_json_writer, last_install_result->step_results[i].extended_result_code));

      if (!az_span_is_content_equal(
              last_install_result->step_results[i].result_details, AZ_SPAN_EMPTY))
      {
        // TODO: Add quotes if result_details is not enclosed by quotes.
        _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
            ref_json_writer,
            AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RESULT_DETAILS)));
        _az_RETURN_IF_FAILED(az_json_writer_append_string(
            ref_json_writer, last_install_result->step_results[i].result_details));
      }

      _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));
    }

    _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));
  }

  /* Fill the agent state.   */
  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_STATE)));
  _az_RETURN_IF_FAILED(az_json_writer_append_int32(ref_json_writer, agent_state));

  /* Fill the workflow.  */
  if (workflow != NULL && (az_span_ptr(workflow->id) != NULL && az_span_size(workflow->id) > 0))
  {
    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_WORKFLOW)));
    _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ACTION)));
    _az_RETURN_IF_FAILED(az_json_writer_append_int32(ref_json_writer, workflow->action));

    _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
        ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ID)));
    _az_RETURN_IF_FAILED(az_json_writer_append_string(ref_json_writer, workflow->id));

    /* Append retry timestamp in workflow if existed.  */
    if (!az_span_is_content_equal(workflow->retry_timestamp, AZ_SPAN_EMPTY))
    {
      _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
          ref_json_writer,
          AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_RETRY_TIMESTAMP)));
      _az_RETURN_IF_FAILED(
          az_json_writer_append_string(ref_json_writer, workflow->retry_timestamp));
    }
    _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));
  }

  /* Fill installed update id.  */
  // TODO: Find way to not use internal field
  az_span update_id_string = az_span_slice_to_end(
      ref_json_writer->_internal.destination_buffer,
      az_span_size(az_json_writer_get_bytes_used_in_destination(ref_json_writer))
          + UPDATE_ID_ESCAPING_CHARS_LENGTH);

  if (az_span_is_content_equal(update_id_string, AZ_SPAN_EMPTY))
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  _az_RETURN_IF_FAILED(az_json_writer_append_property_name(
      ref_json_writer,
      AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INSTALLED_UPDATE_ID)));
  _az_RETURN_IF_FAILED(az_json_writer_append_json_text(
      ref_json_writer, generate_update_id_string(device_properties->update_id, update_id_string)));

  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));

  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_end_component(NULL, ref_json_writer));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_adu_client_parse_service_properties(
    az_iot_adu_client* client,
    az_json_reader* ref_json_reader,
    az_span buffer,
    az_iot_adu_client_update_request* update_request,
    az_span* buffer_remainder)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_reader);
  _az_PRECONDITION_VALID_SPAN(buffer, 1, false);
  _az_PRECONDITION_NOT_NULL(update_request);

  (void)client;

  int32_t required_size;
  int32_t out_length;

  RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME);
  RETURN_IF_JSON_TOKEN_NOT_TEXT(ref_json_reader, AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SERVICE);

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_BEGIN_OBJECT);
  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  update_request->workflow.action = 0;
  update_request->workflow.id = AZ_SPAN_EMPTY;
  update_request->workflow.retry_timestamp = AZ_SPAN_EMPTY;
  update_request->update_manifest = AZ_SPAN_EMPTY;
  update_request->update_manifest_signature = AZ_SPAN_EMPTY;
  update_request->file_urls_count = 0;

  while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME);

    if (az_json_token_is_text_equal(
            &ref_json_reader->token,
            AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_WORKFLOW)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME);

        if (az_json_token_is_text_equal(
                &ref_json_reader->token,
                AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ACTION)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
          _az_RETURN_IF_FAILED(
              az_json_token_get_int32(&ref_json_reader->token, &update_request->workflow.action));
        }
        else if (az_json_token_is_text_equal(
                     &ref_json_reader->token,
                     AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_ID)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

          required_size = ref_json_reader->token.size + NULL_TERM_CHAR_SIZE;

          _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

          update_request->workflow.id = split_az_span(buffer, required_size, &buffer);

          _az_RETURN_IF_FAILED(az_json_token_get_string(
              &ref_json_reader->token,
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

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      }
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_UPDATE_MANIFEST)))
    {
      int32_t update_manifest_length;

      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      _az_RETURN_IF_FAILED(az_json_token_get_string(
          &ref_json_reader->token,
          (char*)az_span_ptr(buffer),
          az_span_size(buffer),
          &update_manifest_length));

      // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the end!!!!!!)
      //       Preferably have a function that does not copy anything.
      // TODO: optmize the memory usage for update_manifest:
      //       Here we are copying the entire update manifest [originally escaped] json into
      //       update_request->update_manifest. Later az_iot_adu_client_parse_update_manifest
      //       parses that json into a az_iot_adu_client_update_manifest structure, by simply
      //       mapping the values of update_request->update_manifest. Option 1: there seems to be no
      //       workaround for update_request->update_manifest for copying with
      //                 az_json_token_get_string, since the original update manifest comes as an
      //                 escaped json. What can be done is to make it temporary, and parse the
      //                 update manifest within az_iot_adu_client_parse_service_request, saving only
      //                 the update manifest values in the (then) provided buffer.
      //       Option 2: Have a function in azure SDK core that can parse an escaped json, allowing
      //       us to
      //                 avoid copying the update manifest at all.
      update_request->update_manifest = split_az_span(buffer, update_manifest_length, &buffer);
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_UPDATE_MANIFEST_SIGNATURE)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      required_size = ref_json_reader->token.size + NULL_TERM_CHAR_SIZE;

      _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

      update_request->update_manifest_signature = split_az_span(buffer, required_size, &buffer);

      _az_RETURN_IF_FAILED(az_json_token_get_string(
          &ref_json_reader->token,
          (char*)az_span_ptr(update_request->update_manifest_signature),
          az_span_size(update_request->update_manifest_signature),
          &out_length));

      // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the end!!!!!!)
      //       Preferably have a function that does not copy anything.
      update_request->update_manifest_signature
          = az_span_slice(update_request->update_manifest_signature, 0, out_length);
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILEURLS)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_PROPERTY_NAME);

        required_size = ref_json_reader->token.size + NULL_TERM_CHAR_SIZE;

        _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

        update_request->file_urls[update_request->file_urls_count].id
            = split_az_span(buffer, required_size, &buffer);

        _az_RETURN_IF_FAILED(az_json_token_get_string(
            &ref_json_reader->token,
            (char*)az_span_ptr(update_request->file_urls[update_request->file_urls_count].id),
            az_span_size(update_request->file_urls[update_request->file_urls_count].id),
            &out_length));

        // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the
        // end!!!!!!)
        //       Preferably have a function that does not copy anything.
        update_request->file_urls[update_request->file_urls_count].id = az_span_slice(
            update_request->file_urls[update_request->file_urls_count].id, 0, out_length);

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
        RETURN_IF_JSON_TOKEN_NOT_TYPE(ref_json_reader, AZ_JSON_TOKEN_STRING);

        required_size = ref_json_reader->token.size + NULL_TERM_CHAR_SIZE;

        _az_RETURN_IF_NOT_ENOUGH_SIZE(buffer, required_size);

        update_request->file_urls[update_request->file_urls_count].url
            = split_az_span(buffer, required_size, &buffer);

        _az_RETURN_IF_FAILED(az_json_token_get_string(
            &ref_json_reader->token,
            (char*)az_span_ptr(update_request->file_urls[update_request->file_urls_count].url),
            az_span_size(update_request->file_urls[update_request->file_urls_count].url),
            &out_length));

        // TODO: find a way to get rid of az_json_token_get_string (which adds a \0 at the
        // end!!!!!!)
        //       Preferably have a function that does not copy anything.
        update_request->file_urls[update_request->file_urls_count].url = az_span_slice(
            update_request->file_urls[update_request->file_urls_count].url, 0, out_length);

        update_request->file_urls_count++;

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      }
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  }

  if (buffer_remainder != NULL)
  {
    *buffer_remainder = buffer;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_adu_client_get_service_properties_response(
    az_iot_adu_client* client,
    int32_t version,
    int32_t status,
    az_json_writer* ref_json_writer)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_writer);

  (void)client;

  // Component and response status
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_begin_component(
      NULL, ref_json_writer, AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_COMPONENT_NAME)));
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_begin_response_status(
      NULL,
      ref_json_writer,
      AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SERVICE),
      status,
      version,
      AZ_SPAN_EMPTY));

  // It is not necessary to send the properties back in the acknowledgement.
  // We opt not to send them to reduce the size of the payload.
  _az_RETURN_IF_FAILED(az_json_writer_append_begin_object(ref_json_writer));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));

  _az_RETURN_IF_FAILED(
      az_iot_hub_client_properties_writer_end_response_status(NULL, ref_json_writer));
  _az_RETURN_IF_FAILED(az_iot_hub_client_properties_writer_end_component(NULL, ref_json_writer));
  _az_RETURN_IF_FAILED(az_json_writer_append_end_object(ref_json_writer));

  return AZ_OK;
}

AZ_NODISCARD az_result az_iot_adu_client_parse_update_manifest(
    az_iot_adu_client* client,
    az_json_reader* ref_json_reader,
    az_iot_adu_client_update_manifest* update_manifest)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(ref_json_reader);
  _az_PRECONDITION_NOT_NULL(update_manifest);

  (void)client;

  // Initialize the update_manifest with empty values.
  update_manifest->manifest_version = AZ_SPAN_EMPTY;
  update_manifest->update_id.name = AZ_SPAN_EMPTY;
  update_manifest->update_id.provider = AZ_SPAN_EMPTY;
  update_manifest->update_id.version = AZ_SPAN_EMPTY;
  update_manifest->instructions.steps_count = 0;
  update_manifest->files_count = 0;
  update_manifest->create_date_time = AZ_SPAN_EMPTY;

  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
  RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
  _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

  while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
  {
    RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

    bool property_parsed = true;

    if (az_json_token_is_text_equal(
            &ref_json_reader->token,
            AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_MANIFEST_VERSION)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
      update_manifest->manifest_version = ref_json_reader->token.slice;
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INSTRUCTIONS)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

      if (az_json_token_is_text_equal(
              &ref_json_reader->token,
              AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_STEPS)))
      {
        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
        RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_ARRAY);
        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

        update_manifest->instructions.steps_count = 0;

        while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_ARRAY)
        {
          uint32_t step_index = update_manifest->instructions.steps_count;

          RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

          while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
          {
            RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

            if (az_json_token_is_text_equal(
                    &ref_json_reader->token,
                    AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_HANDLER)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);

              update_manifest->instructions.steps[step_index].handler
                  = ref_json_reader->token.slice;
            }
            else if (az_json_token_is_text_equal(
                         &ref_json_reader->token,
                         AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILES)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_ARRAY);
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

              update_manifest->instructions.steps[step_index].files_count = 0;

              while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_ARRAY)
              {
                uint32_t file_index = update_manifest->instructions.steps[step_index].files_count;

                RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);

                update_manifest->instructions.steps[step_index].files[file_index]
                    = ref_json_reader->token.slice;
                update_manifest->instructions.steps[step_index].files_count++;

                _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              }
            }
            else if (az_json_token_is_text_equal(
                         &ref_json_reader->token,
                         AZ_SPAN_FROM_STR(
                             AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_HANDLER_PROPERTIES)))
            {
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

              if (az_json_token_is_text_equal(
                      &ref_json_reader->token,
                      AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_INSTALLED_CRITERIA)))
              {
                _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
                RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
                update_manifest->instructions.steps[step_index]
                    .handler_properties.installed_criteria
                    = ref_json_reader->token.slice;
                _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
                RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_END_OBJECT);
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

            _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
          }

          update_manifest->instructions.steps_count++;

          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
        }

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
        RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_END_OBJECT);
      }
      else
      {
        // TODO: log unexpected property.
        return AZ_ERROR_JSON_INVALID_STATE;
      }
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_UPDATE_ID)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

        if (az_json_token_is_text_equal(
                &ref_json_reader->token,
                AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_PROVIDER)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
          RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
          update_manifest->update_id.provider = ref_json_reader->token.slice;
        }
        else if (az_json_token_is_text_equal(
                     &ref_json_reader->token,
                     AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_NAME)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
          RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
          update_manifest->update_id.name = ref_json_reader->token.slice;
        }
        else if (az_json_token_is_text_equal(
                     &ref_json_reader->token,
                     AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_VERSION)))
        {
          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
          RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
          update_manifest->update_id.version = ref_json_reader->token.slice;
        }
        else
        {
          // TODO: log unexpected property.
          return AZ_ERROR_JSON_INVALID_STATE;
        }

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      }
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_COMPATIBILITY)))
    {
      /*
       * According to ADU design, the ADU service compatibility properties
       * are not intended to be consumed by the ADU agent.
       * To save on processing, the properties are not being exposed.
       */
      _az_RETURN_IF_FAILED(az_json_reader_skip_children(ref_json_reader));
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILES)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

      while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
      {
        uint32_t files_index = update_manifest->files_count;

        RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

        update_manifest->files[files_index].id = ref_json_reader->token.slice;

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
        RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

        while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
        {
          RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);

          if (az_json_token_is_text_equal(
                  &ref_json_reader->token,
                  AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_FILE_NAME)))
          {
            _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
            RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
            update_manifest->files[files_index].file_name = ref_json_reader->token.slice;
          }
          else if (az_json_token_is_text_equal(
                       &ref_json_reader->token,
                       AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_SIZE_IN_BYTES)))
          {
            _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
            RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_NUMBER);

            _az_RETURN_IF_FAILED(az_json_token_get_uint32(
                &ref_json_reader->token, &update_manifest->files[files_index].size_in_bytes));
          }
          else if (az_json_token_is_text_equal(
                       &ref_json_reader->token,
                       AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_HASHES)))
          {
            _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
            RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_BEGIN_OBJECT);
            _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));

            update_manifest->files[files_index].hashes_count = 0;

            while (ref_json_reader->token.kind != AZ_JSON_TOKEN_END_OBJECT)
            {
              uint32_t hashes_count = update_manifest->files[files_index].hashes_count;

              RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_PROPERTY_NAME);
              update_manifest->files[files_index].hashes[hashes_count].hash_type
                  = ref_json_reader->token.slice;
              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
              RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
              update_manifest->files[files_index].hashes[hashes_count].hash_value
                  = ref_json_reader->token.slice;

              update_manifest->files[files_index].hashes_count++;

              _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
            }
          }
          else
          {
            return AZ_ERROR_JSON_INVALID_STATE;
          }

          _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
        }

        update_manifest->files_count++;

        _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      }
    }
    else if (az_json_token_is_text_equal(
                 &ref_json_reader->token,
                 AZ_SPAN_FROM_STR(AZ_IOT_ADU_CLIENT_AGENT_PROPERTY_NAME_CREATED_DATE_TIME)))
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      RETURN_IF_JSON_TOKEN_NOT_TYPE((ref_json_reader), AZ_JSON_TOKEN_STRING);
      update_manifest->create_date_time = ref_json_reader->token.slice;
    }
    else
    {
      property_parsed = false;
    }

    if (!property_parsed)
    {
      _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
      _az_RETURN_IF_FAILED(az_json_reader_skip_children(ref_json_reader));
    }

    _az_RETURN_IF_FAILED(az_json_reader_next_token(ref_json_reader));
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
