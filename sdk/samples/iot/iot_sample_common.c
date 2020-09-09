// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4996: 'getenv': This function or variable may be unsafe. Consider using _dupenv_s
// instead.
#pragma warning(disable : 4996)
#endif

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include "iot_sample_common.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#define IOT_SAMPLE_PRECONDITION_NOT_NULL(arg)   \
  do                                            \
  {                                             \
    if ((arg) == NULL)                          \
    {                                           \
      IOT_SAMPLE_LOG_ERROR("Pointer is NULL."); \
      exit(1);                                  \
    }                                           \
  } while (0)

//
// MQTT endpoints
//
//#define USE_WEB_SOCKET // Comment to use MQTT without WebSockets.
#ifdef USE_WEB_SOCKET
static az_span const mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("wss://");
// Note: Paho fails to connect to Hub when using AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH or an X509
// certificate.
static az_span const mqtt_url_suffix
    = AZ_SPAN_LITERAL_FROM_STR(":443" AZ_IOT_HUB_CLIENT_WEB_SOCKET_PATH_NO_X509_CLIENT_CERT);
#else
static az_span const mqtt_url_prefix = AZ_SPAN_LITERAL_FROM_STR("ssl://");
static az_span const mqtt_url_suffix = AZ_SPAN_LITERAL_FROM_STR(":8883");
#endif
static az_span const provisioning_global_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("ssl://global.azure-devices-provisioning.net:8883");

