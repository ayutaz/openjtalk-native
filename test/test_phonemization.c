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

static void test_phonemize(void* handle, const char* text, const char* test_name) {
    printf("\n--- %s: \"%s\" ---\n", test_name, text);

    OpenJTalkNativePhonemeResult* result = openjtalk_native_phonemize(handle, text);
    ASSERT(result != NULL, "result is not NULL");

    if (result) {
        ASSERT(result->phonemes != NULL, "phonemes string is not NULL");
        ASSERT(result->phoneme_count > 0, "phoneme count > 0");
        ASSERT(result->phoneme_ids != NULL, "phoneme_ids is not NULL");
        ASSERT(result->durations != NULL, "durations is not NULL");
        ASSERT(result->total_duration > 0.0f, "total_duration > 0");

        if (result->phonemes) {
            printf("  Phonemes: %s\n", result->phonemes);
            printf("  Count: %d\n", result->phoneme_count);
        }

        openjtalk_native_free_result(result);
    }
}

static void test_prosody(void* handle, const char* text, const char* test_name) {
    printf("\n--- %s (prosody): \"%s\" ---\n", test_name, text);

    OpenJTalkNativeProsodyResult* result = openjtalk_native_phonemize_with_prosody(handle, text);
    ASSERT(result != NULL, "prosody result is not NULL");

    if (result) {
        ASSERT(result->phonemes != NULL, "phonemes string is not NULL");
        ASSERT(result->phoneme_count > 0, "phoneme count > 0");
        ASSERT(result->prosody_a1 != NULL, "prosody_a1 is not NULL");
        ASSERT(result->prosody_a2 != NULL, "prosody_a2 is not NULL");
        ASSERT(result->prosody_a3 != NULL, "prosody_a3 is not NULL");

        if (result->phonemes) {
            printf("  Phonemes: %s\n", result->phonemes);
            printf("  Count: %d\n", result->phoneme_count);
        }

        openjtalk_native_free_prosody_result(result);
    }
}

static void test_analyze(void* handle) {
    printf("\n--- test_analyze ---\n");

    char* result = openjtalk_native_analyze(handle, "テスト");
    ASSERT(result != NULL, "analyze result is not NULL");
    if (result) {
        printf("  Result: %s\n", result);
        openjtalk_native_free_string(result);
    }
}

static void test_analyze_utf8(void* handle) {
    printf("\n--- test_analyze_utf8 ---\n");

    const char* text = "テスト";
    char* result = openjtalk_native_analyze_utf8(handle, (const unsigned char*)text, (int)strlen(text));
    ASSERT(result != NULL, "analyze_utf8 result is not NULL");
    if (result) {
        printf("  Result: %s\n", result);
        openjtalk_native_free_string(result);
    }
}

static void test_options(void* handle) {
    printf("\n--- test_options ---\n");

    int ret = openjtalk_native_set_option(handle, "speech_rate", "1.5");
    ASSERT(ret == OPENJTALK_NATIVE_SUCCESS, "set speech_rate=1.5");

    const char* val = openjtalk_native_get_option(handle, "speech_rate");
    ASSERT(val != NULL, "get speech_rate is not NULL");
    if (val) printf("  speech_rate: %s\n", val);

    ret = openjtalk_native_set_option(handle, "pitch", "2.0");
    ASSERT(ret == OPENJTALK_NATIVE_SUCCESS, "set pitch=2.0");

    ret = openjtalk_native_set_option(handle, "volume", "0.8");
    ASSERT(ret == OPENJTALK_NATIVE_SUCCESS, "set volume=0.8");

    /* Invalid option */
    ret = openjtalk_native_set_option(handle, "unknown_key", "value");
    ASSERT(ret != OPENJTALK_NATIVE_SUCCESS, "set unknown key returns error");
}

