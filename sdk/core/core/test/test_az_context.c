#include <az_test.h>

#include <az_context.h>
#include <az_result.h>

#include <stddef.h>

#include <_az_cfg.h>

void test_az_context()
{
  void *key = "k", *value = "v";
  az_context ctx1 = az_context_with_expiration(&az_context_app, 100);
  az_context ctx2 = az_context_with_value(&ctx1, key, value);
  az_context ctx3 = az_context_with_expiration(&ctx2, 250);

  int64_t expiration = az_context_get_expiration(&ctx3);
  void* value2 = NULL;
  az_result r = az_context_get_value(&ctx3, key, &value2);

  TEST_ASSERT(r == AZ_OK);
  TEST_ASSERT(value2 != NULL)

  r = az_context_get_value(&ctx3, "", &value2);

  TEST_ASSERT(r == AZ_ERROR_ITEM_NOT_FOUND);
  TEST_ASSERT(value2 == NULL)

  az_context_cancel(&ctx1);
  expiration = az_context_get_expiration(&ctx3); // Should be 0

  TEST_ASSERT(expiration == 0);
}
