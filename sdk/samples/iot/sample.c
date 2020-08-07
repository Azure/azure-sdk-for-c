// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "sample.h"

//
// MQTT endpoints
//
#ifdef USE_WEB_SOCKET
az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("wss://");
// Note: Paho fails to connect to Hub when using AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH or an X509
// certificate.
az_span mqtt_url_suffix
    = AZ_SPAN_LITERAL_FROM_STR(":443" AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT);
#else
//az_span mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
//az_span mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");
#endif
az_span provisioning_global_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("ssl://global.azure-devices-provisioning.net:8883");

//
// Functions
//
az_result read_environment_variables(
    sample_type type,
    sample_name name,
    sample_environment_variables* env_vars)
{
  if (type == PAHO_IOT_HUB)
  {
    env_vars->hub_hostname = AZ_SPAN_FROM_BUFFER(hub_hostname_buffer);
    AZ_RETURN_IF_FAILED(
        read_configuration_entry(ENV_IOT_HUB_HOSTNAME, NULL, false, &(env_vars->hub_hostname)));

    switch (name)
    {
      case PAHO_IOT_HUB_C2D_SAMPLE:
      case PAHO_IOT_HUB_METHODS_SAMPLE:
      case PAHO_IOT_HUB_TELEMETRY_SAMPLE:
      case PAHO_IOT_HUB_TWIN_SAMPLE:

        env_vars->hub_device_id = AZ_SPAN_FROM_BUFFER(hub_device_id_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_HUB_DEVICE_ID, NULL, false, &(env_vars->hub_device_id)));

        env_vars->x509_cert_pem_file_path = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file_path_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_DEVICE_X509_CERT_PEM_FILE_PATH,
            NULL,
            false,
            &(env_vars->x509_cert_pem_file_path)));
        break;

      case PAHO_IOT_HUB_SAS_TELEMETRY_SAMPLE:

        env_vars->hub_device_id = AZ_SPAN_FROM_BUFFER(hub_device_id_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_HUB_DEVICE_ID_SAS, NULL, false, &(env_vars->hub_device_id)));

        env_vars->hub_sas_key = AZ_SPAN_FROM_BUFFER(hub_sas_key_buffer);
        AZ_RETURN_IF_FAILED(
            read_configuration_entry(ENV_IOT_HUB_SAS_KEY, NULL, true, &(env_vars->hub_sas_key)));

        char duration_buffer[SAS_KEY_DURATION_TIME_DIGITS];
        az_span duration = AZ_SPAN_FROM_BUFFER(duration_buffer);
        AZ_RETURN_IF_FAILED(
            read_configuration_entry(ENV_IOT_SAS_KEY_DURATION_MINUTES, "120", false, &duration));
        AZ_RETURN_IF_FAILED(az_span_atou32(duration, &(env_vars->sas_key_duration_minutes)));
        break;

      default:
        LOG_ERROR("Sample name undefined.");
        return AZ_ERROR_ARG;
    }
  }
  else if (type == PAHO_IOT_PROVISIONING)
  {
    env_vars->provisioning_id_scope = AZ_SPAN_FROM_BUFFER(provisioning_id_scope_buffer);
    AZ_RETURN_IF_FAILED(read_configuration_entry(
        ENV_IOT_PROVISIONING_ID_SCOPE, NULL, false, &(env_vars->provisioning_id_scope)));

    switch (name)
    {
      case PAHO_IOT_PROVISIONING_SAMPLE:

        env_vars->provisioning_registration_id
            = AZ_SPAN_FROM_BUFFER(provisioning_registration_id_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_PROVISIONING_REGISTRATION_ID,
            NULL,
            false,
            &(env_vars->provisioning_registration_id)));

        env_vars->x509_cert_pem_file_path = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file_path_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_DEVICE_X509_CERT_PEM_FILE_PATH,
            NULL,
            false,
            &(env_vars->x509_cert_pem_file_path)));
        break;

      case PAHO_IOT_PROVISIONING_SAS_SAMPLE:

        env_vars->provisioning_registration_id
            = AZ_SPAN_FROM_BUFFER(provisioning_registration_id_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_PROVISIONING_REGISTRATION_ID_SAS,
            NULL,
            false,
            &(env_vars->provisioning_registration_id)));

        env_vars->provisioning_sas_key = AZ_SPAN_FROM_BUFFER(provisioning_sas_key_buffer);
        AZ_RETURN_IF_FAILED(read_configuration_entry(
            ENV_IOT_PROVISIONING_SAS_KEY, NULL, true, &(env_vars->provisioning_sas_key)));

        char duration_buffer[SAS_KEY_DURATION_TIME_DIGITS];
        az_span duration = AZ_SPAN_FROM_BUFFER(duration_buffer);
        AZ_RETURN_IF_FAILED(
            read_configuration_entry(ENV_IOT_SAS_KEY_DURATION_MINUTES, "120", false, &duration));
        AZ_RETURN_IF_FAILED(az_span_atou32(duration, &(env_vars->sas_key_duration_minutes)));
        break;

      default:
        LOG_ERROR("Sample name undefined.");
        return AZ_ERROR_ARG;
    }
  }
  else
  {
    LOG_ERROR("Sample type undefined.");
    return AZ_ERROR_ARG;
  }

  env_vars->x509_trust_pem_file_path = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file_path_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH, "", false, &(env_vars->x509_trust_pem_file_path)));

  LOG(" "); // Log formatting
  return AZ_OK;
}

