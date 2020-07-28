// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_json.h
 *
 * @brief This header defines the types and functions your application uses
 *        to build or parse JSON objects.
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
 * @brief A limited stack used by the #az_json_builder to track state information for validation.
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
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.
 */
AZ_NODISCARD az_result
az_json_token_get_uint64(az_json_token const* json_token, uint64_t* out_value);

/**
 * @brief Returns the JSON token's number as a 32-bit unsigned integer.
 *
 * @param json_token A pointer to an #az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_JSON_INVALID_STATE if the kind != AZ_JSON_TOKEN_NUMBER.
 */
AZ_NODISCARD az_result
az_json_token_get_uint32(az_json_token const* json_token, uint32_t* out_value);

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

/************************************ JSON BUILDER ******************/

/**
 * @brief Allows the user to define custom behavior when building JSON using the #az_json_builder.
 *
 */
typedef struct
{
  struct
  {
    // Currently, this is unused, but needed as a placeholder since we can't have an empty struct.
    bool unused;
  } _internal;
} az_json_builder_options;

/**
 * @brief Gets the default json builder options which builds minimized JSON (with no extra white
 * space) according to the JSON RFC.
 * @details Call this to obtain an initialized #az_json_builder_options structure that can be
 * modified and passed to #az_json_builder_init().
 *
 * @return The default #az_json_builder_options.
 */
AZ_NODISCARD AZ_INLINE az_json_builder_options az_json_builder_options_default()
{
  az_json_builder_options options = (az_json_builder_options) {
    ._internal = {
      .unused = false,
    },
  };

  return options;
}

/**
 * @brief Provides forward-only, non-cached building of UTF-8 encoded JSON text into the provided
 * buffer.
 *
 * @remarks #az_json_builder builds the text sequentially with no caching and by default adheres to
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
    az_json_builder_options options;
  } _internal;
} az_json_builder;

/**
 * @brief Initializes an #az_json_builder which writes JSON text into a buffer.
 *
 * @param[out] json_builder A pointer to an #az_json_builder instance to initialize.
 * @param[in] destination_buffer An #az_span over the byte buffer where the JSON text is to be
 * written.
 * @param[in] options __[nullable]__ A reference to an #az_json_builder_options
 * structure which defines custom behavior of the #az_json_builder. If `NULL` is passed, the builder
 * will use the default options (i.e. #az_json_builder_options_default()).
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the az_json_builder is initialized successfully
 */
AZ_NODISCARD AZ_INLINE az_result az_json_builder_init(
    az_json_builder* json_builder,
    az_span destination_buffer,
    az_json_builder_options const* options)
{
  *json_builder = (az_json_builder){
    ._internal = {
      .destination_buffer = destination_buffer,
      .bytes_written = 0,
      .need_comma = false,
      .token_kind = AZ_JSON_TOKEN_NONE,
      .bit_stack = { 0 },
      .options = options == NULL ? az_json_builder_options_default() : *options,
    },
  };
  return AZ_OK;
}

/**
 * @brief Returns the #az_span containing the JSON text written to the underlying buffer so far.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance wrapping the destination
 * buffer.
 *
 * @note Do NOT modify or override the contents of the returned #az_span unless you are no longer
 * building JSON text into it.
 *
 * @return An #az_span containing the JSON text built so far.
 */
AZ_NODISCARD AZ_INLINE az_span az_json_builder_get_json(az_json_builder const* json_builder)
{
  return az_span_slice(
      json_builder->_internal.destination_buffer, 0, json_builder->_internal.bytes_written);
}

/**
 * @brief Appends the UTF-8 text value (as a JSON string) into the buffer.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
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
AZ_NODISCARD az_result az_json_builder_append_string(az_json_builder* json_builder, az_span value);

/**
 * @brief Appends the UTF-8 property name (as a JSON string) which is the first part of a name/value
 * pair of a JSON object.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the property name to.
 * @param[in] name The UTF-8 encoded property name of the JSON value to be written. The name is
 * escaped before writing.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the property name was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result
az_json_builder_append_property_name(az_json_builder* json_builder, az_span name);

/**
 * @brief Appends a boolean value (as a JSON literal `true` or `false`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the boolean to.
 * @param[in] value The value to be written as a JSON literal true or false.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the boolean was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_bool(az_json_builder* json_builder, bool value);

/**
 * @brief Appends an int32_t number value.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the number to.
 * @param[in] value The value to be written as a JSON number.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the number was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result
az_json_builder_append_int32_number(az_json_builder* json_builder, int32_t value);

/**
 * @brief Appends the JSON literal `null`.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the `null` literal to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if `null` was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_null(az_json_builder* json_builder);

/**
 * @brief Appends the beginning of a JSON object (i.e. `{`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the start of object to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if object start was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 *         - #AZ_ERROR_JSON_NESTING_OVERFLOW if the depth of the JSON exceeds the maximum allowed
 *           depth of 64
 */
