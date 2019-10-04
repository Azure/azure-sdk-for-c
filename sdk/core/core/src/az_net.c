// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <az_net.h>
#include <az_static_assert.h>

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <_az_cfg_warn.h>

#ifdef _MSC_VER
// warning C4996: 'strcpy/strcat': This function or variable may be unsafe. Consider using
// strcpy_s/strcat_s instead.
#pragma warning(disable : 4996)
#endif

static inline void delay() {
  int const milliseconds = 1000;
#ifdef _WIN32
  Sleep(milliseconds);
#else
  usleep(milliseconds * 1000);
#endif
}

static char const * alloc_shell_exec(char const * const cmd) {
  assert(cmd != NULL);

  FILE * const cmd_output = _popen(cmd, "r");
  assert(cmd_output != NULL);

  delay();

  size_t output_length
      = 4 * 1024; // trying to determine file size via fseek isn't going to work for the size
                  // >=~1200 bytes for the reason that we are reading from a pipe. So we allocate
                  // just large enough buffer and hope it is sufficient.

  char * result = calloc(output_length, sizeof(char));
  assert(result != NULL);

  fgets(result, (int)output_length, cmd_output);

  _pclose(cmd_output);

  result[strlen(result) - 1] = '\0'; // drop EOL at EOF

  return result;
}

static inline bool should_escape(char c) { return (c == '\"' || c == '\\'); }

static char const * alloc_shell_string_escape(char const * const s) {
  assert(s != NULL);
  size_t result_strlen = 0;

  size_t const input_strlen = strlen(s);
  for (size_t i = 0; i < input_strlen; ++i) {
    result_strlen += should_escape(s[i]) ? 2 : 1;
  }

  char * const result = calloc(result_strlen + 1, sizeof(char));
  assert(result != NULL);
  {
    for (size_t i = 0, j = 0; i < input_strlen; ++i, ++j) {
      char c = s[i];

      if (should_escape(c)) {
        result[j] = '\\';
        ++j;
      }

      assert(j <= result_strlen);
      result[j] = c;
    }
  }

  return result;
}

static inline char const * alloc_concat_3_strings(
    char const * const prefix,
    char const * const body,
    char const * const suffix) {
  size_t result_size = strlen(prefix) + strlen(body) + strlen(suffix);

  char * const result = calloc(result_size + 1, sizeof(char));
  assert(result != NULL);

  strcpy(result, prefix);
  strcat(result, body);
  strcat(result, suffix);

  return result;
}

static inline char const * alloc_shell_exec_powershell(char const * const ps) {
  char const * const escaped_ps = alloc_shell_string_escape(ps);
  char const * const cmd = alloc_concat_3_strings("powershell \"", escaped_ps, "\"");

  char const * const result = alloc_shell_exec(cmd);

  free((void *)escaped_ps);
  free((void *)cmd);

  return result;
}

static inline char const * alloc_c_str(az_const_span const span) {
  AZ_STATIC_ASSERT(sizeof(char) == sizeof(*span.begin));

  char * const result = calloc(span.size + 1, sizeof(char));
  assert(result != NULL);
  memcpy(result, span.begin, span.size * sizeof(char));

  return result;
}

static inline az_result copy_to_span(
    const char * const s,
    az_span const span,
    az_const_span * const out_result) {
  AZ_STATIC_ASSERT(sizeof(char) == sizeof(*span.begin));

  assert(out_result != NULL);
  assert(span.begin == out_result->begin);

  size_t s_strlen = strlen(s);
  if (s_strlen > span.size) {
    return AZ_ERROR_NO_BUFFER_SPACE;
  }

  memcpy((void *)out_result->begin, s, s_strlen * sizeof(char));
  out_result->size = s_strlen;

  return AZ_OK;
}

az_result az_net_uri_escape(
    az_const_span const s,
    az_span const buffer,
    az_const_span * const out_result) {
  if (out_result == NULL) {
    return AZ_ERROR_ARG;
  }

  char const * param = alloc_c_str(s);
  char const * script = alloc_concat_3_strings("[uri]::EscapeDataString(\"", param, "\")");

  char const * escaped = alloc_shell_exec_powershell(script);

  az_result const result = copy_to_span(escaped, buffer, out_result);

  free((void *)param);
  free((void *)script);
  free((void *)escaped);

  return result;
}

az_result az_net_invoke_rest_method(
    az_net_http_method const http_method,
    az_const_span const uri,
    az_const_span const body,
    az_const_span const headers,
    az_span const response_buffer,
    az_const_span * const out_response) {
  if (uri.size < sizeof("https://x")) {
    return AZ_ERROR_ARG;
  }

  char const * http_method_str = NULL;
  switch (http_method) {
    case AZ_NET_HTTP_METHOD_POST:
      http_method_str = "Post";
      break;

    case AZ_NET_HTTP_METHOD_GET:
      http_method_str = "Get";
      break;

    default:
      return AZ_ERROR_NOT_IMPLEMENTED;
  }

  char const * cmd = NULL;
  {
    char const * cmd_part1 = "Invoke-RestMethod -Method ";
    char const * cmd_part2 = " -Uri '";
    char const * cmd_part34_body = "' -Body '";
    char const * cmd_part34_body_end = "'";
    char const * cmd_part34_headers = "' -Headers ";
    char const * cmd_part45_end = " | ConvertTo-Json -Compress";

    bool has_body = (body.size > 0);
    bool has_headers = (headers.size > 0);

    cmd = alloc_concat_3_strings(cmd_part1, http_method_str, cmd_part2);

    {
      char const * next_part = cmd_part45_end;
      if (has_body) {
        next_part = cmd_part34_body;
      } else if (has_headers) {
        next_part = cmd_part34_headers;
      }

      char const * const uri_str = alloc_c_str(uri);
      char const * const cmd_old = cmd;

      cmd = alloc_concat_3_strings(cmd, uri_str, next_part);

      free((void *)cmd_old);
      free((void *)uri_str);
    }

    if (has_body) {
      char const * const body_str = alloc_c_str(body);
      char const * const cmd_old = cmd;

      cmd = alloc_concat_3_strings(cmd, body_str, cmd_part34_body_end);

      free((void *)cmd_old);
      free((void *)body_str);

      {
        char const * const cmd_old2 = cmd;
        cmd = alloc_concat_3_strings(cmd, "", has_headers ? cmd_part34_headers : cmd_part45_end);
        free((void *)cmd_old2);
      }
    }

    if (has_headers) {
      char const * const headers_str = alloc_c_str(headers);
      char const * const cmd_old = cmd;

      cmd = alloc_concat_3_strings(cmd, headers_str, cmd_part45_end);

      free((void *)cmd_old);
      free((void *)headers_str);
    }
  }

  char const * json_str = alloc_shell_exec_powershell(cmd);
  az_result const result = copy_to_span(json_str, response_buffer, out_response);
  free((void *)json_str);

  return result;
}
