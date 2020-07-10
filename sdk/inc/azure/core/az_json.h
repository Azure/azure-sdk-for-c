// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_json.h
 *
 * @brief This header defines the types and functions your application uses
 *        to read or write JSON objects.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_JSON_H
#define _az_JSON_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Defines symbols for the various kinds of JSON tokens that make up any JSON text.
 */
typedef enum
{
  AZ_JSON_TOKEN_NONE, ///< There is no value (as distinct from #AZ_JSON_TOKEN_NULL).
  AZ_JSON_TOKEN_BEGIN_OBJECT, ///< The token kind is the start of a JSON object.
  AZ_JSON_TOKEN_END_OBJECT, ///< The token kind is the end of a JSON object.
  AZ_JSON_TOKEN_BEGIN_ARRAY, ///< The token kind is the start of a JSON array.
  AZ_JSON_TOKEN_END_ARRAY, ///< The token kind is the end of a JSON array.
  AZ_JSON_TOKEN_PROPERTY_NAME, ///< The token kind is a JSON property name.
  AZ_JSON_TOKEN_STRING, ///< The token kind is a JSON string.
  AZ_JSON_TOKEN_NUMBER, ///< The token kind is a JSON number.
  AZ_JSON_TOKEN_TRUE, ///< The token kind is the JSON literal `true`.
  AZ_JSON_TOKEN_FALSE, ///< The token kind is the JSON literal `false`.
  AZ_JSON_TOKEN_NULL, ///< The token kind is the JSON literal `null`.
} az_json_token_kind;

/**
 * @brief A limited stack used by the #az_json_writer to track state information for validation.
 */
typedef struct
{
  struct
  {
    // This uint64_t container represents a tiny stack to track the state during nested transitions.
    // The first bit represents the state of the current depth (1 == object, 0 == array).
    // Each subsequent bit is the parent / containing type (object or array).
    uint64_t az_json_stack;
    int32_t current_depth;
  } _internal;
} _az_json_bit_stack;

/**
 * @brief Represents a JSON token. The kind field indicates the type of the JSON token and the slice
 * represents the portion of the JSON payload that points to the token value.
 */
typedef struct
{
  az_json_token_kind kind;
  az_span slice;

  struct
  {
    bool string_has_escaped_chars;
  } _internal;
} az_json_token;

/**
 * @brief Returns the JSON token's boolean.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the boolean is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind is not AZ_JSON_TOKEN_BOOLEAN.
 */
AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token const* json_token, bool* out_value);

/**
 * @brief Returns the JSON token's number as a 64-bit unsigned integer.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.<br>
 * AZ_ERROR_UNEXPECTED_CHAR if a non-ASCII digit is found within the token or if it contains
 * a number that would overflow or underflow uint64
 */
AZ_NODISCARD az_result
az_json_token_get_uint64(az_json_token const* json_token, uint64_t* out_value);

/**
 * @brief Returns the JSON token's number as a 32-bit unsigned integer.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.<br>
 * AZ_ERROR_UNEXPECTED_CHAR if a non-ASCII digit is found within the token or if it contains
 * a number that would overflow or underflow uint32
 */
AZ_NODISCARD az_result
az_json_token_get_uint32(az_json_token const* json_token, uint32_t* out_value);

/**
 * @brief Returns the JSON token's number as a 64-bit signed integer.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.<br>
 * AZ_ERROR_UNEXPECTED_CHAR if a non-ASCII digit is found within the token or if it contains
 * a number that would overflow or underflow int64
 */
AZ_NODISCARD az_result az_json_token_get_int64(az_json_token const* json_token, int64_t* out_value);

/**
 * @brief Returns the JSON token's number as a 32-bit signed integer.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.<br>
 * AZ_ERROR_UNEXPECTED_CHAR if a non-ASCII digit is found within the token or if it contains
 * a number that would overflow or underflow int32
 */
AZ_NODISCARD az_result az_json_token_get_int32(az_json_token const* json_token, int32_t* out_value);

/**
 * @brief Returns the JSON token's number as a double.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.
 */
AZ_NODISCARD az_result az_json_token_get_double(az_json_token const* json_token, double* out_value);

/**
 * @brief Returns the JSON token's string after unescaping it, if required.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param destination A pointer to a buffer where the string should be copied into.
 * @param destination_max_size The maximum available space within the buffer referred to by
 * \p destination.
 * @param[out] out_string_length __[nullable]__ Contains the number of bytes written to the \p
 * destination which denote the length of the unescaped string. If `NULL` is passed, the parameter
 * is ignored.
 * @return AZ_OK if the string is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_STRING.<br>
 * AZ_ERROR_INSUFFICIENT_SPAN_SIZE if \p destination does not have enough size.
 */
AZ_NODISCARD az_result az_json_token_get_string(
    az_json_token const* json_token,
    char* destination,
    int32_t destination_max_size,
    int32_t* out_string_length);

