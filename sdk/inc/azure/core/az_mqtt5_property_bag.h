// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief This header defines the MQTT 5 property bag and its related methods. The following
 * structures and API are used by the SDK when sending and receiving MQTT5 properties.
 *
 * @note A property bag struct contains the required memory structures to store the MQTT 5
 * properties based on the MQTT 5 stack implementation.
 *
 * @note All API implementations must return az_result.
 * We recommend using `_az_RESULT_MAKE_ERROR(_az_FACILITY_IOT_MQTT, mqtt_stack_ret);` if compatible.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT5_PROPERTY_BAG_H
#define _az_MQTT5_PROPERTY_BAG_H

#include <azure/core/az_config.h>
#include <azure/core/az_log.h>
#include <azure/core/az_result.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(TRANSPORT_MOSQUITTO)
#include <azure/platform/az_mqtt5_mosquitto.h>
#elif defined(TRANSPORT_PAHO)
#include <azure/platform/az_mqtt5_pahoasync.h>
#else
#include <azure/platform/az_mqtt5_notransport.h>
#endif

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT 5 property types.
 *
 * @note Only property types used by the SDK are defined.
 *
 */
typedef enum
{
  /// Payload format indicator property type.
  AZ_MQTT5_PROPERTY_TYPE_PAYLOAD_FORMAT_INDICATOR = 1,

  /// Message expiry interval property type.
  AZ_MQTT5_PROPERTY_TYPE_MESSAGE_EXPIRY_INTERVAL = 2,

  /// Content type property type (usually UTF-8 MIME type)
  AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE = 3,

  /// Response topic property type.
  AZ_MQTT5_PROPERTY_TYPE_RESPONSE_TOPIC = 8,

  /// Correlation data property type.
  AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA = 9,

  /// Subscription identifier property type.
  AZ_MQTT5_PROPERTY_TYPE_SUBSCRIPTION_IDENTIFIER = 11,

  /// Session expiry interval property type.
  AZ_MQTT5_PROPERTY_TYPE_SESSION_EXPIRY_INTERVAL = 17,

  /// Assigned client identifier property type.
  AZ_MQTT5_PROPERTY_TYPE_ASSIGNED_CLIENT_IDENTIFIER = 18,

  /// Server keep alive property type.
  AZ_MQTT5_PROPERTY_TYPE_SERVER_KEEP_ALIVE = 19,

  /// Authentication method property type.
  AZ_MQTT5_PROPERTY_TYPE_AUTHENTICATION_METHOD = 21,

  /// Authentication data property type.
  AZ_MQTT5_PROPERTY_TYPE_AUTHENTICATION_DATA = 22,

  /// Request problem information property type.
  AZ_MQTT5_PROPERTY_TYPE_REQUEST_PROBLEM_INFORMATION = 23,

  /// Will delay interval property type.
  AZ_MQTT5_PROPERTY_TYPE_WILL_DELAY_INTERVAL = 24,

  /// Request response information property type.
  AZ_MQTT5_PROPERTY_TYPE_REQUEST_RESPONSE_INFORMATION = 25,

  /// Response information property type.
  AZ_MQTT5_PROPERTY_TYPE_RESPONSE_INFORMATION = 26,

  /// Server reference property type.
  AZ_MQTT5_PROPERTY_TYPE_SERVER_REFERENCE = 28,

  /// Reason string property type.
  AZ_MQTT5_PROPERTY_TYPE_REASON_STRING = 31,

  /// Receive maximum property type.
  AZ_MQTT5_PROPERTY_TYPE_RECEIVE_MAXIMUM = 33,

  /// Topic alias maximum property type.
  AZ_MQTT5_PROPERTY_TYPE_TOPIC_ALIAS_MAXIMUM = 34,

  /// Topic alias property type.
  AZ_MQTT5_PROPERTY_TYPE_TOPIC_ALIAS = 35,

  /// Maximum QoS property type.
  AZ_MQTT5_PROPERTY_TYPE_MAXIMUM_QOS = 36,

  /// Retain available property type.
  AZ_MQTT5_PROPERTY_TYPE_RETAIN_AVAILABLE = 37,

  /// User property property type.
  AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY = 38,

  /// Maximum packet size property type.
  AZ_MQTT5_PROPERTY_TYPE_MAXIMUM_PACKET_SIZE = 39,

  /// Wildcard subscription available property type.
  AZ_MQTT5_PROPERTY_TYPE_WILDCARD_SUBSCRIPTION_AVAILABLE = 40,

  /// Subscription identifier available property type.
  AZ_MQTT5_PROPERTY_TYPE_SUBSCRIPTION_IDENTIFIER_AVAILABLE = 41,

  /// Shared subscription available property type.
  AZ_MQTT5_PROPERTY_TYPE_SHARED_SUBSCRIPTION_AVAILABLE = 42,
} az_mqtt5_property_type;

