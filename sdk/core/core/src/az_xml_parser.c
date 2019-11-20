// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_xml_parser.h"
#include "az_span_reader_private.h"

#include <az_contract.h>

#include <ctype.h>
#include <math.h>

#include <_az_cfg.h>

AZ_NODISCARD AZ_INLINE bool az_xml_is_white_space(az_result_byte const c) {
  switch (c) {
    case ' ':
    case '\t':
    case '\n':
    case '\r':
      return true;
  }
  return false;
}

AZ_NODISCARD AZ_INLINE bool az_xml_is_e(az_result_byte const c) {
  switch (c) {
    case 'e':
    case 'E':
      return true;
  }
  return false;
}

AZ_NODISCARD AZ_INLINE bool az_xml_parser_stack_is_empty(az_xml_parser const * const self) {
  return self->stack == 1;
}

AZ_NODISCARD AZ_INLINE az_xml_stack_item
az_xml_parser_stack_last(az_xml_parser const * const self) {
  return self->stack & 1;
}

AZ_NODISCARD AZ_INLINE az_result
az_xml_parser_push_stack(az_xml_parser * const self, az_xml_stack const stack) {
  if (self->stack >> AZ_XML_STACK_SIZE != 0) {
    return AZ_ERROR_XML_STACK_OVERFLOW;
  }
  self->stack = (self->stack << 1) | stack;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_xml_stack_pop(az_xml_stack * const self) {
  if (*self <= 1) {
    return AZ_ERROR_XML_INVALID_STATE;
  }
  *self >>= 1;
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result az_xml_parser_pop_stack(az_xml_parser * const self) {
  return az_xml_stack_pop(&self->stack);
}

AZ_NODISCARD az_xml_parser az_xml_parser_create(az_span const buffer) {
  return (az_xml_parser){
    .reader = az_span_reader_create(buffer),
    .stack = 1,
  };
}

static void az_span_reader_skip_xml_white_space(az_span_reader * const self) {
  while (az_xml_is_white_space(az_span_reader_current(self))) {
    az_span_reader_next(self);
  }
}

// 18 decimal digits. 10^18 - 1.
//                        0         1
//                        012345678901234567
#define AZ_DEC_NUMBER_MAX 999999999999999999ull

typedef struct {
  int sign;
  uint64_t value;
  bool remainder;
  int16_t exp;
} az_dec_number;

AZ_NODISCARD static double az_xml_number_to_double(az_dec_number const * p) {
  return p->value * pow(10, p->exp) * p->sign;
}

AZ_NODISCARD az_result
az_xml_parser_read_header(az_xml_parser * const self, az_xml_header * const header) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(header);
  if (self->stack != 1 /* Not at the first stack of the XML*/) {
    return AZ_ERROR_XML_INVALID_STATE;
  }

  az_span_reader * const p_reader = &self->reader;

  // read version
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '<'));
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '"'));
  size_t begin = p_reader->i;
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '"'));
  header->version = az_span_sub(p_reader->span, begin, p_reader->i - 1);

  // read encoding
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '"'));
  begin = p_reader->i;
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '"'));
  header->encoding = az_span_sub(p_reader->span, begin, p_reader->i - 1);

  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '>'));

  return AZ_OK;
}

bool az_xml_parser_is_empty_element_tag(az_xml_parser * const self) {
  bool result = false;
  AZ_CONTRACT_ARG_NOT_NULL(self);
  az_span_reader * const p_reader = &self->reader;
  if (p_reader != NULL) {
    size_t curr_pos = p_reader->i;
    if (az_span_reader_expect_char(p_reader, ' ')) {
      az_span_reader_next(p_reader);
      if (az_span_reader_expect_char(p_reader, '/')) {
        if (az_span_reader_expect_char(p_reader, '>')) {
          result = true;
        }
      }
    }

    // Reset position.
    AZ_RETURN_IF_FAILED(az_span_reader_set_pos(p_reader, curr_pos));
  }
  return result;
}

AZ_NODISCARD az_result
az_xml_parser_read_tag_begin(az_xml_parser * const self, az_span * const tag) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(tag);
  az_span_reader * const p_reader = &self->reader;

  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '<'));
  size_t begin = p_reader->i;
  // Tag should not contain space
  // https://www.w3.org/TR/2008/REC-xml-20081126/#NT-NameChar
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_chars(p_reader, '>', ' '));
  *tag = az_span_sub(p_reader->span, begin, p_reader->i - 1);
  if (!az_xml_parser_is_empty_element_tag(self)) {
    self->stack += 1;
  }
  return AZ_OK;
}

AZ_NODISCARD az_result
az_xml_parser_read_tag_close(az_xml_parser * const self, az_span * const tag) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(tag);
  az_span_reader * const p_reader = &self->reader;

  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '<'));
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '/'));
  size_t begin = p_reader->i;
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '>'));
  *tag = az_span_sub(p_reader->span, begin, p_reader->i - 1);

  self->stack -= 1;
  return AZ_OK;
}

AZ_NODISCARD az_xml_stack_item az_xml_parser_peek_next_item(az_xml_parser* const self) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  az_span_reader * const p_reader = &self->reader;
  az_span_reader_skip_xml_white_space(p_reader);

  az_result_byte c = az_span_reader_peek_next(p_reader);
  switch (c) {
    case '<':
      if (az_xml_parser_is_tag_close(p_reader)) {
        return AZ_XML_STACK_TAG_CLOSE;
      }
      return AZ_XML_STACK_TAG_BEGIN;
    case '\n':
      return AZ_XML_STACK_EOF;
    default:
      return AZ_XML_STACK_VALUE;
  }
}

AZ_NODISCARD bool az_xml_parser_is_tag_close(az_span_reader* const reader) {
  bool result = false;
  if (reader != NULL) {
    size_t curr_pos = reader->i;
    if (az_span_reader_expect_char(reader, '<')) {
      az_span_reader_next(reader);
      if (az_span_reader_expect_char(reader, '/')) {
        result = true;
      }
    }

    //Reset position.
    AZ_RETURN_IF_FAILED(az_span_reader_set_pos(reader, curr_pos));
  }
  return result;
}

AZ_NODISCARD az_result
az_xml_parser_read_value(az_xml_parser * const self, az_xml_value * const value) {
  AZ_CONTRACT_ARG_NOT_NULL(self);
  AZ_CONTRACT_ARG_NOT_NULL(value);
  az_span_reader * const p_reader = &self->reader;

  size_t begin = p_reader->i;
  AZ_RETURN_IF_FAILED(az_span_reader_find_next_char(p_reader, '<'));
  value->kind = AZ_XML_VALUE_STRING;
  value->data.string = az_span_sub(p_reader->span, begin, p_reader->i - 1);
  AZ_RETURN_IF_FAILED(az_span_reader_set_pos(p_reader, p_reader->i - 1));

  return AZ_OK;
}
