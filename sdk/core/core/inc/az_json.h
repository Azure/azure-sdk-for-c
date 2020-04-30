// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_json.h
 *
 * @brief This header defines the types and functions your application uses
 *        to build or parse JSON objects.
 *
 * NOTE: You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_JSON_H
#define _az_JSON_H

#include <az_result.h>
#include <az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

/*
 * @brief Defines symbols for the various kinds of JSON tokens that make up any JSON text.
 */
typedef enum
{
  AZ_JSON_TOKEN_NONE, ///< There is no value (as distinct from #AZ_JSON_TOKEN_NULL).
  AZ_JSON_TOKEN_OBJECT_START, ///< The token kind is the start of a JSON object.
  AZ_JSON_TOKEN_OBJECT_END, ///< The token kind is the end of a JSON object.
  AZ_JSON_TOKEN_ARRAY_START, ///< The token kind is the start of a JSON array.
  AZ_JSON_TOKEN_ARRAY_END, ///< The token kind is the end of a JSON array.
  AZ_JSON_TOKEN_PROPERTY_NAME, ///< The token kind is a JSON property name.
  AZ_JSON_TOKEN_STRING, ///< The token kind is a JSON string.
  AZ_JSON_TOKEN_NUMBER, ///< The token kind is a JSON number.
  AZ_JSON_TOKEN_TRUE, ///< The token kind is the JSON literal `true`.
  AZ_JSON_TOKEN_FALSE, ///< The token kind is the JSON literal `false`.
  AZ_JSON_TOKEN_NULL, ///< The token kind is the JSON literal `null`.
} az_json_token_kind;

typedef struct
{
  struct
  {
    uint64_t az_json_stack;
    int32_t current_depth;
  } _internal;
} _az_json_bit_stack;

/*
 * @brief An az_json_token instance represents a JSON token. The kind field indicates the kind of
 * token and based on the kind, you can access the corresponding field.
 */
typedef struct
{
  az_json_token_kind kind;
  union {
    bool boolean;
    double number;
    az_span string;
    az_span span;
  } _internal;
} az_json_token;

/*
 * @brief az_json_token_null Returns the "null" JSON token.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_null()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_NULL, ._internal = { 0 } };
}

/*
 * @brief az_json_token_boolean Returns a boolean JSON token representing either "true" or "false".
 *
 * @param value A boolean indicating how the az_json_token should be initialized.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_boolean(bool value)
{
  return (az_json_token){
    .kind = value ? AZ_JSON_TOKEN_TRUE : AZ_JSON_TOKEN_FALSE,
    ._internal.boolean = value,
  };
}

/*
 * @brief az_json_token_number returns an az_json_token containing a number.
 *
 * @param value A double indicating how the az_json_token should be initialized.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_number(double value)
{
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_NUMBER,
    ._internal.number = value,
  };
}

/*
 * @brief az_json_token_string returns an az_json_token containing a string.
 *
 * @param value A span over a string indicating how the az_json_token should be initialized.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_string(az_span value)
{
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_STRING,
    ._internal.string = value,
  };
}

/*
 * @brief az_json_token_string returns an az_json_token containing an object.
 *
 * @param value A span over an object indicating how the az_json_token should be initialized.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object(az_span value)
{
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_OBJECT_START,
    ._internal.span = value,
  };
}

/*
 * @brief az_json_token_object_start returns an az_json_token representing the start of an object.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object_start()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_OBJECT_START, ._internal = { 0 } };
}

/*
 * @brief az_json_token_object_end returns an az_json_token representing the end of an object.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object_end()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_OBJECT_END, ._internal = { 0 } };
}

/*
 * @brief az_json_token_array_start returns an az_json_token representing the start of an array.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_array_start()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_ARRAY_START, ._internal = { 0 } };
}

/*
 * @brief az_json_token_array_end returns an az_json_token representing the end of an array.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_array_end()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_ARRAY_END, ._internal = { 0 } };
}

/*
 * @brief az_json_token_get_boolean returns the JSON token's boolean.
 *
 * @param token A pointer to an az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the boolean is returned.<br>
 * AZ_ERROR_ITEM_NOT_FOUND if the kind is not AZ_JSON_TOKEN_BOOLEAN.
 */
AZ_NODISCARD az_result az_json_token_get_boolean(az_json_token const* token, bool* out_value);

/*
 * @brief az_json_token_get_number returns the JSON token's number.
 *
 * @param token A pointer to an az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the number is returned.<br>
 * AZ_ERROR_ITEM_NOT_FOUND if the kind != AZ_JSON_TOKEN_NUMBER.
 */