/**
 * @brief Creates a property string with the given string value.
 *
 * @param[in] str The #az_span that defines the property string value.
 * @return An #az_mqtt5_property_string.
 */
AZ_NODISCARD AZ_INLINE az_mqtt5_property_string az_mqtt5_property_string_create(az_span str)
{
  return (az_mqtt5_property_string){ .str = str };
}

/**
 * @brief An empty #az_mqtt5_property_string literal.
 *
 */
#define AZ_MQTT5_PROPERTY_STRING_LITERAL_EMPTY \
  {                                            \
    .str = AZ_SPAN_EMPTY                       \
  }

/**
 * @brief An empty #az_mqtt5_property_string.
 *
 */
#define AZ_MQTT5_PROPERTY_STRING_EMPTY \
  (az_mqtt5_property_string) AZ_MQTT5_PROPERTY_STRING_LITERAL_EMPTY

/**
 * @brief Creates a property string pair with the given key and value.
 *
 * @param[in] key The #az_span that defines the property name.
 * @param[in] value The #az_span that defines the property value.
 * @return An #az_mqtt5_property_stringpair.
 */
AZ_NODISCARD AZ_INLINE az_mqtt5_property_stringpair
az_mqtt5_property_stringpair_create(az_span key, az_span value)
{
  return (az_mqtt5_property_stringpair){ .key = key, .value = value };
}

/**
 * @brief An empty #az_mqtt5_property_stringpair literal.
 *
 */
#define AZ_MQTT5_PROPERTY_STRINGPAIR_LITERAL_EMPTY \
  {                                                \
    .key = AZ_SPAN_EMPTY, .value = AZ_SPAN_EMPTY   \
  }

/**
 * @brief An empty #az_mqtt5_property_stringpair.
 *
 */
#define AZ_MQTT5_PROPERTY_STRINGPAIR_EMPTY \
  (az_mqtt5_property_stringpair) AZ_MQTT5_PROPERTY_STRINGPAIR_LITERAL_EMPTY

/**
 * @brief Creates a property binary data with the given binary data value.
 *
 * @param[in] bindata The #az_span that defines the property binary value.
 * @return An #az_mqtt5_property_binarydata.
 */
AZ_NODISCARD AZ_INLINE az_mqtt5_property_binarydata
az_mqtt5_property_binarydata_create(az_span bindata)
{
  return (az_mqtt5_property_binarydata){ .bindata = bindata };
}

/**
 * @brief An empty #az_mqtt5_property_binarydata literal.
 *
 */
#define AZ_MQTT5_PROPERTY_BINARYDATA_LITERAL_EMPTY \
  {                                                \
    .bindata = AZ_SPAN_EMPTY                       \
  }

/**
 * @brief An empty #az_mqtt5_property_binarydata.
 *
 */
#define AZ_MQTT5_PROPERTY_BINARYDATA_EMPTY \
  (az_mqtt5_property_binarydata) AZ_MQTT5_PROPERTY_BINARYDATA_LITERAL_EMPTY

