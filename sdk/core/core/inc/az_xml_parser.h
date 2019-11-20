// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef AZ_XML_PARSER_H
#define AZ_XML_PARSER_H

#include <az_xml_value.h>
#include <az_result.h>
#include <az_span_reader.h>
#include <az_str.h>

#include <stdbool.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

typedef struct {
  az_span name;
  az_xml_value value;
} az_xml_member;

typedef struct {
  az_span version;
  az_span encoding;
} az_xml_header;

enum { AZ_XML_STACK_SIZE = 63 };

typedef uint64_t az_xml_stack;

typedef enum {
  AZ_XML_STACK_VALUE = 0,
  AZ_XML_STACK_ARRAY = 1,
  AZ_XML_STACK_TAG_BEGIN = 2,
  AZ_XML_STACK_TAG_CLOSE = 3,
  AZ_XML_STACK_EOF = 4,
} az_xml_stack_item;

typedef struct {
  az_span_reader reader;
  az_xml_stack stack;
} az_xml_parser;

AZ_NODISCARD az_xml_parser az_xml_parser_create(az_span const buffer);

AZ_NODISCARD az_result az_xml_parser_read_tag_begin(az_xml_parser * const self, az_span * const tag);

AZ_NODISCARD az_result
az_xml_parser_read_tag_close(az_xml_parser * const self, az_span * const tag);

AZ_NODISCARD az_result az_xml_parser_read_header(az_xml_parser * const self, az_xml_header * const header);

AZ_NODISCARD az_xml_stack_item az_xml_parser_peek_next_item(az_xml_parser * const self);

AZ_NODISCARD bool az_xml_parser_is_tag_close(az_span_reader * const reader);

AZ_NODISCARD az_result
az_xml_parser_read_value(az_xml_parser * const self, az_xml_value * const value);

#include <_az_cfg_suffix.h>

#endif //#define AZ_XML_PARSER_H