/**
 * @brief Determines whether the unescaped JSON token value that the #az_json_token points to is
 * equal to the expected text within the provided byte span by doing a case-sensitive comparison.
 *
 * @param[in] json_token A pointer to an #az_json_token instance containing the JSON string token.
 * @param[in] expected_text The lookup text to compare the token against.
 *
 * @return `true` if the current JSON token value in the JSON source semantically matches the
 * expected lookup text, with the exact casing; otherwise, false.
 *
 * @remarks This operation is only valid for the string and property name token kinds. For all other
 * token kinds, it returns false.
 */
AZ_NODISCARD bool az_json_token_is_text_equal(
    az_json_token const* json_token,
    az_span expected_text);

/************************************ JSON WRITER ******************/

/**
 * @brief Allows the user to define custom behavior when writing JSON using the #az_json_writer.
 *
 */
typedef struct
{
  struct
  {
    // Currently, this is unused, but needed as a placeholder since we can't have an empty struct.
    bool unused;
  } _internal;
} az_json_writer_options;

/**
 * @brief Gets the default json writer options which builds minimized JSON (with no extra white
 * space) according to the JSON RFC.
 * @details Call this to obtain an initialized #az_json_writer_options structure that can be
 * modified and passed to #az_json_writer_init().
 *
 * @return The default #az_json_writer_options.
 */
AZ_NODISCARD AZ_INLINE az_json_writer_options az_json_writer_options_default()
{
  az_json_writer_options options = (az_json_writer_options) {
    ._internal = {
      .unused = false,
    },
  };

  return options;
}

/**
 * @brief Provides forward-only, non-cached writing of UTF-8 encoded JSON text into the provided
 * buffer.
 *
 * @remarks #az_json_writer builds the text sequentially with no caching and by default adheres to
 * the JSON RFC: https://tools.ietf.org/html/rfc8259.
 *
 */
typedef struct
{
  struct
  {
    az_span destination_buffer;
    int32_t bytes_written;
    bool need_comma;
    az_json_token_kind token_kind; // needed for validation, potentially #if/def with preconditions.
    _az_json_bit_stack bit_stack; // needed for validation, potentially #if/def with preconditions.
    az_json_writer_options options;
  } _internal;
} az_json_writer;

/**
 * @brief Initializes an #az_json_writer which writes JSON text into a buffer.
 *
 * @param[out] json_writer A pointer to an #az_json_writer instance to initialize.
 * @param[in] destination_buffer An #az_span over the byte buffer where the JSON text is to be
 * written.
 * @param[in] options __[nullable]__ A reference to an #az_json_writer_options
 * structure which defines custom behavior of the #az_json_writer. If `NULL` is passed, the writer
 * will use the default options (i.e. #az_json_writer_options_default()).
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the az_json_writer is initialized successfully
 */
AZ_NODISCARD az_result az_json_writer_init(
    az_json_writer* json_writer,
    az_span destination_buffer,
    az_json_writer_options const* options);

/**
 * @brief Returns the #az_span containing the JSON text written to the underlying buffer so far.
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance wrapping the destination
 * buffer.
 *
 * @note Do NOT modify or override the contents of the returned #az_span unless you are no longer
 * writing JSON text into it.
 *
 * @return An #az_span containing the JSON text built so far.
 */
AZ_NODISCARD AZ_INLINE az_span az_json_writer_get_json(az_json_writer const* json_writer)
{
  return az_span_slice(
      json_writer->_internal.destination_buffer, 0, json_writer->_internal.bytes_written);
}

/**
 * @brief Appends the UTF-8 text value (as a JSON string) into the buffer.
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the string value to.
 * @param[in] value The UTF-8 encoded value to be written as a JSON string. The value is escaped
 * before writing.
 *
 * @remarks If \p value is #AZ_SPAN_NULL, the empty JSON string value is written (i.e. "").
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the string value was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_writer_append_string(az_json_writer* json_writer, az_span value);

/**
 * @brief Appends the UTF-8 property name (as a JSON string) which is the first part of a name/value
 * pair of a JSON object.
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the property name to.
 * @param[in] name The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the property name was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result
az_json_writer_append_property_name(az_json_writer* json_writer, az_span name);

/**
 * @brief Appends a boolean value (as a JSON literal `true` or `false`).
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the boolean to.
 * @param[in] value The value to be written as a JSON literal true or false.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the boolean was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_writer_append_bool(az_json_writer* json_writer, bool value);

/**
 * @brief Appends an int32_t number value.
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the number to.
 * @param[in] value The value to be written as a JSON number.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the number was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_writer_append_int32(az_json_writer* json_writer, int32_t value);

/**
 * @brief Appends a double number value.
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the number to.
 * @param[in] value The value to be written as a JSON number.
 * @param[in] fractional_digits The number of digits of the \p value to write after the decimal
 * point and truncate the rest.
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the number was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 *         - #AZ_ERROR_NOT_SUPPORTED if the \p value contains an integer component that is too
 * large and would overflow beyond 2^53 - 1
 *
 * @remark Only finite double values are supported. Values such as NAN and INFINITY are not allowed
 * and would lead to invalid JSON being written.
 *
 * @remark Non-significant trailing zeros (after the decimal point) are not written, even if \p
 * fractional_digits is large enough to allow the zero padding.
 *
 * @remark The \p fractional_digits must be between 0 and 15 (inclusive). Any value passed in that
 * is larger will be clamped down to 15.
 */