/**
 * @brief Resets the MQTT 5 property bag to its initial state.
 *
 * @param[in] property_bag A pointer to an #az_mqtt5_property_bag instance to reset.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_clear(az_mqtt5_property_bag* property_bag);

/**
 * @brief Appends an MQTT 5 string property to the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param prop_str The MQTT 5 string property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_append_string(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_string* prop_str);

/**
 * @brief Appends an MQTT 5 string pair property to the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param prop_strpair The MQTT 5 string pair property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_append_stringpair(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_stringpair* prop_strpair);

/**
 * @brief Appends an MQTT 5 byte property to the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param prop_byte The MQTT 5 byte property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_append_byte(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t prop_byte);

/**
 * @brief Appends an MQTT 5 integer property to the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param prop_int The MQTT 5 integer property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_append_int(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t prop_int);

/**
 * @brief Appends an MQTT 5 binary data property to the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param prop_bindata The MQTT 5 binary data property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_append_binary(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_binarydata* prop_bindata);

/**
 * @brief Reads an MQTT 5 string property from the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param out_prop_str The output MQTT 5 string property.
 *
 * @note After reading, the property string must be freed using #az_mqtt5_property_free_string when
 * it is no longer needed.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_read_string(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_string* out_prop_str);

/**
 * @brief Finds an MQTT 5 string pair property in the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param key The key of the MQTT 5 string pair property.
 * @param out_prop_strpair The output MQTT 5 string pair property.
 *
 * @note After reading, the property string pair must be freed using
 * #az_mqtt5_property_free_stringpair when it is no longer needed.
 *
 * @note User property is the only MQTT 5 property that uses string pairs. The type argument is kept
 * in the function below to safeguard against future MQTT 5 properties that may use string pairs.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_find_stringpair(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_span key,
    az_mqtt5_property_stringpair* out_prop_strpair);

/**
 * @brief Reads an MQTT 5 byte property from the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param out_prop_byte The output MQTT 5 byte property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_read_byte(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint8_t* out_prop_byte);

/**
 * @brief Reads an MQTT 5 integer property from the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param out_prop_int The output MQTT 5 integer property.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_read_int(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    uint32_t* out_prop_int);

/**
 * @brief Reads an MQTT 5 binary data property from the property bag.
 *
 * @param property_bag The MQTT 5 property bag instance.
 * @param type The MQTT 5 property type.
 * @param out_prop_bindata The output MQTT 5 binary data property.
 *
 * @note After reading, the property binary data must be freed using
 * #az_mqtt5_property_free_binarydata when it is no longer needed.
 *
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_mqtt5_property_bag_read_binarydata(
    az_mqtt5_property_bag* property_bag,
    az_mqtt5_property_type type,
    az_mqtt5_property_binarydata* out_prop_bindata);

/**
 * @brief Gets the string value of an MQTT 5 property string.
 *
 * @param[in] prop_str The MQTT 5 property string.
 *
 * @return The string value of the property.
 */
AZ_NODISCARD az_span az_mqtt5_property_get_string(az_mqtt5_property_string* prop_str);

/**
 * @brief Gets the key of an MQTT 5 property string pair.
 *
 * @param[in] prop_strpair The MQTT 5 property string pair.
 *
 * @return The key of the property.
 */
AZ_NODISCARD az_span
az_mqtt5_property_stringpair_get_key(az_mqtt5_property_stringpair* prop_strpair);

/**
 * @brief Gets the value of an MQTT 5 property string pair.
 *
 * @param[in] prop_strpair The MQTT 5 property string pair.
 *
 * @return The value of the property.
 */
AZ_NODISCARD az_span
az_mqtt5_property_stringpair_get_value(az_mqtt5_property_stringpair* prop_strpair);

/**
 * @brief Gets the binary data value of an MQTT 5 property binary data.
 *
 * @param[in] prop_bindata The MQTT 5 property binary data.
 *
 * @return The binary data value of the property.
 */
AZ_NODISCARD az_span az_mqtt5_property_get_binarydata(az_mqtt5_property_binarydata* prop_bindata);

/**
 * @brief Frees an MQTT 5 string property. Called after value is read from property bag.
 *
 * @param prop_str The MQTT 5 string property.
 *
 * @note This function is only required for MQTT stacks that require memory allocation for the
 * property. Otherwise, this function is a no-op.
 */
void az_mqtt5_property_free_string(az_mqtt5_property_string* prop_str);

/**
 * @brief Frees an MQTT 5 string pair property. Called after value is read from property bag.
 *
 * @param prop_strpair The MQTT 5 string pair property.
 *
 * @note This function is only required for MQTT stacks that require memory allocation for the
 * property. Otherwise, this function is a no-op.
 */
void az_mqtt5_property_free_stringpair(az_mqtt5_property_stringpair* prop_strpair);

/**
 * @brief Frees an MQTT 5 binary data property. Called after value is read from property bag.
 *
 * @param prop_bindata The MQTT 5 binary data property.
 *
 * @note This function is only required for MQTT stacks that require memory allocation for the
 * property. Otherwise, this function is a no-op.
 */
void az_mqtt5_property_free_binarydata(az_mqtt5_property_binarydata* prop_bindata);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT5_PROPERTY_BAG_H
