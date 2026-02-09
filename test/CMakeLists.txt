enable_testing()

# Test: API lifecycle and error handling
add_executable(test_api test_api.c)
target_include_directories(test_api PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_api openjtalk_native)
add_test(NAME test_api COMMAND test_api)

# Test: Phonemization (requires dictionary)
add_executable(test_phonemization test_phonemization.c)
target_include_directories(test_phonemization PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test_phonemization openjtalk_native)
add_test(NAME test_phonemization COMMAND test_phonemization)

# Set dictionary path for tests via environment variable
set_tests_properties(test_phonemization PROPERTIES
    ENVIRONMENT "OPENJTALK_DICT=${CMAKE_SOURCE_DIR}/external/open_jtalk_dic_utf_8-1.11"
)