AZ_NODISCARD az_result az_json_builder_append_begin_object(az_json_builder* json_builder);

/**
 * @brief Appends the beginning of a JSON array (i.e. `[`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the start of array to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if array start was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 *         - #AZ_ERROR_JSON_NESTING_OVERFLOW if the depth of the JSON exceeds the maximum allowed
 *           depth of 64
 */
AZ_NODISCARD az_result az_json_builder_append_begin_array(az_json_builder* json_builder);

/**
 * @brief Appends the end of the current JSON object (i.e. `}`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the closing character to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if object end was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_end_object(az_json_builder* json_builder);

/**
 * @brief Appends the end of the current JSON array (i.e. `]`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the closing character to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if array end was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_end_array(az_json_builder* json_builder);

/************************************ JSON PARSER ******************/

/**
 * @brief Allows the user to define custom behavior when parsing JSON using the #az_json_parser.
 *
 */
typedef struct
{
  struct
  {
    // Currently, this is unused, but needed as a placeholder since we can't have an empty struct.
    bool unused;
  } _internal;
} az_json_parser_options;

/**
 * @brief Gets the default json parser options which parses the JSON strictly according to the JSON
 * RFC.
 * @details Call this to obtain an initialized #az_json_parser_options structure that can be
 * modified and passed to #az_json_parser_init().
 *
 * @return The default #az_json_parser_options.
 */
AZ_NODISCARD AZ_INLINE az_json_parser_options az_json_parser_options_default()
{
  az_json_parser_options options = (az_json_parser_options) {
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
 * parsing the JSON. Do NOT modify it.
 */
typedef struct
{
  az_json_token
      token; ///< This read-only field gives access to the current token that the #az_json_parser
             ///< has processed, and it shouldn't be modified by the caller.

  struct
  {
    az_span json_buffer;
    int32_t bytes_consumed;
    bool is_complex_json;
    _az_json_bit_stack bit_stack;
    az_json_parser_options options;
  } _internal;
} az_json_parser;

/**
 * @brief Initializes an #az_json_parser to parse the JSON payload contained within the provided
 * buffer.
 *
 * @param[out] json_parser A pointer to an #az_json_parser instance to initialize.
 * @param[in] json_buffer An #az_span over the byte buffer containing the JSON text to parse.
 * @param[in] options __[nullable]__ A reference to an #az_json_parser_options
 * structure which defines custom behavior of the #az_json_parser. If `NULL` is passed, the parser
 * will use the default options (i.e. #az_json_parser_options_default()).
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the az_json_parser is initialized successfully
 *         - #AZ_ERROR_EOF if the provided json buffer is empty
 */
AZ_NODISCARD az_result az_json_parser_init(
    az_json_parser* json_parser,
    az_span json_buffer,
    az_json_parser_options const* options);

/**
 * @brief Reads the next token in the JSON text and updates the parser state.
 *
 * @param json_parser A pointer to an #az_json_parser instance containing the JSON to parse.
 *
 * @return AZ_OK if the token was parsed successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.
 */
AZ_NODISCARD az_result az_json_parser_next_token(az_json_parser* json_parser);

/**
 * @brief Parses and skips over any nested JSON elements.
 *
 * @param json_parser A pointer to an #az_json_parser instance containing the JSON to parse.
 *
 * @return AZ_OK if the children of the current JSON token are skipped successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.
 *
 * @remarks If the current token kind is a property name, the parser first moves to the property
 * value. Then, if the token kind is start of an object or array, the parser moves to the matching
 * end object or array. For all other token kinds, the parser doesn't move and returns #AZ_OK.
 */
AZ_NODISCARD az_result az_json_parser_skip_children(az_json_parser* json_parser);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_JSON_H
