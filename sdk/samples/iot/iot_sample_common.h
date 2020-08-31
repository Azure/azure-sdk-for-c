// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef IOT_SAMPLE_COMMON_H
#define IOT_SAMPLE_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#define IOT_SAMPLE_SAS_KEY_DURATION_TIME_DIGITS 4
#define IOT_SAMPLE_MQTT_PUBLISH_QOS 0

//
// Logging
//
#define IOT_SAMPLE_LOG_ERROR(...)                                                  \
  do                                                                               \
  {                                                                                \
    (void)fprintf(stderr, "ERROR:\t\t%s:%s():%d: ", __FILE__, __func__, __LINE__); \
    (void)fprintf(stderr, __VA_ARGS__);                                            \
    (void)fprintf(stderr, "\n");                                                   \
    fflush(stdout);                                                                \
    fflush(stderr);                                                                \
  } while (0)

#define IOT_SAMPLE_LOG_SUCCESS(...) \
  do                                \
  {                                 \
    (void)printf("SUCCESS:\t");     \
    (void)printf(__VA_ARGS__);      \
    (void)printf("\n");             \
  } while (0)

#define IOT_SAMPLE_LOG(...)    \
  do                           \
  {                            \
    (void)printf("\t\t");      \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n");        \
  } while (0)

#define IOT_SAMPLE_LOG_AZ_SPAN(span_description, span)                                           \
  do                                                                                             \
  {                                                                                              \
    (void)printf("\t\t%s ", span_description);                                                   \
    (void)fwrite((char*)az_span_ptr(span), sizeof(uint8_t), (size_t)az_span_size(span), stdout); \
    (void)printf("\n");                                                                          \
  } while (0)

//
// Error handling
//
#define IOT_SAMPLE_RETURN_IF_FAILED(exp)        \
  do                                            \
  {                                             \
    az_result const _iot_sample_result = (exp); \
    if (az_result_failed(_iot_sample_result))   \
    {                                           \
      return _iot_sample_result;                \
    }                                           \
  } while (0)

#define IOT_SAMPLE_RETURN_IF_NOT_ENOUGH_SIZE(span, required_size)          \
  do                                                                       \
  {                                                                        \
    int32_t _iot_sample_req_sz = (required_size);                          \
    if (az_span_size(span) < _iot_sample_req_sz || _iot_sample_req_sz < 0) \
    {                                                                      \
      return AZ_ERROR_INSUFFICIENT_SPAN_SIZE;                              \
    }                                                                      \
  } while (0)

//
// Environment Variables
//
// DO NOT MODIFY: Service information
#define IOT_SAMPLE_ENV_HUB_HOSTNAME "AZ_IOT_HUB_HOSTNAME"
#define IOT_SAMPLE_ENV_PROVISIONING_ID_SCOPE "AZ_IOT_PROVISIONING_ID_SCOPE"

// DO NOT MODIFY: Device information
#define IOT_SAMPLE_ENV_HUB_DEVICE_ID "AZ_IOT_HUB_DEVICE_ID"
#define IOT_SAMPLE_ENV_HUB_SAS_DEVICE_ID "AZ_IOT_HUB_SAS_DEVICE_ID"
#define IOT_SAMPLE_ENV_PROVISIONING_REGISTRATION_ID "AZ_IOT_PROVISIONING_REGISTRATION_ID"
#define IOT_SAMPLE_ENV_PROVISIONING_SAS_REGISTRATION_ID "AZ_IOT_PROVISIONING_SAS_REGISTRATION_ID"

// DO NOT MODIFY: SAS Key
#define IOT_SAMPLE_ENV_HUB_SAS_KEY "AZ_IOT_HUB_SAS_KEY"
#define IOT_SAMPLE_ENV_PROVISIONING_SAS_KEY "AZ_IOT_PROVISIONING_SAS_KEY"
#define IOT_SAMPLE_ENV_SAS_KEY_DURATION_MINUTES \
  "AZ_IOT_SAS_KEY_DURATION_MINUTES" // default is 2 hrs.

// DO NOT MODIFY: the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define IOT_SAMPLE_ENV_DEVICE_X509_CERT_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_CERT_PEM_FILE_PATH"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define IOT_SAMPLE_ENV_DEVICE_X509_TRUST_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE_PATH"

char iot_sample_hub_hostname_buffer[128];
char iot_sample_provisioning_id_scope_buffer[16];

char iot_sample_hub_device_id_buffer[64];
char iot_sample_provisioning_registration_id_buffer[256];

char iot_sample_hub_sas_key_buffer[128];
char iot_sample_provisioning_sas_key_buffer[128];

char iot_sample_x509_cert_pem_file_path_buffer[256];
char iot_sample_x509_trust_pem_file_path_buffer[256];

typedef struct
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
} iot_sample_environment_variables;

