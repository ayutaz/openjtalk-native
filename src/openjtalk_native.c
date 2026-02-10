#include "openjtalk_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef _WIN32
#define strdup _strdup
#endif

#include <jpcommon.h>
#include <mecab.h>
#include <njd.h>
#include <text2mecab.h>
#include <mecab2njd.h>
#include <njd_set_pronunciation.h>
#include <njd_set_digit.h>
#include <njd_set_accent_phrase.h>
#include <njd_set_accent_type.h>
#include <njd_set_unvoiced_vowel.h>
#include <njd_set_long_vowel.h>
#include <njd2jpcommon.h>

#define VERSION "1.0.0"

/* Maximum input text length (in bytes) to prevent buffer overflows.
   text2mecab can expand input, so we cap well below the 8192 buffer. */
#define MAX_INPUT_TEXT_LENGTH 4096

/* Debug logging */
#ifdef ENABLE_DEBUG_LOG
#ifdef ANDROID
#include <android/log.h>
#define DEBUG_LOG(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "OpenJTalkNative", fmt, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) fprintf(stderr, "[OpenJTalkNative] " fmt "\n", ##__VA_ARGS__)
#endif
#else
#define DEBUG_LOG(fmt, ...)
#endif

/* OpenJTalk context structure */
typedef struct {
    Mecab* mecab;
    NJD* njd;
    JPCommon* jpcommon;
    char* dict_path;
    int last_error;
    bool initialized;
    double speech_rate;
    double pitch;
    double volume;
    char option_buffer[32]; /* Per-instance buffer for get_option return values */
} OpenJTalkNativeContext;

const char* openjtalk_native_get_version(void) {
    return VERSION;
}

void* openjtalk_native_create(const char* dict_path) {
    DEBUG_LOG("openjtalk_native_create called with dict_path: %s", dict_path ? dict_path : "NULL");

    if (!dict_path) {
        DEBUG_LOG("ERROR: dict_path is NULL");
        return NULL;
    }

    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)calloc(1, sizeof(OpenJTalkNativeContext));
    if (!ctx) {
        return NULL;
    }

    ctx->speech_rate = 1.0;
    ctx->pitch = 0.0;
    ctx->volume = 1.0;

    ctx->dict_path = strdup(dict_path);
    if (!ctx->dict_path) {
        free(ctx);
        return NULL;
    }

    /* Initialize Mecab */
    ctx->mecab = (Mecab*)calloc(1, sizeof(Mecab));
    if (!ctx->mecab) {
        free(ctx->dict_path);
        free(ctx);
        return NULL;
    }

    if (Mecab_initialize(ctx->mecab) != TRUE) {
        DEBUG_LOG("ERROR: Mecab_initialize failed");
        free(ctx->mecab);
        free(ctx->dict_path);
        free(ctx);
        return NULL;
    }

    if (Mecab_load(ctx->mecab, ctx->dict_path) != TRUE) {
        DEBUG_LOG("ERROR: Mecab_load failed with path: %s", ctx->dict_path);
        Mecab_clear(ctx->mecab);
        free(ctx->mecab);
        free(ctx->dict_path);
        free(ctx);
        return NULL;
    }

    /* Initialize NJD */
    ctx->njd = (NJD*)calloc(1, sizeof(NJD));
    if (!ctx->njd) {
        Mecab_clear(ctx->mecab);
        free(ctx->mecab);
        free(ctx->dict_path);
        free(ctx);
        return NULL;
    }
    NJD_initialize(ctx->njd);

    /* Initialize JPCommon */
    ctx->jpcommon = (JPCommon*)calloc(1, sizeof(JPCommon));
    if (!ctx->jpcommon) {
        NJD_clear(ctx->njd);
        free(ctx->njd);
        Mecab_clear(ctx->mecab);
        free(ctx->mecab);
        free(ctx->dict_path);
        free(ctx);
        return NULL;
    }
    JPCommon_initialize(ctx->jpcommon);

    ctx->initialized = true;
    ctx->last_error = OPENJTALK_NATIVE_SUCCESS;

    DEBUG_LOG("OpenJTalk initialized with dictionary: %s", dict_path);
    return ctx;
}

