// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_http.h>

#include "az_http_header_validation_private.h"
#include "az_http_private.h"
#include "az_span_private.h"
#include <az_precondition.h>
#include <az_precondition_internal.h>

#include <_az_cfg.h>
#include <ctype.h>

// HTTP Response utility functions

AZ_NODISCARD AZ_INLINE az_result _az_is_char(az_span slice, uint8_t c)
{
  return az_span_ptr(slice)[0] == c ? AZ_OK : AZ_CONTINUE;
}

static AZ_NODISCARD az_result _az_valid_header_name_to_colon(az_span slice)
{
  az_result is_colon_result = _az_is_char(slice, ':');
  if (is_colon_result == AZ_OK)
  {
    return is_colon_result;
  }

  if (!az_http_valid_token[az_span_ptr(slice)[0]])
  {
    return AZ_ERROR_HTTP_CORRUPT_RESPONSE_HEADER;
  }

  return is_colon_result;
}
static AZ_NODISCARD az_result _az_is_new_line(az_span slice) { return _az_is_char(slice, '\n'); }

static AZ_NODISCARD bool _az_is_http_whitespace(uint8_t c)
{
  switch (c)
  {
    case ' ':
    case '\t':
      return true;
      ;
    default:
      return false;
  }
}

static AZ_NODISCARD az_result _az_slice_is_not_http_whitespace(az_span slice)
{
  return _az_is_http_whitespace(az_span_ptr(slice)[0]) == true ? AZ_CONTINUE : AZ_OK;
}

/* PRIVATE Function. parse next  */
static AZ_NODISCARD az_result _az_get_digit(az_span* self, uint8_t* save_here)
{

  uint8_t c_ptr = az_span_ptr(*self)[0];
  if (!isdigit(c_ptr))
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }
  //
  *save_here = (uint8_t)(c_ptr - '0');

  // move reader after the expected digit (means it was parsed as expected)
  *self = az_span_slice_to_end(*self, 1);

  return AZ_OK;
}

/**
 * Status line https://tools.ietf.org/html/rfc7230#section-3.1.2
 * HTTP-version SP status-code SP reason-phrase CRLF
 */
static AZ_NODISCARD az_result
_az_get_http_status_line(az_span* self, az_http_response_status_line* out_status_line)
{

  // HTTP-version = HTTP-name "/" DIGIT "." DIGIT
  // https://tools.ietf.org/html/rfc7230#section-2.6
  az_span const start = AZ_SPAN_FROM_STR("HTTP/");
  az_span const dot = AZ_SPAN_FROM_STR(".");
  az_span const space = AZ_SPAN_FROM_STR(" ");

  // parse and move reader if success
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, start));
  AZ_RETURN_IF_FAILED(_az_get_digit(self, &out_status_line->major_version));
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, dot));
  AZ_RETURN_IF_FAILED(_az_get_digit(self, &out_status_line->minor_version));

  // SP = " "
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, space));

  // status-code = 3DIGIT
  {
    uint64_t code = 0;
    AZ_RETURN_IF_FAILED(az_span_atou64(az_span_init(az_span_ptr(*self), 3), &code));
    out_status_line->status_code = (az_http_status_code)code;
    // move reader
    *self = az_span_slice_to_end(*self, 3);
  }

  // SP
  AZ_RETURN_IF_FAILED(_az_is_expected_span(self, space));
  // get a pointer to read response until end of reason-phrase is found
  // reason-phrase = *(HTAB / SP / VCHAR / obs-text)
  // HTAB = "\t"
  // VCHAR or obs-text is %x21-FF,
  int32_t offset = 0;
  AZ_RETURN_IF_FAILED(_az_span_scan_until(*self, _az_is_new_line, &offset));

  // save reason-phrase in status line now that we got the offset. Remove 1 last chars(\r)
  out_status_line->reason_phrase = az_span_slice(*self, 0, offset - 1);
  // move position of reader after reason-phrase (parsed done)
  *self = az_span_slice_to_end(*self, offset + 1);
  // CR LF
  // AZ_RETURN_IF_FAILED(_az_is_expected_span(response, AZ_SPAN_FROM_STR("\r\n")));

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_get_status_line(
    az_http_response* response,
    az_http_response_status_line* out_status_line)
{
  _az_PRECONDITION_NOT_NULL(response);
  _az_PRECONDITION_NOT_NULL(out_status_line);

  // Restart parser to the beggining
  response->_internal.parser.remaining = response->_internal.http_response;

  // read an HTTP status line.
  AZ_RETURN_IF_FAILED(
      _az_get_http_status_line(&response->_internal.parser.remaining, out_status_line));

  // set state.kind of the next HTTP response value.
  response->_internal.parser.next_kind = _az_HTTP_RESPONSE_KIND_HEADER;

  return AZ_OK;
}

