#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openjtalk_native.h"

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { \
        tests_passed++; \
        printf("  PASS: %s\n", msg); \
    } else { \
        printf("  FAIL: %s\n", msg); \
    } \
} while(0)

void test_version(void) {
    printf("\n--- test_version ---\n");
    const char* version = openjtalk_native_get_version();
    ASSERT(version != NULL, "version is not NULL");
    ASSERT(strlen(version) > 0, "version is not empty");
    printf("  Version: %s\n", version);
}

void test_null_handling(void) {
    printf("\n--- test_null_handling ---\n");

    /* Create with NULL dict_path should fail */
    void* handle = openjtalk_native_create(NULL);
    ASSERT(handle == NULL, "create with NULL dict_path returns NULL");

    /* Destroy NULL handle should not crash */
    openjtalk_native_destroy(NULL);
    ASSERT(1, "destroy NULL handle does not crash");

    /* Phonemize with NULL handle should return NULL */
    OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(NULL, "test");
    ASSERT(result == NULL, "phonemize with NULL handle returns NULL");

    /* Phonemize with NULL text should return NULL */
    result = openjtalk_native_phonemize((void*)1, NULL);
    ASSERT(result == NULL, "phonemize with NULL text returns NULL");

    /* Free NULL result should not crash */
    openjtalk_native_free_result(NULL);
    ASSERT(1, "free_result NULL does not crash");

    /* Free NULL prosody result should not crash */
    openjtalk_native_free_prosody_result(NULL);
    ASSERT(1, "free_prosody_result NULL does not crash");

    /* Free NULL string should not crash */
    openjtalk_native_free_string(NULL);
    ASSERT(1, "free_string NULL does not crash");
}

void test_error_codes(void) {
    printf("\n--- test_error_codes ---\n");

    /* get_last_error with NULL handle */
    int err = openjtalk_native_get_last_error(NULL);
    ASSERT(err == OPENJTALK_NATIVE_ERROR_INVALID_HANDLE, "get_last_error(NULL) returns INVALID_HANDLE");

    /* Error strings */
    const char* str = openjtalk_native_get_error_string(OPENJTALK_NATIVE_SUCCESS);
    ASSERT(str != NULL && strcmp(str, "Success") == 0, "error string for SUCCESS");

    str = openjtalk_native_get_error_string(OPENJTALK_NATIVE_ERROR_INVALID_HANDLE);
    ASSERT(str != NULL && strcmp(str, "Invalid handle") == 0, "error string for INVALID_HANDLE");

    str = openjtalk_native_get_error_string(-999);
    ASSERT(str != NULL && strcmp(str, "Unknown error") == 0, "error string for unknown code");
}

void test_invalid_dict(void) {
    printf("\n--- test_invalid_dict ---\n");

    /* Create with non-existent dictionary should fail */
    void* handle = openjtalk_native_create("/nonexistent/path/to/dict");
    ASSERT(handle == NULL, "create with invalid dict path returns NULL");
}

void test_option_handling(void) {
    printf("\n--- test_option_handling ---\n");

    /* set_option with NULL handle */
    int result = openjtalk_native_set_option(NULL, "key", "value");
    ASSERT(result == OPENJTALK_NATIVE_ERROR_INVALID_INPUT, "set_option with NULL handle returns error");

    /* get_option with NULL handle */
    const char* value = openjtalk_native_get_option(NULL, "key");
    ASSERT(value == NULL, "get_option with NULL handle returns NULL");
}

void test_legacy_api(void) {
    printf("\n--- test_legacy_api ---\n");

    /* Initialize with NULL should fail */
    void* handle = openjtalk_native_initialize(NULL);
    ASSERT(handle == NULL, "initialize(NULL) returns NULL");

    /* Finalize NULL should not crash */
    openjtalk_native_finalize(NULL);
    ASSERT(1, "finalize(NULL) does not crash");

    /* Analyze with NULL handle */
    char* result = openjtalk_native_analyze(NULL, "test");
    ASSERT(result == NULL, "analyze(NULL, ...) returns NULL");

    /* UTF-8 initialize with NULL */
    handle = openjtalk_native_initialize_utf8(NULL, 0);
    ASSERT(handle == NULL, "initialize_utf8(NULL, 0) returns NULL");

    /* UTF-8 analyze with NULL handle */
    result = openjtalk_native_analyze_utf8(NULL, (const unsigned char*)"test", 4);
    ASSERT(result == NULL, "analyze_utf8(NULL, ...) returns NULL");
}

int main(void) {
    printf("=== openjtalk_native API Tests ===\n");

    test_version();
    test_null_handling();
    test_error_codes();
    test_invalid_dict();
    test_option_handling();
    test_legacy_api();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
