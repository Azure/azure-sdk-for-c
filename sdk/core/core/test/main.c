#include <az_json.h>

#include <stdio.h>

#define ASSERT(c) \
  do { if(!(c)) { printf("- `%s`: failed\n", #c); } else { fprintf(stderr, "- `%s`: succeeded\n", #c); } while(false);

AZ_CSTR(json, "");
AZ_CSTR(json_ws, "    ");

int main() {
  size_t i = 0;
  az_json_value value;
  ASSERT(az_json_parse_value(json, &i, &value) == AZ_JSON_ERROR_UNEXPECTED_END);
  ASSERT(az_json_parse_value(json_ws, &i, &value) != AZ_JSON_ERROR_UNEXPECTED_END);
  return 0;
}