AZ_NODISCARD az_result
az_http_response_get_next_header(az_http_response* response, az_pair* out_header)
{
  _az_PRECONDITION_NOT_NULL(response);
  _az_PRECONDITION_NOT_NULL(out_header);
  az_span* reader = &response->_internal.parser.remaining;
  {
    _az_http_response_kind const kind = response->_internal.parser.next_kind;
    // if reader is expecting to read body (all headers were read), return ITEM_NOT_FOUND so we
    // know we reach end of headers
    if (kind == _az_HTTP_RESPONSE_KIND_BODY)
    {
      return AZ_ERROR_ITEM_NOT_FOUND;
    }
    // Can't read a header if status line was not previously called,
    // User needs to call az_http_response_status_line() which would reset parser and set kind to
    // headers
    if (kind != _az_HTTP_RESPONSE_KIND_HEADER)
    {
      return AZ_ERROR_HTTP_INVALID_STATE;
    }
  }

  // check if we are at the end of all headers to change state to Body.
  // We keep state to Headers if current char is not '\r' (there is another header)
  if (az_span_ptr(response->_internal.parser.remaining)[0] == '\r')
  {
    AZ_RETURN_IF_FAILED(_az_is_expected_span(reader, AZ_SPAN_FROM_STR("\r\n")));
    response->_internal.parser.next_kind = _az_HTTP_RESPONSE_KIND_BODY;
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // https://tools.ietf.org/html/rfc7230#section-3.2
  // header-field   = field-name ":" OWS field-value OWS
  // field-name     = token
  {
    int32_t field_name_length = 0;
    // https://tools.ietf.org/html/rfc7230#section-3.2.6
    // token = 1*tchar
    // tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." / "^" /
    //         "_" / "`" / "|" / "~" / DIGIT / ALPHA;
    // any VCHAR,
    //    except delimiters
    AZ_RETURN_IF_FAILED(
        _az_span_scan_until(*reader, _az_valid_header_name_to_colon, &field_name_length));

    // form a header name. Reader is currently at char ':'
    out_header->key = az_span_slice(*reader, 0, field_name_length);
    // update reader to next position after colon (add one)
    *reader = az_span_slice_to_end(*reader, field_name_length + 1);

    // Remove white spaces from header name https://github.com/Azure/azure-sdk-for-c/issues/604
    out_header->key = _az_span_trim_white_space(out_header->key);

    // OWS -> remove the optional white spaces before header value
    int32_t ows_len = 0;
    AZ_RETURN_IF_FAILED(_az_span_scan_until(*reader, _az_slice_is_not_http_whitespace, &ows_len));
    *reader = az_span_slice_to_end(*reader, ows_len);
  }
  // field-value    = *( field-content / obs-fold )
  // field-content  = field-vchar [ 1*( SP / HTAB ) field-vchar ]
  // field-vchar    = VCHAR / obs-text
  //
  // obs-fold       = CRLF 1*( SP / HTAB )
  //                ; obsolete line folding
  //                ; see Section 3.2.4
  //
  // Note: obs-fold is not implemented.
  {
    int32_t offset = 0;
    int32_t offset_value_end = offset;
    while (true)
    {
      uint8_t c = az_span_ptr(*reader)[offset];
      offset += 1;
      if (c == '\r')
      {
        break; // break as soon as end of value char is found
      }
      if (_az_is_http_whitespace(c))
      {
        continue; // white space or tab is accepted. It can be any number after value (OWS)
      }
      if (c <= ' ')
      {
        return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
      }
      offset_value_end = offset; // increasing index only for valid chars,
    }
    out_header->value = az_span_slice(*reader, 0, offset_value_end);
    // moving reader. It is currently after \r was found
    *reader = az_span_slice_to_end(*reader, offset);

    // Remove white spaces from value https://github.com/Azure/azure-sdk-for-c/issues/604
    out_header->value = _az_span_trim_white_space_from_end(out_header->value);
  }

  AZ_RETURN_IF_FAILED(_az_is_expected_span(reader, AZ_SPAN_FROM_STR("\n")));

  return AZ_OK;
}

AZ_NODISCARD az_result az_http_response_get_body(az_http_response* response, az_span* out_body)
{
  _az_PRECONDITION_NOT_NULL(response);
  _az_PRECONDITION_NOT_NULL(out_body);

  // Make sure get body works no matter where is the current parsing. Allow users to call get body
  // directly and ignore headers and status line
  _az_http_response_kind current_parsing_section = response->_internal.parser.next_kind;
  if (current_parsing_section != _az_HTTP_RESPONSE_KIND_BODY)
  {
    if (current_parsing_section == _az_HTTP_RESPONSE_KIND_EOF
        || current_parsing_section == _az_HTTP_RESPONSE_KIND_STATUS_LINE)
    {
      // Reset parser and get status line
      az_http_response_status_line ignore = { 0 };
      AZ_RETURN_IF_FAILED(az_http_response_get_status_line(response, &ignore));
      // update current parsing section
      current_parsing_section = response->_internal.parser.next_kind;
    }
    // parse any remaining header
    if (current_parsing_section == _az_HTTP_RESPONSE_KIND_HEADER)
    {
      // Parse and ignore all remaining headers
      for (az_pair h; az_http_response_get_next_header(response, &h) == AZ_OK;)
      {
        // ignoring header
      }
    }
  }

  // take all the remaining content from reader as body
  *out_body = az_span_slice_to_end(response->_internal.parser.remaining, 0);

  response->_internal.parser.next_kind = _az_HTTP_RESPONSE_KIND_EOF;
  return AZ_OK;
}

void _az_http_response_reset(az_http_response* http_response)
{
  // never fails, discard the result
  // init will set written to 0 and will use the same az_span. Internal parser's state is also
  // reset
  az_result result = az_http_response_init(http_response, http_response->_internal.http_response);
  (void)result;
}

// internal function to get az_http_response remainder
static az_span _az_http_response_get_remaining(az_http_response const* response)
{
  return az_span_slice_to_end(response->_internal.http_response, response->_internal.written);
}

AZ_NODISCARD az_result az_http_response_append(az_http_response* response, az_span source)
{
  _az_PRECONDITION_NOT_NULL(response);

  az_span remaining = _az_http_response_get_remaining(response);
  int32_t write_size = az_span_size(source);
  AZ_RETURN_IF_NOT_ENOUGH_SIZE(remaining, write_size);

  remaining = az_span_copy(remaining, source);
  response->_internal.written += write_size;

  return AZ_OK;
}