void openjtalk_native_destroy(void* handle) {
    if (!handle) return;

    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)handle;

    if (ctx->jpcommon) {
        JPCommon_clear(ctx->jpcommon);
        free(ctx->jpcommon);
    }
    if (ctx->njd) {
        NJD_clear(ctx->njd);
        free(ctx->njd);
    }
    if (ctx->mecab) {
        Mecab_clear(ctx->mecab);
        free(ctx->mecab);
    }
    if (ctx->dict_path) {
        free(ctx->dict_path);
    }
    free(ctx);
}

/* Convert JPCommon labels to phonemes */
static OpenJTalkNativePhonemeResult* labels_to_phonemes(OpenJTalkNativeContext* ctx, JPCommon* jpcommon) {
    OpenJTalkNativePhonemeResult* result = (OpenJTalkNativePhonemeResult*)calloc(1, sizeof(OpenJTalkNativePhonemeResult));
    if (!result) return NULL;

    int label_size = JPCommon_get_label_size(jpcommon);
    char** label_feature = JPCommon_get_label_feature(jpcommon);

    if (label_size <= 0 || !label_feature) {
        free(result);
        return NULL;
    }

    char phoneme_buffer[8192] = {0};
    char* buf_ptr = phoneme_buffer;
    int phoneme_count = 0;

    for (int i = 0; i < label_size; i++) {
        if (!label_feature[i]) continue;

        DEBUG_LOG("Label[%d]: %s", i, label_feature[i]);

        /* Extract phoneme from full-context label: xx^xx-phoneme+xx=xx/A:... */
        char* phoneme_start = strchr(label_feature[i], '-');
        char* phoneme_end = strchr(label_feature[i], '+');

        if (phoneme_start && phoneme_end && phoneme_start < phoneme_end) {
            phoneme_start++; /* Skip '-' */
            int phoneme_len = (int)(phoneme_end - phoneme_start);

            /* Handle silence: require >= 3 chars to avoid matching 's' as 'sil' */
            if (phoneme_len >= 3 && strncmp(phoneme_start, "sil", 3) == 0) {
                if (i == 0 || i == label_size - 1) {
                    if (buf_ptr != phoneme_buffer) *buf_ptr++ = ' ';
                    memcpy(buf_ptr, "pau", 3);
                    buf_ptr += 3;
                    phoneme_count++;
                }
            } else {
                if (buf_ptr != phoneme_buffer) *buf_ptr++ = ' ';
                strncpy(buf_ptr, phoneme_start, phoneme_len);
                buf_ptr += phoneme_len;
                phoneme_count++;
            }
        }
    }

    *buf_ptr = '\0';

    DEBUG_LOG("Extracted phonemes: %s (count: %d)", phoneme_buffer, phoneme_count);

    result->phoneme_count = phoneme_count;
    result->phonemes = strdup(phoneme_buffer);
    result->phoneme_ids = (int*)calloc(phoneme_count, sizeof(int));
    result->durations = (float*)calloc(phoneme_count, sizeof(float));

    if (!result->phonemes || !result->phoneme_ids || !result->durations) {
        openjtalk_native_free_result(result);
        return NULL;
    }

    for (int i = 0; i < phoneme_count; i++) {
        result->phoneme_ids[i] = 1;
        result->durations[i] = 0.05f;
    }
    result->total_duration = phoneme_count * 0.05f;

    return result;
}

