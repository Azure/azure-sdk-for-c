#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Include the actual production header
#include "azure/platform/az_curl.h"

// Mock the upload callback function signature
typedef size_t (*curl_read_callback)(char *buffer, size_t size, size_t nitems, void *userdata);

START_TEST(test_curl_upload_buffer_bounds)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "A",  // Valid minimal input (1 byte)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",  // Exact buffer size (26 bytes)
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ",  // 2x buffer size (52 bytes)
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",  // 10x buffer size (100 bytes)
    };
    
    // Buffer size from vulnerable code context
    const size_t buffer_size = 26;
    char dst_buffer[buffer_size];
    
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // Clear buffer before each test
        memset(dst_buffer, 0, buffer_size);
        
        // Create az_span from payload
        az_span content = az_span_create_from_str((char*)payloads[i]);
        az_span *upload_content = &content;
        
        // Calculate size to copy (simulating the vulnerable code path)
        size_t size_of_copy = az_span_size(content);
        
        // This is where the actual vulnerability would occur
        // We're testing that size_of_copy doesn't exceed buffer_size
        ck_assert_msg(size_of_copy <= buffer_size, 
                     "Buffer overflow attempted: size_of_copy=%zu, buffer_size=%zu", 
                     size_of_copy, buffer_size);
        
        // If assertion passes, perform the copy (simulating production code)
        if (size_of_copy <= buffer_size) {
            memcpy(dst_buffer, az_span_ptr(*upload_content), size_of_copy);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_curl_upload_buffer_bounds);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}