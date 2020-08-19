// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef IOT_SAMPLES_COMMON_H
#define IOT_SAMPLES_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#define SAS_KEY_DURATION_TIME_DIGITS 4
//
// Logging
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
    for (int32_t az_span_i = 0; az_span_i < az_span_size(span); az_span_i++) \
    { \
      putchar(*buffer++); \
    } \
    (void)printf("\n"); \
  }

//
// Environment Variables
//
// DO NOT MODIFY: Service information
#define ENV_IOT_HUB_HOSTNAME "AZ_IOT_HUB_HOSTNAME"
#define ENV_IOT_PROVISIONING_ID_SCOPE "AZ_IOT_PROVISIONING_ID_SCOPE"

// DO NOT MODIFY: Device information
#define ENV_IOT_HUB_DEVICE_ID "AZ_IOT_HUB_DEVICE_ID"
#define ENV_IOT_HUB_SAS_DEVICE_ID "AZ_IOT_HUB_SAS_DEVICE_ID"
#define ENV_IOT_PROVISIONING_REGISTRATION_ID "AZ_IOT_PROVISIONING_REGISTRATION_ID"
#define ENV_IOT_PROVISIONING_SAS_REGISTRATION_ID "AZ_IOT_PROVISIONING_SAS_REGISTRATION_ID"

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

char iot_hub_hostname_buffer[128];
char iot_provisioning_id_scope_buffer[16];

char iot_hub_device_id_buffer[64];
char iot_provisioning_registration_id_buffer[256];

char iot_hub_sas_key_buffer[128];
char iot_provisioning_sas_key_buffer[128];

char iot_x509_cert_pem_file_path_buffer[256];
char iot_x509_trust_pem_file_path_buffer[256];

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
    iot_sample_type type,
    iot_sample_name name,
    iot_sample_environment_variables* env_vars);

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
    iot_sample_type type,
    const iot_sample_environment_variables* env_vars,
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
 * @brief      Generate the b64 encoded and signed signature using HMAC-SHA256 signing.
 *             Assumes *sas_b64_encoded_hmac256_signed_signature is a valid span.
 *
 * @param[in]  sas_key        az_span containing the SAS key that will be used for signing.
 * @param[in]  sas_signature  az_span containing the signature.
 * @param[in]  sas_b64_encoded_destination
 *                            az_span where the encoded and signed signature will be written to.
 * @param[out] sas_b64_encoded_out
 *                            Pointer to az_span that receives the remainder of the destination
 * az_span after the signature has been written. May NOT be NULL.
 */
void sas_generate_encoded_signed_signature(
    az_span sas_key,
    az_span sas_signature,
    az_span sas_b64_encoded_destination,
    az_span* sas_b64_encoded_out);

#endif // IOT_SAMPLES_COMMON_H
