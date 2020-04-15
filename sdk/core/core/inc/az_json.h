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
 * @brief az_json_token_kind is an enum defining symbols for the various kinds of JSON tokens.
 */
typedef enum
{
  AZ_JSON_TOKEN_NULL,
  AZ_JSON_TOKEN_BOOLEAN,
  AZ_JSON_TOKEN_NUMBER,
  AZ_JSON_TOKEN_STRING,
  AZ_JSON_TOKEN_OBJECT,
  AZ_JSON_TOKEN_OBJECT_START,
  AZ_JSON_TOKEN_OBJECT_END,
  AZ_JSON_TOKEN_ARRAY_START,
  AZ_JSON_TOKEN_ARRAY_END,
  // AZ_JSON_TOKEN_SPAN represents a token consisting of nested JSON; the JSON parser never returns
  // a token of this type.
  AZ_JSON_TOKEN_SPAN,
} az_json_token_kind;

/*
 * @brief An az_json_token instance represents a JSON token. The kind field indicates the kind of
 * token and based on the kind, you access the corresponding field.
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
    .kind = AZ_JSON_TOKEN_BOOLEAN,
    ._internal.boolean = value,
  };
}

/*
 * @brief az_json_token_number returns a az_json_token containing a number.
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
 * @brief az_json_token_string returns a az_json_token containing a string.
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
 * @brief az_json_token_string returns a az_json_token containing an object.
 *
 * @param value A span over an object indicating how the az_json_token should be initialized.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object(az_span value)
{
  return (az_json_token){
    .kind = AZ_JSON_TOKEN_OBJECT,
    ._internal.span = value,
  };
}

/*
 * @brief az_json_token_object_start returns a az_json_token representing the start of an object.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object_start()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_OBJECT_START, ._internal = { 0 } };
}

/*
 * @brief az_json_token_object_end returns a az_json_token representing the end of an object.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_object_end()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_OBJECT_END, ._internal = { 0 } };
}

/*
 * @brief az_json_token_array_start returns a az_json_token representing the start of an array.
 */
AZ_NODISCARD AZ_INLINE az_json_token az_json_token_array_start()
{
  return (az_json_token){ .kind = AZ_JSON_TOKEN_ARRAY_START, ._internal = { 0 } };
}

/*
 * @brief az_json_token_array_end returns a az_json_token representing the end of an array.
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

/*
 * @brief An az_json_builder allows you to build a JSON object into a buffer.
 */
typedef struct
{
  struct
  {
    az_span json;
    int32_t length;
    bool need_comma;
  } _internal;
} az_json_builder;

/*
 * @brief az_json_builder_init initializes an az_json_builder which writes JSON into a buffer.
 *
 * @param json_builder A pointer to an az_json_builder instance to initialize.
 * @param json_buffer An az_span over the buffer where th JSON object is to be written.
 * @return AZ_OK if the az_json_builder is initialized correctly.
 */
AZ_NODISCARD AZ_INLINE az_result
az_json_builder_init(az_json_builder* json_builder, az_span json_buffer)
{
  *json_builder
      = (az_json_builder){ ._internal = { .json = json_buffer, .length = 0, .need_comma = false } };
  return AZ_OK;
}

/*
 * @brief az_json_builder_span_get returns the az_span containing the final JSON object.
 *
 * @param json_builder A pointer to an az_json_builder instance wrapping the JSON buffer.
 * @return an az_span containing the final JSON object.
 */
AZ_NODISCARD AZ_INLINE az_span az_json_builder_span_get(az_json_builder const* json_builder)
{
  return az_span_slice(json_builder->_internal.json, 0, json_builder->_internal.length);
}

/*
 * @brief az_json_builder_append_token appends an az_json_token to the JSON buffer.
 *
 * @param json_builder A pointer to an az_json_builder instance containing the buffer to append the
 * token to.
 * @param token A pointer to the az_json_token to append.
 * @return AZ_OK if the token was appended successfully.<br>
 * AZ_ERROR_BUFFER_OVERFLOW if the buffer is too small.
 */
AZ_NODISCARD az_result
az_json_builder_append_token(az_json_builder* json_builder, az_json_token token);

/*
 * @brief az_json_builder_append_object appends a JSON to the JSON buffer.
 *
 * @param json_builder A pointer to an az_json_builder instance containing the buffer to append the
 * object to.
 * @param name A span containing the JSON object's name.
 * @param token A pointer to the az_json_token to append as the object's value.
 * @return AZ_OK if the token was appended successfully.<br>
 * AZ_ERROR_BUFFER_OVERFLOW if the buffer is too small.
 */
AZ_NODISCARD az_result
az_json_builder_append_object(az_json_builder* json_builder, az_span name, az_json_token token);

/*
 * @brief az_json_builder_append_array_item appends an array item to the JSON buffer.
 *
 * @param json_builder A pointer to an az_json_builder instance containing the buffer to append the
 * array item to.
 * @param token A pointer to the az_json_token representing the array item.
 * @return AZ_OK if the token was appended successfully.<br>
 * AZ_ERROR_BUFFER_OVERFLOW if the buffer is too small.
 */
AZ_NODISCARD az_result
az_json_builder_append_array_item(az_json_builder* json_builder, az_json_token token);

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
 * within the passed in buffer. JSON buffer.
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