AZ_NODISCARD az_result az_json_token_get_number(az_json_token const* token, double* out_value);

/*
 * @brief az_json_token_get_string returns the JSON token's string via an az_span.
 *
 * @param token A pointer to an az_json_token instance.
 * @param out_value A pointer to a variable to receive the value.
 * @return AZ_OK if the string is returned.<br>
 * AZ_ERROR_ITEM_NOT_FOUND if the kind != AZ_JSON_TOKEN_STRING.
 */
AZ_NODISCARD az_result az_json_token_get_string(az_json_token const* token, az_span* out_value);

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
    bool write_formatted; // Indicates whether the #az_json_builder should format the JSON output,
                          // which includes indenting nested JSON tokens, adding new lines, and
                          // adding white space between property names and values.
  } _internal;
} az_json_builder_options;

/**
 * @brief Gets the default json builder options which builds minimized JSON (with no extra white
 * space) according to the JSON RFC.
 * @details Call this to obtain an initialized #az_json_builder_options structure that can be
 * modified and passed to #az_json_builder_init.
 *
 * @return The default #az_json_builder_options.
 */
AZ_NODISCARD AZ_INLINE az_json_builder_options az_json_builder_options_default()
{
  az_json_builder_options options = (az_json_builder_options) {
    ._internal = {
      .write_formatted = false,
    },
  };

  return options;
}

/*
 * @brief Provides forward-only, non-cached building of UTF-8 encoded JSON text into the provided
 * buffer.
 * @remarks #az_json_builder builds the text sequentially with no caching and by default adheres to
 * the JSON RFC: https://tools.ietf.org/html/rfc8259.
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

/*
 * @brief Initializes an #az_json_builder which writes JSON text into a buffer.
 *
 * @param[out] json_builder A pointer to an #az_json_builder instance to initialize.
 * @param[in] destination_buffer An #az_span over the byte buffer where the JSON text is to be
 * written.
 * @param[in] az_json_builder_options __[nullable]__ A reference to an #az_json_builder_options
 * structure which defines custom behavior of the #az_json_builder. If `NULL` is passed,
 * `az_json_builder_init()` will use the default options (i.e. #az_json_builder_options_default()).
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the az_json_builder is initialized successfully
 */
AZ_NODISCARD AZ_INLINE az_result az_json_builder_init(
    az_json_builder* json_builder,
    az_span destination_buffer,
    az_json_builder_options const* options)
{
  *json_builder
      = (az_json_builder){ ._internal = {
                               .destination_buffer = destination_buffer,
                               .bytes_written = 0,
                               .need_comma = false,
                               .token_kind = AZ_JSON_TOKEN_NONE,
                               .bit_stack = { 0 },
                               .options
                               = options == NULL ? az_json_builder_options_default() : *options,
                           } };
  return AZ_OK;
}

/*
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
  // TODO: Consider precondition validation of the state before returning.
  return az_span_slice(
      json_builder->_internal.destination_buffer, 0, json_builder->_internal.bytes_written);
}

/*
 * @brief Appends the UTF-8 text value (as a JSON string) into the buffer.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the string value to.
 * @param[in] value The UTF-8 encoded value to be written as a JSON string. The value is escaped
 * before writing.
 *
 * @remarks If \p value is #AZ_SPAN_NULL, the JSON `null` value is written, as if the
 * #az_json_builder_append_null() method was called.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the string value was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_string(az_json_builder* json_builder, az_span value);

// TODO: Consider adding char* overloads to make passing string literals as property names easier.
/*
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

/*
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

// TODO: Consider removing for now until we have complete double formatting and parsing.
/*
 * @brief Appends a double number value.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the number to.
 * @param[in] value The value to be written as a JSON number.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if the number was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_number(az_json_builder* json_builder, double value);

/*
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

/*
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

/*
 * @brief Appends the beginning of a JSON object (i.e. `{`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the start of object to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if object start was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_object_start(az_json_builder* json_builder);

/*
 * @brief Appends the beginning of a JSON array (i.e. `[`).
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the start of array to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if array start was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_array_start(az_json_builder* json_builder);

/*
 * @brief Appends the end of the current JSON object (i.e. `}`) or JSON array (i.e. `]`), depending
 * on which JSON container is currently open.
 *
 * @param[in] json_builder A pointer to an #az_json_builder instance containing the buffer to append
 * the closing character to.
 *
 * @return An #az_result value indicating the result of the operation:
 *         - #AZ_OK if container end was appended successfully
 *         - #AZ_ERROR_INSUFFICIENT_SPAN_SIZE if the buffer is too small
 */
AZ_NODISCARD az_result az_json_builder_append_container_end(az_json_builder* json_builder);