//
// Functions
//
static az_result read_configuration_entry(
    char const* env_name,
    char* default_value,
    bool hide_value,
    az_span destination,
    az_span* out_env_value)
{
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    (void)printf("%s = %s\n", env_name, hide_value ? "***" : env_value);
    az_span env_span = az_span_create_from_str(env_value);

    IOT_SAMPLE_RETURN_IF_NOT_ENOUGH_SIZE(destination, az_span_size(env_span));
    az_span_copy(destination, env_span);
    *out_env_value = az_span_slice(destination, 0, az_span_size(env_span));
  }
  else
  {
    IOT_SAMPLE_LOG_ERROR("(missing) Please set the %s environment variable.", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

az_result iot_sample_read_environment_variables(
    iot_sample_type type,
    iot_sample_name name,
    iot_sample_environment_variables* out_env_vars)
{
  IOT_SAMPLE_PRECONDITION_NOT_NULL(out_env_vars);

  if (type == PAHO_IOT_HUB)
  {
    out_env_vars->hub_hostname = AZ_SPAN_FROM_BUFFER(iot_sample_hub_hostname_buffer);
    IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
        IOT_SAMPLE_ENV_HUB_HOSTNAME,
        NULL,
        false,
        out_env_vars->hub_hostname,
        &(out_env_vars->hub_hostname)));

    switch (name)
    {
      case PAHO_IOT_HUB_C2D_SAMPLE:
      case PAHO_IOT_HUB_METHODS_SAMPLE:
      case PAHO_IOT_HUB_PNP_COMPONENT_SAMPLE:
      case PAHO_IOT_HUB_PNP_SAMPLE:
      case PAHO_IOT_HUB_TELEMETRY_SAMPLE:
      case PAHO_IOT_HUB_TWIN_SAMPLE:
        out_env_vars->hub_device_id = AZ_SPAN_FROM_BUFFER(iot_sample_hub_device_id_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_HUB_DEVICE_ID,
            NULL,
            false,
            out_env_vars->hub_device_id,
            &(out_env_vars->hub_device_id)));

        out_env_vars->x509_cert_pem_file_path
            = AZ_SPAN_FROM_BUFFER(iot_sample_x509_cert_pem_file_path_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_DEVICE_X509_CERT_PEM_FILE_PATH,
            NULL,
            false,
            out_env_vars->x509_cert_pem_file_path,
            &(out_env_vars->x509_cert_pem_file_path)));
        break;

      case PAHO_IOT_HUB_SAS_TELEMETRY_SAMPLE:
        out_env_vars->hub_device_id = AZ_SPAN_FROM_BUFFER(iot_sample_hub_device_id_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_HUB_SAS_DEVICE_ID,
            NULL,
            false,
            out_env_vars->hub_device_id,
            &(out_env_vars->hub_device_id)));

        out_env_vars->hub_sas_key = AZ_SPAN_FROM_BUFFER(iot_sample_hub_sas_key_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_HUB_SAS_KEY,
            NULL,
            true,
            out_env_vars->hub_sas_key,
            &(out_env_vars->hub_sas_key)));

        char duration_buffer[IOT_SAMPLE_SAS_KEY_DURATION_TIME_DIGITS];
        az_span duration = AZ_SPAN_FROM_BUFFER(duration_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_SAS_KEY_DURATION_MINUTES, "120", false, duration, &duration));
        IOT_SAMPLE_RETURN_IF_FAILED(
            az_span_atou32(duration, &(out_env_vars->sas_key_duration_minutes)));
        break;

      default:
        IOT_SAMPLE_LOG_ERROR("Hub sample name undefined.");
        return AZ_ERROR_ARG;
    }
  }
  else if (type == PAHO_IOT_PROVISIONING)
  {
    out_env_vars->provisioning_id_scope
        = AZ_SPAN_FROM_BUFFER(iot_sample_provisioning_id_scope_buffer);
    IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
        IOT_SAMPLE_ENV_PROVISIONING_ID_SCOPE,
        NULL,
        false,
        out_env_vars->provisioning_id_scope,
        &(out_env_vars->provisioning_id_scope)));

    switch (name)
    {
      case PAHO_IOT_PROVISIONING_SAMPLE:
        out_env_vars->provisioning_registration_id
            = AZ_SPAN_FROM_BUFFER(iot_sample_provisioning_registration_id_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_PROVISIONING_REGISTRATION_ID,
            NULL,
            false,
            out_env_vars->provisioning_registration_id,
            &(out_env_vars->provisioning_registration_id)));

        out_env_vars->x509_cert_pem_file_path
            = AZ_SPAN_FROM_BUFFER(iot_sample_x509_cert_pem_file_path_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_DEVICE_X509_CERT_PEM_FILE_PATH,
            NULL,
            false,
            out_env_vars->x509_cert_pem_file_path,
            &(out_env_vars->x509_cert_pem_file_path)));
        break;

      case PAHO_IOT_PROVISIONING_SAS_SAMPLE:
        out_env_vars->provisioning_registration_id
            = AZ_SPAN_FROM_BUFFER(iot_sample_provisioning_registration_id_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_PROVISIONING_SAS_REGISTRATION_ID,
            NULL,
            false,
            out_env_vars->provisioning_registration_id,
            &(out_env_vars->provisioning_registration_id)));

        out_env_vars->provisioning_sas_key
            = AZ_SPAN_FROM_BUFFER(iot_sample_provisioning_sas_key_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_PROVISIONING_SAS_KEY,
            NULL,
            true,
            out_env_vars->provisioning_sas_key,
            &(out_env_vars->provisioning_sas_key)));

        char duration_buffer[IOT_SAMPLE_SAS_KEY_DURATION_TIME_DIGITS];
        az_span duration = AZ_SPAN_FROM_BUFFER(duration_buffer);
        IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
            IOT_SAMPLE_ENV_SAS_KEY_DURATION_MINUTES, "120", false, duration, &duration));
        IOT_SAMPLE_RETURN_IF_FAILED(
            az_span_atou32(duration, &(out_env_vars->sas_key_duration_minutes)));
        break;

      default:
        IOT_SAMPLE_LOG_ERROR("Provisioning sample name undefined.");
        return AZ_ERROR_ARG;
    }
  }
  else
  {
    IOT_SAMPLE_LOG_ERROR("Sample type undefined.");
    return AZ_ERROR_ARG;
  }

  out_env_vars->x509_trust_pem_file_path
      = AZ_SPAN_FROM_BUFFER(iot_sample_x509_trust_pem_file_path_buffer);
  IOT_SAMPLE_RETURN_IF_FAILED(read_configuration_entry(
      IOT_SAMPLE_ENV_DEVICE_X509_TRUST_PEM_FILE_PATH,
      "",
      false,
      out_env_vars->x509_trust_pem_file_path,
      &(out_env_vars->x509_trust_pem_file_path)));

  IOT_SAMPLE_LOG(" "); // Formatting.
  return AZ_OK;
}