int main(void) {
    printf("=== openjtalk_native Phonemization Tests ===\n");
    printf("Version: %s\n", openjtalk_native_get_version());

    const char* dict_path = getenv("OPENJTALK_DICT");
    if (!dict_path) {
        dict_path = "../external/open_jtalk_dic_utf_8-1.11";
    }
    printf("Dictionary path: %s\n", dict_path);

    void* handle = openjtalk_native_create(dict_path);
    if (!handle) {
        printf("SKIP: Could not create OpenJTalk instance (dictionary not found)\n");
        printf("Set OPENJTALK_DICT environment variable to the dictionary path\n");
        return 0;  /* Skip gracefully if no dictionary */
    }

    /* Phonemization tests */
    test_phonemize(handle, "こんにちは", "greeting");
    test_phonemize(handle, "今日はいい天気ですね", "sentence");
    test_phonemize(handle, "日本語の音声合成", "compound");
    test_phonemize(handle, "123", "numbers");
    test_phonemize(handle, "テスト", "katakana");

    /* Prosody tests */
    test_prosody(handle, "こんにちは", "greeting_prosody");
    test_prosody(handle, "日本語の音声合成", "compound_prosody");

    /* Legacy API tests */
    test_analyze(handle);
    test_analyze_utf8(handle);

    /* Options tests */
    test_options(handle);

    /* Edge case: empty string should return NULL */
    printf("\n--- test_empty_string ---\n");
    {
        OpenJTalkNativePhonemeResult* r = openjtalk_native_phonemize(handle, "");
        ASSERT(r == NULL, "phonemize empty string returns NULL");
        int err = openjtalk_native_get_last_error(handle);
        ASSERT(err == OPENJTALK_NATIVE_ERROR_INVALID_INPUT, "empty string sets INVALID_INPUT error");
    }

    /* Edge case: empty string with prosody should return NULL */
    printf("\n--- test_empty_string_prosody ---\n");
    {
        OpenJTalkNativeProsodyResult* r = openjtalk_native_phonemize_with_prosody(handle, "");
        ASSERT(r == NULL, "phonemize_with_prosody empty string returns NULL");
    }

    /* Edge case: whitespace-only input */
    printf("\n--- test_whitespace_only ---\n");
    {
        OpenJTalkNativePhonemeResult* r = openjtalk_native_phonemize(handle, "   ");
        /* MeCab may or may not produce output for whitespace; just ensure no crash */
        ASSERT(1, "whitespace-only input does not crash");
        if (r) openjtalk_native_free_result(r);
    }

    /* Edge case: ASCII-only input */
    printf("\n--- test_ascii_input ---\n");
    {
        OpenJTalkNativePhonemeResult* r = openjtalk_native_phonemize(handle, "hello");
        /* OpenJTalk processes ASCII differently; ensure no crash */
        ASSERT(1, "ASCII-only input does not crash");
        if (r) openjtalk_native_free_result(r);
    }

    /* Edge case: mixed Japanese and numbers */
    printf("\n--- test_mixed_input ---\n");
    {
        OpenJTalkNativePhonemeResult* r = openjtalk_native_phonemize(handle, "100円です");
        ASSERT(r != NULL, "mixed input returns result");
        if (r) {
            ASSERT(r->phonemes != NULL, "mixed input has phonemes");
            ASSERT(r->phoneme_count > 0, "mixed input has positive phoneme count");
            printf("  Phonemes: %s\n", r->phonemes);
            openjtalk_native_free_result(r);
        }
    }

    /* Concrete option value verification */
    printf("\n--- test_option_value_readback ---\n");
    {
        int ret = openjtalk_native_set_option(handle, "speech_rate", "2.00");
        ASSERT(ret == OPENJTALK_NATIVE_SUCCESS, "set speech_rate=2.00");
        const char* val = openjtalk_native_get_option(handle, "speech_rate");
        ASSERT(val != NULL && strcmp(val, "2.00") == 0, "speech_rate readback is '2.00'");

        ret = openjtalk_native_set_option(handle, "pitch", "-5.00");
        ASSERT(ret == OPENJTALK_NATIVE_SUCCESS, "set pitch=-5.00");
        val = openjtalk_native_get_option(handle, "pitch");
        ASSERT(val != NULL && strcmp(val, "-5.00") == 0, "pitch readback is '-5.00'");

        ret = openjtalk_native_set_option(handle, "volume", "0.50");
        ASSERT(ret == OPENJTALK_NATIVE_SUCCESS, "set volume=0.50");
        val = openjtalk_native_get_option(handle, "volume");
        ASSERT(val != NULL && strcmp(val, "0.50") == 0, "volume readback is '0.50'");

        /* Out-of-range values should be rejected */
        ret = openjtalk_native_set_option(handle, "speech_rate", "0.0");
        ASSERT(ret != OPENJTALK_NATIVE_SUCCESS, "speech_rate=0.0 rejected");

        ret = openjtalk_native_set_option(handle, "speech_rate", "11.0");
        ASSERT(ret != OPENJTALK_NATIVE_SUCCESS, "speech_rate=11.0 rejected");

        ret = openjtalk_native_set_option(handle, "pitch", "-25.0");
        ASSERT(ret != OPENJTALK_NATIVE_SUCCESS, "pitch=-25.0 rejected");

        ret = openjtalk_native_set_option(handle, "volume", "-1.0");
        ASSERT(ret != OPENJTALK_NATIVE_SUCCESS, "volume=-1.0 rejected");

        ret = openjtalk_native_set_option(handle, "volume", "3.0");
        ASSERT(ret != OPENJTALK_NATIVE_SUCCESS, "volume=3.0 rejected");

        /* Unknown option key */
        val = openjtalk_native_get_option(handle, "nonexistent_key");
        ASSERT(val == NULL, "get_option for unknown key returns NULL");
    }

    openjtalk_native_destroy(handle);

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