/* Convert JPCommon labels to phonemes with prosody features (A1/A2/A3) */
static OpenJTalkNativeProsodyResult* labels_to_phonemes_with_prosody(OpenJTalkNativeContext* ctx, JPCommon* jpcommon) {
    OpenJTalkNativeProsodyResult* result = (OpenJTalkNativeProsodyResult*)calloc(1, sizeof(OpenJTalkNativeProsodyResult));
    if (!result) return NULL;

    int label_size = JPCommon_get_label_size(jpcommon);
    char** label_feature = JPCommon_get_label_feature(jpcommon);

    if (label_size <= 0 || !label_feature) {
        free(result);
        return NULL;
    }

    int* temp_a1 = (int*)calloc(label_size, sizeof(int));
    int* temp_a2 = (int*)calloc(label_size, sizeof(int));
    int* temp_a3 = (int*)calloc(label_size, sizeof(int));

    if (!temp_a1 || !temp_a2 || !temp_a3) {
        free(temp_a1);
        free(temp_a2);
        free(temp_a3);
        free(result);
        return NULL;
    }

    char phoneme_buffer[8192] = {0};
    char* buf_ptr = phoneme_buffer;
    int phoneme_count = 0;

    for (int i = 0; i < label_size; i++) {
        if (!label_feature[i]) continue;

        char* phoneme_start = strchr(label_feature[i], '-');
        char* phoneme_end = strchr(label_feature[i], '+');

        if (phoneme_start && phoneme_end && phoneme_start < phoneme_end) {
            phoneme_start++;
            int phoneme_len = (int)(phoneme_end - phoneme_start);

            /* Parse /A:a1+a2+a3/ */
            int a1 = 0, a2 = 0, a3 = 0;
            char* accent_marker = strstr(label_feature[i], "/A:");
            if (accent_marker) {
                accent_marker += 3;
                a1 = (int)strtol(accent_marker, &accent_marker, 10);
                if (*accent_marker == '+') {
                    accent_marker++;
                    a2 = (int)strtol(accent_marker, &accent_marker, 10);
                    if (*accent_marker == '+') {
                        accent_marker++;
                        a3 = (int)strtol(accent_marker, NULL, 10);
                    }
                }
            }

            if (phoneme_len >= 3 && strncmp(phoneme_start, "sil", 3) == 0) {
                if (i == 0 || i == label_size - 1) {
                    if (buf_ptr != phoneme_buffer) *buf_ptr++ = ' ';
                    memcpy(buf_ptr, "pau", 3);
                    buf_ptr += 3;
                    temp_a1[phoneme_count] = a1;
                    temp_a2[phoneme_count] = a2;
                    temp_a3[phoneme_count] = a3;
                    phoneme_count++;
                }
            } else {
                if (buf_ptr != phoneme_buffer) *buf_ptr++ = ' ';
                strncpy(buf_ptr, phoneme_start, phoneme_len);
                buf_ptr += phoneme_len;
                temp_a1[phoneme_count] = a1;
                temp_a2[phoneme_count] = a2;
                temp_a3[phoneme_count] = a3;
                phoneme_count++;
            }
        }
    }

    *buf_ptr = '\0';

    result->phoneme_count = phoneme_count;
    result->phonemes = strdup(phoneme_buffer);
    result->prosody_a1 = (int*)calloc(phoneme_count, sizeof(int));
    result->prosody_a2 = (int*)calloc(phoneme_count, sizeof(int));
    result->prosody_a3 = (int*)calloc(phoneme_count, sizeof(int));

    if (!result->phonemes || !result->prosody_a1 || !result->prosody_a2 || !result->prosody_a3) {
        openjtalk_native_free_prosody_result(result);
        free(temp_a1);
        free(temp_a2);
        free(temp_a3);
        return NULL;
    }

    for (int i = 0; i < phoneme_count; i++) {
        result->prosody_a1[i] = temp_a1[i];
        result->prosody_a2[i] = temp_a2[i];
        result->prosody_a3[i] = temp_a3[i];
    }

    free(temp_a1);
    free(temp_a2);
    free(temp_a3);

    return result;
}

/* Run the NJD processing pipeline */
static void run_njd_pipeline(OpenJTalkNativeContext* ctx) {
    njd_set_pronunciation(ctx->njd);
    njd_set_digit(ctx->njd);
    njd_set_accent_phrase(ctx->njd);
    njd_set_accent_type(ctx->njd);
    njd_set_unvoiced_vowel(ctx->njd);
    njd_set_long_vowel(ctx->njd);
}