typedef enum
{
  PAHO_IOT_HUB,
  PAHO_IOT_PROVISIONING
} iot_sample_type;

typedef enum
{
  PAHO_IOT_HUB_C2D_SAMPLE,
  PAHO_IOT_HUB_METHODS_SAMPLE,
  PAHO_IOT_HUB_PNP_COMPONENT_SAMPLE,
  PAHO_IOT_HUB_PNP_SAMPLE,
  PAHO_IOT_HUB_SAS_TELEMETRY_SAMPLE,
  PAHO_IOT_HUB_TELEMETRY_SAMPLE,
  PAHO_IOT_HUB_TWIN_SAMPLE,
  PAHO_IOT_PROVISIONING_SAMPLE,
  PAHO_IOT_PROVISIONING_SAS_SAMPLE
} iot_sample_name;

extern bool is_device_operational;

/*
 * @brief Reads in environment variables set by user for purposes of running sample.
 *
 * @param[in] type The enumerated type of the sample.
 * @param[in] name The enumerated name of the sample.
 * @param[out] out_env_vars A pointer to the struct containing all read-in environment variables.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK All required environment variables successfully read-in.
 * @retval #AZ_ERROR_ARG Sample type or name is undefined, or environment variable is not set.
 * @retval #AZ_ERROR_INSUFFICIENT_SPAN_SIZE Not enough space set aside to store environment
 * variable.
 */
az_result iot_sample_read_environment_variables(
    iot_sample_type type,
    iot_sample_name name,
    iot_sample_environment_variables* out_env_vars);

/*
 * @brief Builds an MQTT endpoint c-string for an Azure IoT Hub or provisioning service.
 *
 * @param[in] type The enumerated type of the sample.
 * @param[in] env_vars A pointer to environment variable struct.
 * @param[out] out_endpoint A pointer to char buffer containing the built c-string.
 * @param[in] endpoint_size The size of the char buffer to be filled.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK MQTT endpoint successfully created.
 * @retval #AZ_ERROR_ARG Sample type is undefined.
 * @retval #AZ_ERROR_INSUFFICIENT_SPAN_SIZE Buffer size is not large enough to hold c-string.
 */
az_result iot_sample_create_mqtt_endpoint(
    iot_sample_type type,
    iot_sample_environment_variables const* env_vars,
    char* out_endpoint,
    size_t endpoint_size);

/*
 * @brief Sleep for given seconds.
 *
 * @param[in] seconds Number of seconds to sleep.
 */
void iot_sample_sleep_for_seconds(uint32_t seconds);

/*
 * @brief Return total seconds passed including supplied minutes.
 *
 * @param[in] minutes Number of minutes to include in total seconds returned.
 * @return Total time in seconds.
 */
uint32_t iot_sample_get_epoch_expiration_time_from_minutes(uint32_t minutes);

/*
 * @brief Generate the base64 encoded and signed signature using HMAC-SHA256 signing.
 *
 * @param[in] sas_base64_encoded_key An #az_span containing the SAS key that will be used for
 * signing.
 * @param[in] sas_signature An #az_span containing the signature.
 * @param[out] sas_base64_encoded_signed_signature An #az_span with sufficient capacity to hold the
 * encoded signed signature.
 * @param[out] out_sas_base64_encoded_signed_signature A pointer to the #az_span containing the
 * encoded signed signature.
 */
void iot_sample_generate_sas_base64_encoded_signed_signature(
    az_span sas_base64_encoded_key,
    az_span sas_signature,
    az_span sas_base64_encoded_signed_signature,
    az_span* out_sas_base64_encoded_signed_signature);

#endif // IOT_SAMPLE_COMMON_H