AZ_NODISCARD az_result
az_json_writer_append_double(az_json_writer* json_writer, double value, int32_t fractional_digits);

/**
 * @brief Appends the JSON literal `null`.
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the `null` literal to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if `null` was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_writer_append_null(az_json_writer* json_writer);

/**
 * @brief Appends the beginning of a JSON object (i.e. `{`).
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the start of object to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if object start was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 *         - #AZ_ERROR_JSON_NESTING_OVERFLOW if the depth of the JSON exceeds the maximum allowed
 *           depth of 64
 */
AZ_NODISCARD az_result az_json_writer_append_begin_object(az_json_writer* json_writer);

/**
 * @brief Appends the beginning of a JSON array (i.e. `[`).
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the start of array to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if array start was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 *         - #AZ_ERROR_JSON_NESTING_OVERFLOW if the depth of the JSON exceeds the maximum allowed
 *           depth of 64
 */
AZ_NODISCARD az_result az_json_writer_append_begin_array(az_json_writer* json_writer);

/**
 * @brief Appends the end of the current JSON object (i.e. `}`).
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the closing character to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if object end was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_writer_append_end_object(az_json_writer* json_writer);

/**
 * @brief Appends the end of the current JSON array (i.e. `]`).
 *
 * @param[in] json_writer A pointer to an #az_json_writer instance containing the buffer to append
 * the closing character to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if array end was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_writer_append_end_array(az_json_writer* json_writer);

/************************************ JSON READER ******************/

/**
 * @brief Allows the user to define custom behavior when reading JSON using the #az_json_reader.
 *
 */
typedef struct
{
  struct
  {
    // Currently, this is unused, but needed as a placeholder since we can't have an empty struct.
    bool unused;
  } _internal;
} az_json_reader_options;

/**
 * @brief Gets the default json reader options which reads the JSON strictly according to the JSON
 * RFC.
 * @details Call this to obtain an initialized #az_json_reader_options structure that can be
 * modified and passed to #az_json_reader_init().
 *
 * @return The default #az_json_reader_options.
 */
AZ_NODISCARD AZ_INLINE az_json_reader_options az_json_reader_options_default()
{
  az_json_reader_options options = (az_json_reader_options) {
    ._internal = {
      .unused = false,
    },
  };

  return options;
}

/**
 * @brief Returns the JSON tokens contained within a JSON buffer, one at a time.
 *
 * @remarks The token field is meant to be used as read-only to return the #az_json_token while
 * reading the JSON. Do NOT modify it.
 */
typedef struct
{
  az_json_token
      token; ///< This read-only field gives access to the current token that the #az_json_reader
             ///< has processed, and it shouldn't be modified by the caller.

  struct
  {
    az_span json_buffer;
    int32_t bytes_consumed;
    bool is_complex_json;
    _az_json_bit_stack bit_stack;
    az_json_reader_options options;
  } _internal;
} az_json_reader;

/**
 * @brief Initializes an #az_json_reader to read the JSON payload contained within the provided
 * buffer.
 *
 * @param[out] json_reader A pointer to an #az_json_reader instance to initialize.
 * @param[in] json_buffer An #az_span over the byte buffer containing the JSON text to read.
 * @param[in] options __[nullable]__ A reference to an #az_json_reader_options
 * structure which defines custom behavior of the #az_json_reader. If `NULL` is passed, the reader
 * will use the default options (i.e. #az_json_reader_options_default()).
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the az_json_reader is initialized successfully
 *         - #AZ_ERROR_EOF if the provided json buffer is empty
 */
AZ_NODISCARD az_result az_json_reader_init(
    az_json_reader* json_reader,
    az_span json_buffer,
    az_json_reader_options const* options);

/**
 * @brief Reads the next token in the JSON text and updates the reader state.
 *
 * @param json_reader A pointer to an #az_json_reader instance containing the JSON to read.
 *
 * @return AZ_OK if the token was read successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_UNEXPECTED_CHAR when an invalid character is detected.
 */
AZ_NODISCARD az_result az_json_reader_next_token(az_json_reader* json_reader);

/**
 * @brief Reads and skips over any nested JSON elements.
 *
 * @param json_reader A pointer to an #az_json_reader instance containing the JSON to read.
 *
 * @return AZ_OK if the children of the current JSON token are skipped successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_UNEXPECTED_CHAR when an invalid character is detected.
 *
 * @remarks If the current token kind is a property name, the reader first moves to the property
 * value. Then, if the token kind is start of an object or array, the reader moves to the matching
 * end object or array. For all other token kinds, the reader doesn't move and returns #AZ_OK.
 */
AZ_NODISCARD az_result az_json_reader_skip_children(az_json_reader* json_reader);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_JSON_H