OpenJTalkNativePhonemeResult* openjtalk_native_phonemize(void* handle, const char* text) {
    if (!handle || !text) return NULL;

    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)handle;

    if (!ctx->initialized) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_INITIALIZATION_FAILED;
        return NULL;
    }

    /* Reject empty strings */
    size_t text_len = strlen(text);
    if (text_len == 0) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
        return NULL;
    }

    /* Validate input length to prevent buffer overflow in text2mecab */
    if (text_len > MAX_INPUT_TEXT_LENGTH) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
        return NULL;
    }

    DEBUG_LOG("Phonemizing text: %s", text);

    NJD_clear(ctx->njd);
    JPCommon_clear(ctx->jpcommon);

    char mecab_text[8192];
    text2mecab(mecab_text, text);

    if (Mecab_analysis(ctx->mecab, mecab_text) != TRUE) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_PHONEMIZATION_FAILED;
        return NULL;
    }

    mecab2njd(ctx->njd, Mecab_get_feature(ctx->mecab), Mecab_get_size(ctx->mecab));
    run_njd_pipeline(ctx);
    njd2jpcommon(ctx->jpcommon, ctx->njd);
    JPCommon_make_label(ctx->jpcommon);

    OpenJTalkNativePhonemeResult* result = labels_to_phonemes(ctx, ctx->jpcommon);
    if (!result) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_MEMORY_ALLOCATION;
        return NULL;
    }

    ctx->last_error = OPENJTALK_NATIVE_SUCCESS;
    return result;
}

void openjtalk_native_free_result(OpenJTalkNativePhonemeResult* result) {
    if (!result) return;
    if (result->phonemes) free(result->phonemes);
    if (result->phoneme_ids) free(result->phoneme_ids);
    if (result->durations) free(result->durations);
    free(result);
}

OpenJTalkNativeProsodyResult* openjtalk_native_phonemize_with_prosody(void* handle, const char* text) {
    if (!handle || !text) return NULL;

    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)handle;

    if (!ctx->initialized) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_INITIALIZATION_FAILED;
        return NULL;
    }

    /* Reject empty strings */
    size_t text_len = strlen(text);
    if (text_len == 0) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
        return NULL;
    }

    /* Validate input length to prevent buffer overflow in text2mecab */
    if (text_len > MAX_INPUT_TEXT_LENGTH) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
        return NULL;
    }

    NJD_clear(ctx->njd);
    JPCommon_clear(ctx->jpcommon);

    char mecab_text[8192];
    text2mecab(mecab_text, text);

    if (Mecab_analysis(ctx->mecab, mecab_text) != TRUE) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_PHONEMIZATION_FAILED;
        return NULL;
    }

    mecab2njd(ctx->njd, Mecab_get_feature(ctx->mecab), Mecab_get_size(ctx->mecab));
    run_njd_pipeline(ctx);
    njd2jpcommon(ctx->jpcommon, ctx->njd);
    JPCommon_make_label(ctx->jpcommon);

    OpenJTalkNativeProsodyResult* result = labels_to_phonemes_with_prosody(ctx, ctx->jpcommon);
    if (!result) {
        ctx->last_error = OPENJTALK_NATIVE_ERROR_MEMORY_ALLOCATION;
        return NULL;
    }

    ctx->last_error = OPENJTALK_NATIVE_SUCCESS;
    return result;
}

void openjtalk_native_free_prosody_result(OpenJTalkNativeProsodyResult* result) {
    if (!result) return;
    if (result->phonemes) free(result->phonemes);
    if (result->prosody_a1) free(result->prosody_a1);
    if (result->prosody_a2) free(result->prosody_a2);
    if (result->prosody_a3) free(result->prosody_a3);
    free(result);
}

int openjtalk_native_get_last_error(void* handle) {
    if (!handle) return OPENJTALK_NATIVE_ERROR_INVALID_HANDLE;
    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)handle;
    return ctx->last_error;
}

const char* openjtalk_native_get_error_string(int error_code) {
    switch (error_code) {
        case OPENJTALK_NATIVE_SUCCESS:                    return "Success";
        case OPENJTALK_NATIVE_ERROR_INVALID_HANDLE:       return "Invalid handle";
        case OPENJTALK_NATIVE_ERROR_INVALID_INPUT:        return "Invalid input";
        case OPENJTALK_NATIVE_ERROR_MEMORY_ALLOCATION:    return "Memory allocation failed";
        case OPENJTALK_NATIVE_ERROR_DICTIONARY_NOT_FOUND: return "Dictionary not found";
        case OPENJTALK_NATIVE_ERROR_INITIALIZATION_FAILED:return "Initialization failed";
        case OPENJTALK_NATIVE_ERROR_PHONEMIZATION_FAILED: return "Phonemization failed";
        case OPENJTALK_NATIVE_ERROR_PROCESSING:           return "Processing error";
        case OPENJTALK_NATIVE_ERROR_INVALID_OPTION:       return "Invalid option";
        case OPENJTALK_NATIVE_ERROR_INVALID_DICTIONARY:   return "Invalid dictionary";
        case OPENJTALK_NATIVE_ERROR_INVALID_UTF8:         return "Invalid UTF-8";
        default:                                          return "Unknown error";
    }
}