az_result iot_sample_create_mqtt_endpoint(
    iot_sample_type type,
    iot_sample_environment_variables const* env_vars,
    char* out_endpoint,
    size_t endpoint_size)
{
  IOT_SAMPLE_PRECONDITION_NOT_NULL(env_vars);
  IOT_SAMPLE_PRECONDITION_NOT_NULL(out_endpoint);

  if (type == PAHO_IOT_HUB)
  {
    int32_t const required_size = az_span_size(mqtt_url_prefix)
        + az_span_size(env_vars->hub_hostname) + az_span_size(mqtt_url_suffix)
        + (int32_t)sizeof('\0');

    if ((size_t)required_size > endpoint_size)
    {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    }

    az_span hub_mqtt_endpoint = az_span_create((uint8_t*)out_endpoint, (int32_t)endpoint_size);
    az_span remainder = az_span_copy(hub_mqtt_endpoint, mqtt_url_prefix);
    remainder = az_span_copy(remainder, env_vars->hub_hostname);
    remainder = az_span_copy(remainder, mqtt_url_suffix);
    az_span_copy_u8(remainder, '\0');
  }
  else if (type == PAHO_IOT_PROVISIONING)
  {
    int32_t const required_size
        = az_span_size(provisioning_global_endpoint) + (int32_t)sizeof('\0');

    if ((size_t)required_size > endpoint_size)
    {
      return AZ_ERROR_NOT_ENOUGH_SPACE;
    }

    az_span provisioning_mqtt_endpoint
        = az_span_create((uint8_t*)out_endpoint, (int32_t)endpoint_size);
    az_span remainder = az_span_copy(provisioning_mqtt_endpoint, provisioning_global_endpoint);
    az_span_copy_u8(remainder, '\0');
  }
  else
  {
    IOT_SAMPLE_LOG_ERROR("Sample type undefined.");
    return AZ_ERROR_ARG;
  }

  IOT_SAMPLE_LOG_SUCCESS("MQTT endpoint created at \"%s\".", out_endpoint);

  return AZ_OK;
}

void iot_sample_sleep_for_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
  return;
}

uint32_t iot_sample_get_epoch_expiration_time_from_minutes(uint32_t minutes)
{
  return (uint32_t)(time(NULL) + minutes * 60);
}

