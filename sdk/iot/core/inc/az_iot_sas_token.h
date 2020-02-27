// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_SAS_TOKEN_H
#define _az_IOT_SAS_TOKEN_H

#include <az_result.h>
#include <az_span.h>

#include <stdint.h>

#include <_az_cfg_prefix.h>

/* @brief                         Generates the document to be encrypted and passed to az_iot_sas_token_generate().
 * @param[in]  iothub_fqdn        A #az_span containing the FQDN of the Azure IoT Hub.
 * @param[in]  device_id          A #az_span containing the FQDN of the Azure IoT Hub.
 * @param[in]  expiry_time_secs   The expiry time of the SAS token, in number of seconds since UNIX epoch (1970).
 * @param[in]  document           An #az_span to use as base for `out_document`.
 * @param[out] out_document       A pointer to #az_span where to write the SAS token document to be encrypted.
 * @returns                       An #az_result with the result of the operation.
 *                                #AZ_OK                       If no failures occur. 
 *                                #AZ_ERROR_BUFFER_OVERFLOW    If the buffer provided in #az_iot_sas_params or `document` are insuficient in size.
 *                                #AZ_ERROR_ARG                If `params` is NULL or any of its components are NULL or a NULL #az_span, or 
 *                                                          if `document` is a NULL pointer or a NULL #az_span.
 */
az_result az_iot_sas_token_get_document(az_span iothub_fqdn, az_span device_id, int32_t expiry_time_secs, az_span document, az_span* out_document);

/* @brief                        Generates the SAS token based on the parameters and key (encrypted document).
 * @param[in]  iothub_fqdn       A #az_span containing the FQDN of the Azure IoT Hub.
 * @param[in]  device_id         A #az_span containing the FQDN of the Azure IoT Hub.
 * @param[in]  signature         The signature must be generated following this procedure:
 *                               - Obtain the document using az_iot_sas_token_get_document(),
 *                               - Apply HMAC/SHA256 encryption on the document,
 *                                 @note If using a device key as encryption key, it must be Base64 decoded.
 *                               - Base64 encode, then URL encode the encrypted document.
 * @param[in]  expiry_time_secs   The expiry time of the SAS token, in number of seconds since UNIX epoch (1970).
 * @param[in]  key_name           A #az_span containing the key name to be encoded in the sas token (optional, use AZ_SPAN_NULL if not needed).
 * @param[in]  sas_token          An #az_span to use as base for `out_sas_token`.
 * @param[out] out_sas_token      An pointer to #az_span where to write the SAS token.
 * @returns                       An #az_result with the result of the operation.
 *                                #AZ_OK                       If no failures occur. 
 *                                #AZ_ERROR_BUFFER_OVERFLOW    If the buffer provided in #az_iot_sas_params or `sas_token` are insuficient in size.
 *                                #AZ_ERROR_ARG                If `params` is NULL or any of its components are NULL or a NULL #az_span, or
 *                                                             if `signature` is a NULL #az_span, or
 *                                                             if `document` is a NULL pointer or a NULL #az_span.
 */
az_result az_iot_sas_token_generate(az_span iothub_fqdn, az_span device_id, az_span signature, int32_t expiry_time_secs, az_span key_name, az_span sas_token, az_span* out_sas_token);

#include <_az_cfg_suffix.h>

#endif // _az_IOT_SAS_TOKEN_H
