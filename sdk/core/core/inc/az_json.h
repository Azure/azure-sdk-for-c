#include "az_core.h"

typedef enum {
  AZ_JSON_TYPE_NULL = 0,
  AZ_JSON_TYPE_BOOLEAN = 1,
  AZ_JSON_TYPE_NUMBER = 3,
  AZ_JSON_TYPE_STRING = 4,
  AZ_JSON_TYPE_OBJECT = 5,
  AZ_JSON_TYPE_ARRAY = 6,
} AZ_JSON_TYPE;

struct AZ_JSON;
struct AZ_JSON_PROPERTY;

DEFINE_SLICE(struct AZ_JSON_PROPERTY, AZ_JSON_OBJECT);
DEFINE_SLICE(struct AZ_JSON, AZ_JSON_ARRAY);

typedef struct AZ_JSON {
  AZ_JSON_TYPE type;
  union {
    // AZ_JSON_BOOL
    bool boolean;
    // AZ_JSON_NUMBER
    double number;
    // AZ_JSON_STRING
    STRING string;
    // AZ_JSON_OBJECT
    AZ_JSON_OBJECT object;
    // AZ_JSON_ARRAY
    AZ_JSON_ARRAY array;
  };
} AZ_JSON;

typedef struct AZ_JSON_PROPERTY {
  STRING name;
  AZ_JSON value;
} AZ_JSON_PROPERTY;
