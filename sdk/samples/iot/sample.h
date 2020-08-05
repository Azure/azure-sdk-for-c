// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>


#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include <azure/iot/az_iot_provisioning_client.h>

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

#define TIMEOUT_MQTT_RECEIVE_MS (60 * 1000)
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)


// DO NOT MODIFY: Service information
#define ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT "ssl://global.azure-devices-provisioning.net:8883"
#define ENV_GLOBAL_PROVISIONING_ENDPOINT "AZ_IOT_GLOBAL_PROVISIONING_ENDPOINT"
#define ENV_ID_SCOPE_ENV "AZ_IOT_ID_SCOPE"

// DO NOT MODIFY: Device information
#define ENV_REGISTRATION_ID_ENV "AZ_IOT_REGISTRATION_ID"

// DO NOT MODIFY: the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define ENV_DEVICE_X509_CERT_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define ENV_DEVICE_X509_TRUST_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

// Logging with formatting
#define LOG_ERROR(...) \
  { \
    (void)fprintf(stderr, "ERROR:\t\t%s:%s():%d: ", __FILE__, __func__, __LINE__); \
    (void)fprintf(stderr, __VA_ARGS__); \
    (void)fprintf(stderr, "\n"); \
    fflush(stdout); \
    fflush(stderr); \
  }
#define LOG_SUCCESS(...) \
  { \
    (void)printf("SUCCESS:\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG(...) \
  { \
    (void)printf("\t\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG_AZ_SPAN(span_description, span) \
  { \
    (void)printf("\t\t%s ", span_description); \
    char* buffer = (char*)az_span_ptr(span); \
    for (int32_t i = 0; i < az_span_size(span); i++) \
    { \
      putchar(*buffer++); \
    } \
    (void)printf("\n"); \
  }









static az_result read_environment_variables(
    az_span* global_provisioning_endpoint,
    az_span* id_scope,
    az_span* registration_id);
static az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
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
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else
  {
    LOG_ERROR("(missing) Please set the %s environment variable.", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

static void sleep_for_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
  return;
}

/*
 * This serves as an example for fundamental functionality needed to use SAS key authentication.
 * This implementation uses OpenSSL.
 */
az_result sample_base64_decode(az_span base64_encoded, az_span in_span, az_span* out_span);
// Decode an input span from base64 to bytes
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
  if(source_mem_bio == NULL)
  {
    BIO_free(b64_decoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Push the memory through the filter
  source_mem_bio = BIO_push(b64_decoder, source_mem_bio);
  if(source_mem_bio == NULL)
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

az_result sample_base64_encode(az_span bytes, az_span in_span, az_span* out_span);
// Encode an input span of bytes to base64
az_result sample_base64_encode(az_span bytes, az_span in_span, az_span* out_span)
{
  az_result result;

  BIO* sink_mem_bio;
  BIO* b64_encoder;
  BUF_MEM* encoded_mem_ptr;

  // Create a BIO filter to process the bytes
  b64_encoder = BIO_new(BIO_f_base64());
  if(b64_encoder == NULL)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Create a memory sink BIO to process bytes to
  sink_mem_bio = BIO_new(BIO_s_mem());
  if(sink_mem_bio == NULL)
  {
    BIO_free(b64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Push the sink to the encoder
  b64_encoder = BIO_push(b64_encoder, sink_mem_bio);
  if(b64_encoder == NULL)
  {
    BIO_free(sink_mem_bio);
    BIO_free(b64_encoder);
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  // Set no newline flag for the encoder
  BIO_set_flags(b64_encoder, BIO_FLAGS_BASE64_NO_NL);

  // Write the bytes to be encoded
  int bytes_written = BIO_write(b64_encoder, az_span_ptr(bytes), (int)az_span_size(bytes));
  if(bytes_written < 1)
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

az_result sample_hmac_sha256_sign(az_span key, az_span bytes, az_span in_span, az_span* out_span);
// HMAC256 an input span with an input key
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