static az_result decode_base64_bytes(
    az_span base64_encoded_bytes,
    az_span decoded_bytes,
    az_span* out_decoded_bytes)
{
  az_result rc;
  BIO* base64_decoder;
  BIO* source_mem_bio;

  memset(az_span_ptr(decoded_bytes), 0, (size_t)az_span_size(decoded_bytes));

  // Create a BIO filter to process the bytes
  base64_decoder = BIO_new(BIO_f_base64());
  if (base64_decoder == NULL)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Get the source BIO to push through the filter
  source_mem_bio
      = BIO_new_mem_buf(az_span_ptr(base64_encoded_bytes), (int)az_span_size(base64_encoded_bytes));
  if (source_mem_bio == NULL)
  {
    BIO_free(base64_decoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Push the memory through the filter
  source_mem_bio = BIO_push(base64_decoder, source_mem_bio);
  if (source_mem_bio == NULL)
  {
    BIO_free(base64_decoder);
    BIO_free(source_mem_bio);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Set flags to not have a newline and close the BIO
  BIO_set_flags(source_mem_bio, BIO_FLAGS_BASE64_NO_NL);
  BIO_set_close(source_mem_bio, BIO_CLOSE);

  // Read the memory which was pushed through the filter
  int read_data = BIO_read(source_mem_bio, az_span_ptr(decoded_bytes), az_span_size(decoded_bytes));

  // Set the output span
  if (read_data > 0)
  {
    *out_decoded_bytes = az_span_create(az_span_ptr(decoded_bytes), (int32_t)read_data);
    rc = AZ_OK;
  }
  else
  {
    rc = AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  // Free the BIO chain
  BIO_free_all(source_mem_bio);

  return rc;
}

static az_result hmac_sha256_sign_signature(
    az_span decoded_key,
    az_span signature,
    az_span signed_signature,
    az_span* out_signed_signature)
{
  az_result rc;

  unsigned int hmac_encode_len;
  unsigned char const* hmac = HMAC(
      EVP_sha256(),
      (void*)az_span_ptr(decoded_key),
      az_span_size(decoded_key),
      az_span_ptr(signature),
      (size_t)az_span_size(signature),
      az_span_ptr(signed_signature),
      &hmac_encode_len);

  if (hmac != NULL)
  {
    *out_signed_signature = az_span_create(az_span_ptr(signed_signature), (int32_t)hmac_encode_len);
    rc = AZ_OK;
  }
  else
  {
    rc = AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  return rc;
}

static az_result base64_encode_bytes(
    az_span decoded_bytes,
    az_span base64_encoded_bytes,
    az_span* out_base64_encoded_bytes)
{
  az_result rc;
  BIO* base64_encoder;
  BIO* sink_mem_bio;
  BUF_MEM* encoded_mem_ptr;

  // Create a BIO filter to process the bytes
  base64_encoder = BIO_new(BIO_f_base64());
  if (base64_encoder == NULL)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Create a memory sink BIO to process bytes to
  sink_mem_bio = BIO_new(BIO_s_mem());
  if (sink_mem_bio == NULL)
  {
    BIO_free(base64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Push the sink to the encoder
  base64_encoder = BIO_push(base64_encoder, sink_mem_bio);
  if (base64_encoder == NULL)
  {
    BIO_free(sink_mem_bio);
    BIO_free(base64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Set no newline flag for the encoder
  BIO_set_flags(base64_encoder, BIO_FLAGS_BASE64_NO_NL);

  // Write the bytes to be encoded
  int const bytes_written
      = BIO_write(base64_encoder, az_span_ptr(decoded_bytes), (int)az_span_size(decoded_bytes));
  if (bytes_written < 1)
  {
    BIO_free(sink_mem_bio);
    BIO_free(base64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Flush the BIO
  BIO_flush(base64_encoder);

  // Get the pointer to the encoded bytes
  BIO_get_mem_ptr(base64_encoder, &encoded_mem_ptr);

  if ((size_t)az_span_size(base64_encoded_bytes) >= encoded_mem_ptr->length)
  {
    // Copy the bytes to the output and initialize output span
    memcpy(az_span_ptr(base64_encoded_bytes), encoded_mem_ptr->data, encoded_mem_ptr->length);
    *out_base64_encoded_bytes
        = az_span_create(az_span_ptr(base64_encoded_bytes), (int32_t)encoded_mem_ptr->length);

    rc = AZ_OK;
  }
  else
  {
    rc = AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  // Free the BIO chain
  BIO_free_all(base64_encoder);

  return rc;
}

void iot_sample_generate_sas_base64_encoded_signed_signature(
    az_span sas_base64_encoded_key,
    az_span sas_signature,
    az_span sas_base64_encoded_signed_signature,
    az_span* out_sas_base64_encoded_signed_signature)
{
  IOT_SAMPLE_PRECONDITION_NOT_NULL(out_sas_base64_encoded_signed_signature);

  int rc;

  // Decode the sas base64 encoded key to use for HMAC signing.
  char sas_decoded_key_buffer[64];
  az_span sas_decoded_key = AZ_SPAN_FROM_BUFFER(sas_decoded_key_buffer);
  if (az_result_failed(
          rc = decode_base64_bytes(sas_base64_encoded_key, sas_decoded_key, &sas_decoded_key)))
  {
    IOT_SAMPLE_LOG_ERROR("Could not decode the SAS key: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // HMAC-SHA256 sign the signature with the decoded key.
  char sas_hmac256_signed_signature_buffer[128];
  az_span sas_hmac256_signed_signature = AZ_SPAN_FROM_BUFFER(sas_hmac256_signed_signature_buffer);
  if (az_result_failed(
          rc = hmac_sha256_sign_signature(
              sas_decoded_key,
              sas_signature,
              sas_hmac256_signed_signature,
              &sas_hmac256_signed_signature)))
  {
    IOT_SAMPLE_LOG_ERROR("Could not sign the signature: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Base64 encode the result of the HMAC signing.
  if (az_result_failed(
          rc = base64_encode_bytes(
              sas_hmac256_signed_signature,
              sas_base64_encoded_signed_signature,
              out_sas_base64_encoded_signed_signature)))
  {
    IOT_SAMPLE_LOG_ERROR("Could not base64 encode the password: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  return;
}
