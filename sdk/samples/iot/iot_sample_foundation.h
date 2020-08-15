// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef SAMPLE_H
#define SAMPLE_H

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
//warning C4204: nonstandard extension used: non-constant aggregate initializer
#pragma warning(disable : 4204)
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <azure/core/az_json.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_provisioning_client.h>

#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include <paho-mqtt/MQTTClient.h>

#define SAS_KEY_DURATION_TIME_DIGITS 4
#define TIMEOUT_MQTT_DISCONNECT_MS (10 * 1000)

//
// Logging and Pre-condition Check
//
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

#define PRECONDITION_NOT_NULL(arg) \
  { \
    if (arg == NULL) \
    { \
      LOG_ERROR("Pointer is NULL."); \
      exit(1); \
    } \
  }

//
// Environment Variables
//
// DO NOT MODIFY: Service information
#define ENV_IOT_HUB_HOSTNAME "AZ_IOT_HUB_HOSTNAME"
#define ENV_IOT_PROVISIONING_ID_SCOPE "AZ_IOT_PROVISIONING_ID_SCOPE"

// DO NOT MODIFY: Device information
#define ENV_IOT_HUB_DEVICE_ID "AZ_IOT_HUB_DEVICE_ID"
#define ENV_IOT_HUB_DEVICE_ID_SAS "AZ_IOT_HUB_DEVICE_ID_SAS"
#define ENV_IOT_PROVISIONING_REGISTRATION_ID "AZ_IOT_PROVISIONING_REGISTRATION_ID"
#define ENV_IOT_PROVISIONING_REGISTRATION_ID_SAS "AZ_IOT_PROVISIONING_REGISTRATION_ID_SAS"

// DO NOT MODIFY: SAS Key
#define ENV_IOT_HUB_SAS_KEY "AZ_IOT_HUB_SAS_KEY"
#define ENV_IOT_PROVISIONING_SAS_KEY "AZ_IOT_PROVISIONING_SAS_KEY"
#define ENV_IOT_SAS_KEY_DURATION_MINUTES "AZ_IOT_SAS_KEY_DURATION_MINUTES" // default is 2 hrs.

// DO NOT MODIFY: the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define ENV_IOT_DEVICE_X509_CERT_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define ENV_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH"

char hub_hostname_buffer[128];
char provisioning_id_scope_buffer[16];

char hub_device_id_buffer[64];
char provisioning_registration_id_buffer[256];

char hub_sas_key_buffer[128];
char provisioning_sas_key_buffer[128];

char x509_cert_pem_file_path_buffer[256];
char x509_trust_pem_file_path_buffer[256];

typedef struct environment_variables
{
  az_span hub_hostname;
  az_span provisioning_id_scope;
  az_span hub_device_id;
  az_span provisioning_registration_id;
  az_span hub_sas_key;
  az_span provisioning_sas_key;
  uint32_t sas_key_duration_minutes;
  az_span x509_cert_pem_file_path;
  az_span x509_trust_pem_file_path;
} sample_environment_variables;

typedef enum sample_type
{
  PAHO_IOT_HUB,
  PAHO_IOT_PROVISIONING
} sample_type;

typedef enum sample_name
{
  PAHO_IOT_HUB_C2D_SAMPLE,
  PAHO_IOT_HUB_METHODS_SAMPLE,
  PAHO_IOT_HUB_PNP_SAMPLE,
  PAHO_IOT_HUB_SAS_TELEMETRY_SAMPLE,
  PAHO_IOT_HUB_TELEMETRY_SAMPLE,
  PAHO_IOT_HUB_TWIN_SAMPLE,
  PAHO_IOT_PROVISIONING_SAMPLE,
  PAHO_IOT_PROVISIONING_SAS_SAMPLE
} sample_name;

/*
 *
 *
 */
void set_program_start_time();

/*
 * @brief      Reads in environment variables set by user for purposes of running sample.
 *
 * @param[in]  type      Enumerated type of the sample.
 * @param[in]  name      Enumerated name of the sample.
 * @param[out] env_vars  Pointer to struct that will contain all read-in environment variables. May
 * NOT be NULL.
 * @return     AZ_OK if all environment variables required are successfully read and values stored.
 */
az_result read_environment_variables(
    sample_type type,
    sample_name name,
    sample_environment_variables* env_vars);

