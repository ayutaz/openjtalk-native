/**
 * @file openjtalk_native.h
 * @brief OpenJTalk Native - Japanese text-to-phoneme conversion library
 * @version 1.0.0
 *
 * This library provides a C API for converting Japanese text to phonemes
 * using the OpenJTalk engine with MeCab morphological analysis.
 */

#ifndef OPENJTALK_NATIVE_H
#define OPENJTALK_NATIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* Export/Import macros */
#ifdef _WIN32
    #ifdef OPENJTALK_NATIVE_EXPORTS
        #define OPENJTALK_NATIVE_API __declspec(dllexport)
    #else
        #define OPENJTALK_NATIVE_API __declspec(dllimport)
    #endif
#else
    #ifdef OPENJTALK_NATIVE_EXPORTS
        #define OPENJTALK_NATIVE_API __attribute__((visibility("default")))
    #else
        #define OPENJTALK_NATIVE_API
    #endif
#endif

/**
 * @brief Error codes returned by OpenJTalk Native functions
 */
typedef enum {
    OPENJTALK_NATIVE_SUCCESS = 0,
    OPENJTALK_NATIVE_ERROR_INVALID_HANDLE = -1,
    OPENJTALK_NATIVE_ERROR_INVALID_INPUT = -2,
    OPENJTALK_NATIVE_ERROR_MEMORY_ALLOCATION = -3,
    OPENJTALK_NATIVE_ERROR_DICTIONARY_NOT_FOUND = -4,
    OPENJTALK_NATIVE_ERROR_INITIALIZATION_FAILED = -5,
    OPENJTALK_NATIVE_ERROR_PHONEMIZATION_FAILED = -6,
    OPENJTALK_NATIVE_ERROR_PROCESSING = -7,
    OPENJTALK_NATIVE_ERROR_INVALID_OPTION = -8,
    OPENJTALK_NATIVE_ERROR_INVALID_DICTIONARY = -9,
    OPENJTALK_NATIVE_ERROR_INVALID_UTF8 = -10
} OpenJTalkNativeError;

/**
 * @brief Result structure for phoneme conversion
 */
typedef struct {
    char* phonemes;          /**< Space-separated phoneme string (e.g., "k o N n i ch i w a") */
    int* phoneme_ids;        /**< Array of phoneme IDs corresponding to each phoneme */
    int phoneme_count;       /**< Number of phonemes in the result */
    float* durations;        /**< Duration of each phoneme in seconds */
    float total_duration;    /**< Total duration of all phonemes in seconds */
} OpenJTalkNativePhonemeResult;

/**
 * @brief Result structure for phoneme conversion with prosody features
 *
 * Prosody values (per phoneme):
 * - A1: Relative position from accent nucleus (can be negative)
 * - A2: Position in accent phrase (1-based)
 * - A3: Total morae in accent phrase
 */
typedef struct {
    char* phonemes;          /**< Space-separated phoneme string */
    int* prosody_a1;         /**< A1: relative position from accent nucleus (per phoneme) */
    int* prosody_a2;         /**< A2: position in accent phrase, 1-based (per phoneme) */
    int* prosody_a3;         /**< A3: total morae in accent phrase (per phoneme) */
    int phoneme_count;       /**< Number of phonemes in the result */
} OpenJTalkNativeProsodyResult;

/**
 * @brief Get the version string of the library
 * @return Version string (e.g., "1.0.0")
 */
OPENJTALK_NATIVE_API const char* openjtalk_native_get_version(void);

/**
 * @brief Create a new OpenJTalk instance
 * @param dict_path Path to the dictionary directory
 * @return Handle to the OpenJTalk instance, or NULL on failure
 *
 * @note The dictionary directory should contain sys.dic, matrix.bin,
 *       char.bin, and unk.dic files.
 */
OPENJTALK_NATIVE_API void* openjtalk_native_create(const char* dict_path);

/**
 * @brief Destroy an OpenJTalk instance and free resources
 * @param handle Handle returned by openjtalk_native_create()
 */
OPENJTALK_NATIVE_API void openjtalk_native_destroy(void* handle);

/**
 * @brief Convert Japanese text to phonemes
 * @param handle Handle returned by openjtalk_native_create()
 * @param text UTF-8 encoded Japanese text
 * @return Phoneme result, or NULL on failure. Must be freed with openjtalk_native_free_result()
 */
OPENJTALK_NATIVE_API OpenJTalkNativePhonemeResult* openjtalk_native_phonemize(void* handle, const char* text);

/**
 * @brief Free a phoneme result
 * @param result Result returned by openjtalk_native_phonemize()
 */
OPENJTALK_NATIVE_API void openjtalk_native_free_result(OpenJTalkNativePhonemeResult* result);

/**
 * @brief Convert Japanese text to phonemes with prosody features
 * @param handle Handle returned by openjtalk_native_create()
 * @param text UTF-8 encoded Japanese text
 * @return Prosody result, or NULL on failure. Must be freed with openjtalk_native_free_prosody_result()
 */
OPENJTALK_NATIVE_API OpenJTalkNativeProsodyResult* openjtalk_native_phonemize_with_prosody(void* handle, const char* text);

/**
 * @brief Free a prosody result
 * @param result Result returned by openjtalk_native_phonemize_with_prosody()
 */
OPENJTALK_NATIVE_API void openjtalk_native_free_prosody_result(OpenJTalkNativeProsodyResult* result);

/**
 * @brief Get the last error code for an instance
 * @param handle Handle returned by openjtalk_native_create()
 * @return Error code (OpenJTalkNativeError)
 */
OPENJTALK_NATIVE_API int openjtalk_native_get_last_error(void* handle);

/**
 * @brief Get a human-readable error description
 * @param error_code Error code from OpenJTalkNativeError
 * @return Error description string
 */
OPENJTALK_NATIVE_API const char* openjtalk_native_get_error_string(int error_code);

/**
 * @brief Set an option on the OpenJTalk instance
 * @param handle Handle returned by openjtalk_native_create()
 * @param key Option key (e.g., "speech_rate", "pitch", "volume")
 * @param value Option value as string
 * @return OPENJTALK_NATIVE_SUCCESS on success, error code on failure
 */
OPENJTALK_NATIVE_API int openjtalk_native_set_option(void* handle, const char* key, const char* value);

/**
 * @brief Get an option value from the OpenJTalk instance
 * @param handle Handle returned by openjtalk_native_create()
 * @param key Option key
 * @return Option value as string, or NULL if not found
 */
OPENJTALK_NATIVE_API const char* openjtalk_native_get_option(void* handle, const char* key);

/* UTF-8 optimized functions (avoids string marshalling overhead on mobile) */
OPENJTALK_NATIVE_API void* openjtalk_native_initialize_utf8(const unsigned char* dict_path_utf8, int path_length);
OPENJTALK_NATIVE_API char* openjtalk_native_analyze_utf8(void* handle, const unsigned char* text_utf8, int text_length);

/* Legacy compatibility aliases */
OPENJTALK_NATIVE_API void* openjtalk_native_initialize(const char* dict_path);
OPENJTALK_NATIVE_API char* openjtalk_native_analyze(void* handle, const char* text);
OPENJTALK_NATIVE_API void openjtalk_native_finalize(void* handle);
OPENJTALK_NATIVE_API void openjtalk_native_free_string(char* result);

#ifdef __cplusplus
}
#endif

#endif /* OPENJTALK_NATIVE_H */
