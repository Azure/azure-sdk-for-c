// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_XML_VALUE_H
#define AZ_XML_VALUE_H

#include <az_span.h>

#include <stdbool.h>

#include <_az_cfg_prefix.h>

typedef enum {
  AZ_XML_VALUE_NONE = 0,
  AZ_XML_VALUE_NULL = 1,
  AZ_XML_VALUE_BOOLEAN = 2,
  AZ_XML_VALUE_NUMBER = 3,
  AZ_XML_VALUE_STRING = 4,
  AZ_XML_VALUE_OBJECT = 5,
  AZ_XML_VALUE_ARRAY = 6,
} az_xml_value_kind;

typedef struct {
  az_xml_value_kind kind;
  union {
    bool boolean;
    az_span string;
    double number;
  } data;
} az_xml_value;

AZ_NODISCARD AZ_INLINE az_xml_value az_xml_value_create_boolean(bool const value) {
  return (az_xml_value){
    .kind = AZ_XML_VALUE_BOOLEAN,
    .data.boolean = value,
  };
}

AZ_NODISCARD AZ_INLINE az_xml_value az_xml_value_create_string(az_span const value) {
  return (az_xml_value){
    .kind = AZ_XML_VALUE_STRING,
    .data.string = value,
  };
}

AZ_NODISCARD AZ_INLINE az_xml_value az_xml_value_create_number(double const value) {
  return (az_xml_value){
    .kind = AZ_XML_VALUE_NUMBER,
    .data.number = value,
  };
}

/**
 * Copies a boolean value to @out from the given XML value.
 *
 * If the XML value is not a boolean then the function returns an error.
 */
AZ_NODISCARD az_result
az_xml_value_get_boolean(az_xml_value const * const self, bool * const out);

/**
 * Copies a string span to @out from the given XML value.
 *
 * If the XML value is not a string then the function returns an error.
 */
AZ_NODISCARD az_result
az_xml_value_get_string(az_xml_value const * const self, az_span * const out);

/**
 * Copies a number to @out from the given XML value.
 *
 * If the XML value is not a number then the function returns an error.
 */
AZ_NODISCARD az_result
az_xml_value_get_number(az_xml_value const * const self, double * const out);

#include <_az_cfg_suffix.h>

#endif