/************************************ JSON PARSER ******************/

typedef uint64_t _az_json_stack;

/*
 * @brief An az_json_parser returns the JSON tokens contained within a JSON buffer.
 */
typedef struct
{
  struct
  {
    az_span reader;
    _az_json_stack stack;
  } _internal;
} az_json_parser;

/*
 * @brief An az_json_token_member represents a JSON element's name and value.
 */
typedef struct
{
  az_span name;
  az_json_token token;
} az_json_token_member;

/*
 * @brief az_json_parser_init initializes an az_json_parser to parse the JSON payload contained
 * within the provided buffer.
 *
 * @param json_parser A pointer to an az_json_parser instance to initialize.
 * @param json_buffer A pointer to a buffer containing the JSON document to parse.
 * @return AZ_OK if the token was appended successfully.<br>
 */
AZ_NODISCARD az_result az_json_parser_init(az_json_parser* json_parser, az_span json_buffer);

/*
 * @brief az_json_parser_parse_token returns the next token in the JSON document.
 *
 * @param json_parser A pointer to an az_json_parser instance containing the JSON to parse.
 * @param out_token A pointer to an az_json_token containing the next parsed JSON token.
 * @return AZ_OK if the token was parsed successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.<br>
 *         AZ_ERROR_ITEM_NOT_FOUND when no more items are found.
 */
AZ_NODISCARD az_result
az_json_parser_parse_token(az_json_parser* json_parser, az_json_token* out_token);

/*
 * @brief az_json_parser_parse_token_member returns the next token member in the JSON document.
 *
 * @param json_parser A pointer to an az_json_parser instance containing the JSON to parse.
 * @param out_token_member A pointer to an az_json_token_member containing the next parsed JSON
 * token member.
 * @return AZ_OK if the token was parsed successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.<br>
 *         AZ_ERROR_ITEM_NOT_FOUND when no more items are found.
 */
AZ_NODISCARD az_result az_json_parser_parse_token_member(
    az_json_parser* json_parser,
    az_json_token_member* out_token_member);

/*
 * @brief az_json_parser_parse_array_item returns the next array item in the JSON document.
 *
 * @param json_parser A pointer to an az_json_parser instance containing the JSON to parse.
 * @param out_token A pointer to an az_json_token containing the next parsed JSON array item.
 * @return AZ_OK if the token was parsed successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.<br>
 *         AZ_ERROR_ITEM_NOT_FOUND when no more items are found.
 */
AZ_NODISCARD az_result
az_json_parser_parse_array_item(az_json_parser* json_parser, az_json_token* out_token);

/*
 * @brief az_json_parser_skip_children parses and skips over any nested JSON elements.
 *
 * @param json_parser A pointer to an az_json_parser instance containing the JSON to parse.
 * @param out_token A pointer to an az_json_token containing the next parsed JSON token.
 * @return AZ_OK if the token was parsed successfully.<br>
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.<br>
 *         AZ_ERROR_ITEM_NOT_FOUND when no more items are found.
 */
AZ_NODISCARD az_result
az_json_parser_skip_children(az_json_parser* json_parser, az_json_token token);

/*
 * @brief  az_json_parser_done validates that there is nothing else to parse in the JSON document.
 *
 * @param json_parser A pointer to an az_json_parser instance containing the JSON that was parsed.
 * @return AZ_OK if the token was parsed completely.<br>
 *         AZ_ERROR_JSON_INVALID_STATE if not all of the JSON document was parsed.
 */
AZ_NODISCARD az_result az_json_parser_done(az_json_parser* json_parser);

/************************************ JSON POINTER ******************/

/*
 * @brief az_json_parse_by_pointer parses a JSON document and returns the az_json_token identified
 * by a JSON pointer.
 *
 * @param json_buffer An az_span over a buffer containing the JSON document to parse.
 * @param json_pointer An az_span over a string containing JSON-pointer syntax (see
 * https://tools.ietf.org/html/rfc6901).
 * @param out_token A pointer to an az_json_token that receives the JSON token.
 * @return AZ_OK if the desired token was found in the JSON document.
 *         AZ_ERROR_EOF when the end of the JSON document is reached.<br>
 *         AZ_ERROR_PARSER_UNEXPECTED_CHAR when an invalid character is detected.<br>
 *         AZ_ERROR_ITEM_NOT_FOUND when no more items are found.
 */
AZ_NODISCARD az_result
az_json_parse_by_pointer(az_span json_buffer, az_span json_pointer, az_json_token* out_token);

#include <_az_cfg_suffix.h>

#endif // _az_JSON_H