az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span* out_value)
{
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    (void)printf("%s = %s\n", env_name, hide_value ? "***" : env_value);
    az_span env_span = az_span_from_str(env_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(*out_value, az_span_size(env_span));
    az_span_copy(*out_value, env_span);
    *out_value = az_span_slice(
        *out_value, 0, az_span_size(env_span)); // Trim any excess. Important for numbers
  }
  else
  {
    LOG_ERROR("(missing) Please set the %s environment variable.", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

az_result create_mqtt_endpoint(sample_type type, char* endpoint, size_t endpoint_size)
{
  if (type == PAHO_IOT_HUB)
  {
    int32_t hub_hostname_length = (int32_t)strlen(hub_hostname_buffer);
    int32_t required_size = az_span_size(mqtt_url_prefix) + hub_hostname_length
        + az_span_size(mqtt_url_suffix) + (int32_t)sizeof('\0');

    if (required_size > (int32_t)endpoint_size)
    {
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
    }

    az_span hub_mqtt_endpoint = az_span_init((uint8_t*)endpoint, (int32_t)endpoint_size);
    az_span remainder = az_span_copy(hub_mqtt_endpoint, mqtt_url_prefix);
    remainder = az_span_copy(
        remainder, az_span_slice(AZ_SPAN_FROM_BUFFER(hub_hostname_buffer), 0, hub_hostname_length));
    remainder = az_span_copy(remainder, mqtt_url_suffix);
    az_span_copy_u8(remainder, '\0');
  }
  else if (type == PAHO_IOT_PROVISIONING)
  {
    int32_t required_size = az_span_size(provisioning_global_endpoint) + (int32_t)sizeof('\0');

    if (required_size > (int32_t)endpoint_size)
    {
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
    }

    az_span provisioning_mqtt_endpoint = az_span_init((uint8_t*)endpoint, (int32_t)endpoint_size);
    az_span remainder = az_span_copy(provisioning_mqtt_endpoint, provisioning_global_endpoint);
    az_span_copy_u8(remainder, '\0');
    ;
  }
  else
  {
    LOG_ERROR("Sample type undefined.");
    return AZ_ERROR_ARG;
  }

  LOG_SUCCESS("MQTT endpoint created at \"%s\".", endpoint);

  return AZ_OK;
}

void sleep_for_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
  return;
}

uint32_t get_epoch_expiration_time_from_hours(uint32_t hours)
{
  return (uint32_t)(time(NULL) + hours * 60 * 60);
}

az_result sample_base64_decode(az_span base64_encoded, az_span in_span, az_span* out_span)
{
  az_result result;

  BIO* b64_decoder;
  BIO* source_mem_bio;

  memset(az_span_ptr(in_span), 0, (size_t)az_span_size(in_span));

  // Create a BIO filter to process the bytes
  b64_decoder = BIO_new(BIO_f_base64());
  if (b64_decoder == NULL)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Get the source BIO to push through the filter
  source_mem_bio = BIO_new_mem_buf(az_span_ptr(base64_encoded), (int)az_span_size(base64_encoded));
  if (source_mem_bio == NULL)
  {
    BIO_free(b64_decoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Push the memory through the filter
  source_mem_bio = BIO_push(b64_decoder, source_mem_bio);
  if (source_mem_bio == NULL)
  {
    BIO_free(b64_decoder);
    BIO_free(source_mem_bio);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Set flags to not have a newline and close the BIO
  BIO_set_flags(source_mem_bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_set_close(source_mem_bio, BIO_CLOSE);

  // Read the memory which was pushed through the filter
  int read_data = BIO_read(source_mem_bio, az_span_ptr(in_span), az_span_size(in_span));

  // Set the output span
  if (read_data > 0)
  {
    *out_span = az_span_init(az_span_ptr(in_span), (int32_t)read_data);
    result = AZ_OK;
  }
  else
  {
    result = AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  // Free the BIO chain
  BIO_free_all(source_mem_bio);

  return result;
}

az_result sample_base64_encode(az_span bytes, az_span in_span, az_span* out_span)
{
  az_result result;

  BIO* sink_mem_bio;
  BIO* b64_encoder;
  BUF_MEM* encoded_mem_ptr;

  // Create a BIO filter to process the bytes
  b64_encoder = BIO_new(BIO_f_base64());
  if (b64_encoder == NULL)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Create a memory sink BIO to process bytes to
  sink_mem_bio = BIO_new(BIO_s_mem());
  if (sink_mem_bio == NULL)
  {
    BIO_free(b64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Push the sink to the encoder
  b64_encoder = BIO_push(b64_encoder, sink_mem_bio);
  if (b64_encoder == NULL)
  {
    BIO_free(sink_mem_bio);
    BIO_free(b64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Set no newline flag for the encoder
  BIO_set_flags(b64_encoder, BIO_FLAGS_BASE64_NO_NL);

  // Write the bytes to be encoded
  int bytes_written = BIO_write(b64_encoder, az_span_ptr(bytes), (int)az_span_size(bytes));
  if (bytes_written < 1)
  {
    BIO_free(sink_mem_bio);
    BIO_free(b64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Flush the BIO
  BIO_flush(b64_encoder);

  // Get the pointer to the encoded bytes
  BIO_get_mem_ptr(b64_encoder, &encoded_mem_ptr);

  if ((size_t)az_span_size(in_span) >= encoded_mem_ptr->length)
  {
    // Copy the bytes to the output and initialize output span
    memcpy(az_span_ptr(in_span), encoded_mem_ptr->data, encoded_mem_ptr->length);
    *out_span = az_span_init(az_span_ptr(in_span), (int32_t)encoded_mem_ptr->length);

    result = AZ_OK;
  }
  else
  {
    result = AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  // Free the BIO chain
  BIO_free_all(b64_encoder);

  return result;
}

az_result sample_hmac_sha256_sign(az_span key, az_span bytes, az_span in_span, az_span* out_span)
{
  az_result result;

  unsigned int hmac_encode_len;
  unsigned char* hmac = HMAC(
      EVP_sha256(),
      (void*)az_span_ptr(key),
      az_span_size(key),
      az_span_ptr(bytes),
      (size_t)az_span_size(bytes),
      az_span_ptr(in_span),
      &hmac_encode_len);

  if (hmac != NULL)
  {
    *out_span = az_span_init(az_span_ptr(in_span), (int32_t)hmac_encode_len);
    result = AZ_OK;
  }
  else
  {
    result = AZ_ERROR_INSUFFICIENT_SPAN_SIZE;
  }

  return result;
}
