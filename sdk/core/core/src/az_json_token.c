#include <az_json_token.h>

#include <stdbool.h>

bool az_is_whitespace(char const c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

az_json_progress az_json_progress_whitespace_parse(char const c) {
  if (az_is_whitespace(c)) {
    return az_json_progress_create_none();
  }
  {
    az_json_keyword const keyword = az_json_keyword_parse(az_json_keyword_create_none(), c);
    if (keyword.tag != AZ_JSON_KEYWORD_NONE) {
      return az_json_progress_create_keyword(keyword);
    }
  }
  {
    az_json_string const string = az_json_string_parse(az_json_string_create_none(), c);
    if (string.tag != AZ_JSON_STRING_NONE) {
      return az_json_progress_create_string(string);
    }
  }
  {
    az_json_number const number = az_json_number_parse(az_json_number_create_none(), c);
    if (number.tag != AZ_JSON_NUMBER_NONE) {
      return az_json_progress_create_number(number);
    }
  }
  {
    az_json_symbol const symbol = az_json_symbol_parse(az_json_symbol_create_none(), c);
    if (symbol.tag != AZ_JSON_SYMBOL_NONE) {
      return az_json_progress_create_symbol(symbol);
    }
  }
  return az_json_progress_create_error();
}

az_json_token az_json_token_progress_whitespace_parse(char const c) {
  az_json_progress const progress = az_json_progress_whitespace_parse(c);
  if (progress.tag == AZ_JSON_TOKEN_ERROR) {
    return az_json_token_create_error();
  }
  return az_json_token_create_progress(progress);
}

az_json_token az_json_token_progress_keyword_parse(az_json_keyword const old_keyword, char const c) {
  az_json_keyword const keyword = az_json_keyword_parse(old_keyword, c);
  switch (keyword.tag)
  {
    case AZ_JSON_KEYWORD_ERROR:
      return az_json_token_create_error();
    case AZ_JSON_KEYWORD_DONE:
      return az_json_token_create_keyword(keyword.done);
  }
  return az_json_token_create_progress(az_json_progress_create_keyword(keyword));
}

az_json_token az_json_token_progress_string_parse(az_json_string const old_string, char const c) {
  az_json_string const string = az_json_string_parse(old_string, c);
  switch (string.tag)
  {
    case AZ_JSON_STRING_ERROR:
      return az_json_token_create_error();
    case AZ_JSON_STRING_DONE:
      return az_json_token_create_string(string.done);
  }
  return az_json_token_create_progress(az_json_progress_create_string(string));
}

az_json_token az_json_token_progress_number_parse(az_json_number const old_number, char const c) {
  az_json_number const number = az_json_number_parse(old_number, c);
  switch (number.tag)
  {
    case AZ_JSON_NUMBER_ERROR:
      return az_json_token_create_error();
    case AZ_JSON_NUMBER_DONE:
      return az_json_token_create_number(number.done);
  }
  return az_json_token_create_progress(az_json_progress_create_number(number));
}

az_json_token az_json_token_progress_symbol_parse(az_json_symbol const old_symbol, char const c) {
  az_json_symbol const symbol = az_json_symbol_parse(old_symbol, c);
  switch (symbol.tag)
  {
    case AZ_JSON_SYMBOL_ERROR:
      return az_json_token_create_error();
    case AZ_JSON_SYMBOL_DONE:
      return az_json_token_create_symbol(symbol.done);
  }
  return az_json_token_create_progress(az_json_progress_create_symbol(symbol));
}

az_json_token az_json_token_progress_parse(az_json_progress const progress, char const c) {
  switch (progress.tag) {
    case AZ_JSON_TOKEN_PROGRESS:
      return az_json_token_progress_whitespace_parse(c);
    case AZ_JSON_TOKEN_KEYWORD:
      return az_json_token_progress_keyword_parse(progress.keyword, c);
    case AZ_JSON_TOKEN_STRING:
      return az_json_token_progress_string_parse(progress.string, c);
    case AZ_JSON_TOKEN_NUMBER:
      return az_json_token_progress_number_parse(progress.number, c);
    case AZ_JSON_TOKEN_SYMBOL:
      return az_json_token_progress_symbol_parse(progress.symbol, c);
  }
  return az_json_token_create_error();
}

inline az_json_token az_json_token_done_parse(char const prior, char const c) {
  az_json_token token = az_json_token_progress_parse(az_json_progress_create_none(), prior);
  if (token.tag == AZ_JSON_TOKEN_PROGRESS) {
    return az_json_token_progress_parse(token.progress, c);
  }
  return az_json_token_create_error();
}

az_json_token az_json_token_parse(az_json_token const token, char const c) {
  switch (token.tag) {
    case AZ_JSON_TOKEN_PROGRESS:
      return az_json_token_progress_parse(token.progress, c);
    case AZ_JSON_TOKEN_KEYWORD:
      return az_json_token_done_parse(token.keyword.next, c);
    case AZ_JSON_TOKEN_STRING:
      return az_json_token_done_parse(token.string.next, c);
    case AZ_JSON_TOKEN_NUMBER:
      return az_json_token_done_parse(token.number.next, c);
    case AZ_JSON_TOKEN_SYMBOL:
      return az_json_token_done_parse(token.symbol.next, c);
  }
  return az_json_token_create_error();
}
