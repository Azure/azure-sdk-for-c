#include "./az_types.h"

typedef void const *az_error;

#define AZ_OK NULL

typedef struct {
  void *context;
  az_error (*write)(void *context, az_string s);
} az_write;

typedef struct {
  void *context;
  az_error (*read)(void *context, az_string *p_s);
} az_read;