/*
 * @brief      Reads a single environment variable and stores in an az_span.
 *             Assumes *out_value is a valid span.
 *
 * @param[in]  env_name       Name of environment variable. May NOT be NULL.
 * @param[in]  default_value  Default value if envionment variable has not been set. May be NULL.
 * @param[in]  hide_value     True if value should not be printed to console.
 * @param[out] out_value      Pointer to az_span that will contain the envrionment variable value.
 * May NOT be NULL. az_span's size reflects actual size of content in az_span, not size of buffer
 * used to create az_span.
 * @return     AZ_ERROR_INSUFFICIENT_SPAN_SIZE if buffer size is not large enough to hold
 * environment variable value. AZ_ERROR_ARG if required environment variable not set. Else AZ_OK.
 */
az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span* out_value);

/*
 * @brief      Builds an MQTT endpoint c-string for an Azure IoT Hub or provisioning service.
 *
 * @param[in]  type          Enumerated type of the sample.
 * @param[in]  env_vars      Pointer to environment variable struct. May NOT be NULL.
 * @param[out] endpoint      Pointer to char buffer that will contain the c-string. May
 * NOT be NULL.
 * @param[in]  endpoint_size Size of the char buffer to be filled.

 * @return     AZ_ERROR_INSUFFICIENT_SPAN_SIZE if buffer size is not large enough to hold endpoint
 * c-string. AZ_ERROR_ARG is sample type is undefined. Else AZ_OK.
 */
az_result create_mqtt_endpoint(
    sample_type type,
    const sample_environment_variables* env_vars,
    char* endpoint,
    size_t endpoint_size);

/*
 * @brief      Sleeps for given seconds.
 *
 * @param[in]  seconds   Number of seconds to sleep.
 */
void sleep_for_seconds(uint32_t seconds);

/*
 * @brief      Returns total seconds passed including given minutes.
 *
 * @param[in]  minutes  Number of minutes to include in total seconds returned.
 * @return     Total time in seconds.
 */
uint32_t get_epoch_expiration_time_from_minutes(uint32_t minutes);

/*
 * The following functions serve as an example for fundamental functionality needed to use SAS key
 * authentication. This implementation uses OpenSSL.
 */

/*
 * @brief      Decodes an input az_span from base64 to bytes.
 *             Assumes *out_span is a valid span.
 *
 * @param[in]  base64_encoded   Pointer to az_span to be decoded. May NOT be NULL.
 * @param[out] out_span         Pointer to the az_span that will contain the decoded input. May NOT
 * be NULL.
 * @return
 */
az_result base64_decode(const az_span* base64_encoded, az_span* out_span);

/*
 * @brief      HMAC256 signing of the signature with a decoded key.
 *             Assumes *out_span is a valid span.
 *
 * @param[in]  decoded_key  Pointer to az_span containing the decoded key for signing. May NOT be
 * NULL.
 * @param[in]  signature    Pointer to az_span containing the signature for signing. May NOT be
 * NULL.
 * @param[out] out_span     Pointer to az_span that will contain the HMAC256 signed signature. May
 * NOT be NULL.
 * @return
 */
az_result hmac_sha256_sign(const az_span* decoded_key, const az_span* signature, az_span* out_span);

/*
 * @brief      Encodes an input az_span from bytes to base64.
 *             Assumes *out_span is a valid span.
 *
 * @param[in]  bytes      Pointer to az_span containing the bytes to be encoded. May NOT be NULL.
 * @param[out] out_span   Pointer to az_span that will contain the encoded bytes. May NOT be NULL.
 * @return
 */
az_result base64_encode(const az_span* bytes, az_span* out_span);

/*
 * @brief      Generate the b64 encoded and signed signature using HMAC-SHA256 signing.
 *             Assumes *sas_b64_encoded_hmac256_signed_signature is a valid span.
 *
 * @param[in]  sas_key        Pointer to az_span containing the SAS key that will be used for
 * signing. May NOT be NULL.
 * @param[in]  sas_signature  Pointer to az_span containing the signature. May NOT be NULL.
 * @param[out] sas_b64_encoded_hmac256_signed_signature
 *                            Pointer to az_span that will contain the encoded and signed signature.
 * May NOT be NULL.
 */
void sas_generate_encoded_signed_signature(
    const az_span* sas_key,
    const az_span* sas_signature,
    az_span* sas_b64_encoded_hmac256_signed_signature);

#endif // SAMPLE_H