int openjtalk_native_set_option(void* handle, const char* key, const char* value) {
    if (!handle || !key || !value) return OPENJTALK_NATIVE_ERROR_INVALID_INPUT;

    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)handle;

    if (strcmp(key, "speech_rate") == 0) {
        double rate = atof(value);
        if (rate > 0.0 && rate <= 10.0) {
            ctx->speech_rate = rate;
            return OPENJTALK_NATIVE_SUCCESS;
        }
        return OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
    }
    else if (strcmp(key, "pitch") == 0) {
        double p = atof(value);
        if (p >= -20.0 && p <= 20.0) {
            ctx->pitch = p;
            return OPENJTALK_NATIVE_SUCCESS;
        }
        return OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
    }
    else if (strcmp(key, "volume") == 0) {
        double v = atof(value);
        if (v >= 0.0 && v <= 2.0) {
            ctx->volume = v;
            return OPENJTALK_NATIVE_SUCCESS;
        }
        return OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
    }

    return OPENJTALK_NATIVE_ERROR_INVALID_INPUT;
}

const char* openjtalk_native_get_option(void* handle, const char* key) {
    if (!handle || !key) return NULL;

    OpenJTalkNativeContext* ctx = (OpenJTalkNativeContext*)handle;

    if (strcmp(key, "speech_rate") == 0) {
        snprintf(ctx->option_buffer, sizeof(ctx->option_buffer), "%.2f", ctx->speech_rate);
        return ctx->option_buffer;
    }
    else if (strcmp(key, "pitch") == 0) {
        snprintf(ctx->option_buffer, sizeof(ctx->option_buffer), "%.2f", ctx->pitch);
        return ctx->option_buffer;
    }
    else if (strcmp(key, "volume") == 0) {
        snprintf(ctx->option_buffer, sizeof(ctx->option_buffer), "%.2f", ctx->volume);
        return ctx->option_buffer;
    }

    return NULL;
}

/* UTF-8 optimized functions */

void* openjtalk_native_initialize_utf8(const unsigned char* dict_path_utf8, int path_length) {
    if (!dict_path_utf8 || path_length <= 0) return NULL;

    char* dict_path = (char*)malloc(path_length + 1);
    if (!dict_path) return NULL;

    memcpy(dict_path, dict_path_utf8, path_length);
    dict_path[path_length] = '\0';

    void* handle = openjtalk_native_create(dict_path);
    free(dict_path);
    return handle;
}

void* openjtalk_native_initialize(const char* dict_path) {
    return openjtalk_native_create(dict_path);
}

char* openjtalk_native_analyze_utf8(void* handle, const unsigned char* text_utf8, int text_length) {
    if (!handle || !text_utf8 || text_length <= 0) return NULL;

    char* text = (char*)malloc(text_length + 1);
    if (!text) return NULL;

    memcpy(text, text_utf8, text_length);
    text[text_length] = '\0';

    OpenJTalkNativePhonemeResult* phoneme_result = openjtalk_native_phonemize(handle, text);
    free(text);

    if (!phoneme_result) return NULL;

    char* result = strdup(phoneme_result->phonemes);
    openjtalk_native_free_result(phoneme_result);
    return result;
}

char* openjtalk_native_analyze(void* handle, const char* text) {
    OpenJTalkNativePhonemeResult* phoneme_result = openjtalk_native_phonemize(handle, text);
    if (!phoneme_result) return NULL;

    char* result = strdup(phoneme_result->phonemes);
    openjtalk_native_free_result(phoneme_result);
    return result;
}

void openjtalk_native_free_string(char* result) {
    if (result) free(result);
}

void openjtalk_native_finalize(void* handle) {
    openjtalk_native_destroy(handle);
}
