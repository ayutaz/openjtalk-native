cmake_minimum_required(VERSION 3.10)
project(openjtalk_native VERSION 1.0.0)

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Platform detection
if(WIN32)
    set(PLATFORM_NAME "windows")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DOPENJTALK_NATIVE_EXPORTS)
elseif(ANDROID)
    set(PLATFORM_NAME "android")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_definitions(-DANDROID)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -ffunction-sections -fdata-sections")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os -ffunction-sections -fdata-sections")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,--strip-all")
    endif()
elseif(IOS)
    set(PLATFORM_NAME "ios")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_definitions(-DIOS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_NAME "linux")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
elseif(APPLE)
    set(PLATFORM_NAME "macos")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Build options
option(ENABLE_DEBUG_LOG "Enable debug logging" OFF)
option(BUILD_TESTS "Build test executables" ON)

# Find OpenJTalk installation
if(ANDROID)
    set(OPENJTALK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/android_${ANDROID_ABI}")
    set(OPENJTALK_INCLUDE_DIR "${OPENJTALK_ROOT}/include")
    set(OPENJTALK_LIB_DIR "${OPENJTALK_ROOT}/lib")
else()
    set(OPENJTALK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/install")
    set(OPENJTALK_INCLUDE_DIR "${OPENJTALK_ROOT}/include")
    set(OPENJTALK_LIB_DIR "${OPENJTALK_ROOT}/lib")
    if(NOT EXISTS "${OPENJTALK_ROOT}")
        set(OPENJTALK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/install")
        set(OPENJTALK_INCLUDE_DIR "${OPENJTALK_ROOT}/include")
        set(OPENJTALK_LIB_DIR "${OPENJTALK_ROOT}/lib")
    endif()
endif()

# Source files
set(SOURCES src/openjtalk_native.c)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OPENJTALK_INCLUDE_DIR}
)

# Fallback include paths for CI builds
if(NOT EXISTS "${OPENJTALK_INCLUDE_DIR}/jpcommon.h")
    message(STATUS "OpenJTalk headers not found in install dir, checking alternate locations")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/install/include")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/install/include)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/open_jtalk-1.11")
        set(OJT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external/open_jtalk-1.11")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/open_jtalk-1.11")
        set(OJT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/open_jtalk-1.11")
    endif()
    if(OJT_SRC)
        include_directories(
            ${OJT_SRC}/jpcommon
            ${OJT_SRC}/njd
            ${OJT_SRC}/mecab/src
            ${OJT_SRC}/text2mecab
            ${OJT_SRC}/mecab2njd
            ${OJT_SRC}/njd_set_pronunciation
            ${OJT_SRC}/njd_set_digit
            ${OJT_SRC}/njd_set_accent_phrase
            ${OJT_SRC}/njd_set_accent_type
            ${OJT_SRC}/njd_set_unvoiced_vowel
            ${OJT_SRC}/njd_set_long_vowel
            ${OJT_SRC}/njd2jpcommon
        )
    endif()
endif()

# Create library (static for iOS, shared for others)
if(IOS)
    add_library(openjtalk_native STATIC ${SOURCES})
    set_target_properties(openjtalk_native PROPERTIES
        XCODE_ATTRIBUTE_VALID_ARCHS "arm64"
        XCODE_ATTRIBUTE_ARCHS "arm64"
    )
else()
    add_library(openjtalk_native SHARED ${SOURCES})
endif()

# Find OpenJTalk static libraries
if(ANDROID)
    set(OPENJTALK_LIBS_DIR "${OPENJTALK_LIB_DIR}")
else()
    set(OPENJTALK_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/open_jtalk-1.11")
    if(NOT EXISTS "${OPENJTALK_SRC_DIR}")
        set(OPENJTALK_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/open_jtalk-1.11")
    endif()
endif()

# Link static libraries
if(ANDROID AND EXISTS "${OPENJTALK_LIBS_DIR}/libjpcommon.a")
    set(OPENJTALK_LIBS
        ${OPENJTALK_LIBS_DIR}/libtext2mecab.a
        ${OPENJTALK_LIBS_DIR}/libmecab.a
        ${OPENJTALK_LIBS_DIR}/libmecab2njd.a
        ${OPENJTALK_LIBS_DIR}/libnjd.a
        ${OPENJTALK_LIBS_DIR}/libnjd_set_pronunciation.a
        ${OPENJTALK_LIBS_DIR}/libnjd_set_digit.a
        ${OPENJTALK_LIBS_DIR}/libnjd_set_accent_phrase.a
        ${OPENJTALK_LIBS_DIR}/libnjd_set_accent_type.a
        ${OPENJTALK_LIBS_DIR}/libnjd_set_unvoiced_vowel.a
        ${OPENJTALK_LIBS_DIR}/libnjd_set_long_vowel.a
        ${OPENJTALK_LIBS_DIR}/libnjd2jpcommon.a
        ${OPENJTALK_LIBS_DIR}/libjpcommon.a
        ${OPENJTALK_LIBS_DIR}/libHTSEngine.a
    )
elseif(NOT ANDROID AND EXISTS "${OPENJTALK_SRC_DIR}/jpcommon/libjpcommon.a")
    # Find HTSEngine library
    if(EXISTS "${OPENJTALK_LIB_DIR}/libHTSEngine.a")
        set(HTSENGINE_LIB "${OPENJTALK_LIB_DIR}/libHTSEngine.a")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/install/lib/libHTSEngine.a")
        set(HTSENGINE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/openjtalk_build/install/lib/libHTSEngine.a")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/install/lib/libHTSEngine.a")
        set(HTSENGINE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/external/install/lib/libHTSEngine.a")
    else()
        message(WARNING "HTSEngine library not found - building without it")
        set(HTSENGINE_LIB "")
    endif()

    set(OPENJTALK_LIBS
        ${OPENJTALK_SRC_DIR}/jpcommon/libjpcommon.a
        ${OPENJTALK_SRC_DIR}/njd2jpcommon/libnjd2jpcommon.a
        ${OPENJTALK_SRC_DIR}/njd_set_unvoiced_vowel/libnjd_set_unvoiced_vowel.a
        ${OPENJTALK_SRC_DIR}/njd_set_pronunciation/libnjd_set_pronunciation.a
        ${OPENJTALK_SRC_DIR}/njd_set_long_vowel/libnjd_set_long_vowel.a
        ${OPENJTALK_SRC_DIR}/njd_set_digit/libnjd_set_digit.a
        ${OPENJTALK_SRC_DIR}/njd_set_accent_type/libnjd_set_accent_type.a
        ${OPENJTALK_SRC_DIR}/njd_set_accent_phrase/libnjd_set_accent_phrase.a
        ${OPENJTALK_SRC_DIR}/njd/libnjd.a
        ${OPENJTALK_SRC_DIR}/mecab2njd/libmecab2njd.a
        ${OPENJTALK_SRC_DIR}/text2mecab/libtext2mecab.a
        ${OPENJTALK_SRC_DIR}/mecab/src/libmecab.a
        ${HTSENGINE_LIB}
    )
endif()

# Android-specific optimizations
if(ANDROID)
    target_compile_options(openjtalk_native PRIVATE
        -Os -ffunction-sections -fdata-sections -fvisibility=hidden
    )
    target_link_options(openjtalk_native PRIVATE
        -Wl,--gc-sections -Wl,--strip-debug -Wl,--exclude-libs,ALL -fuse-ld=lld
    )
    target_compile_definitions(openjtalk_native PRIVATE OPENJTALK_NATIVE_EXPORTS)
endif()

# Platform-specific linking
if(ANDROID)
    target_link_libraries(openjtalk_native ${OPENJTALK_LIBS} c++_shared m log)
elseif(IOS)
    target_link_libraries(openjtalk_native ${OPENJTALK_LIBS}
        "-framework Foundation"
        "-framework CoreFoundation"
    )
elseif(WIN32)
    if(MSVC)
        target_link_libraries(openjtalk_native ${OPENJTALK_LIBS} winmm)
    else()
        target_link_libraries(openjtalk_native ${OPENJTALK_LIBS} stdc++ winmm)
    endif()
elseif(APPLE)
    target_link_libraries(openjtalk_native ${OPENJTALK_LIBS}
        "-framework CoreFoundation"
        "-liconv"
        "-lc++"
        m
    )
else()
    find_package(Threads REQUIRED)
    target_link_libraries(openjtalk_native ${OPENJTALK_LIBS} stdc++ m Threads::Threads)
endif()

# Platform-specific output properties
if(WIN32)
    set_target_properties(openjtalk_native PROPERTIES
        PREFIX ""
        OUTPUT_NAME "openjtalk_native"
        SUFFIX ".dll"
    )
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
elseif(APPLE AND NOT IOS)
    set_target_properties(openjtalk_native PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME "openjtalk_native"
        SUFFIX ".dylib"
    )
elseif(UNIX AND NOT ANDROID AND NOT IOS)
    set_target_properties(openjtalk_native PROPERTIES
        PREFIX "lib"
        OUTPUT_NAME "openjtalk_native"
        SUFFIX ".so"
    )
endif()

# Installation rules
install(TARGETS openjtalk_native
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
    LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
    ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
)
install(FILES include/openjtalk_native.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include
)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(MSVC)
        target_compile_options(openjtalk_native PRIVATE /W4 /Od /Zi)
    else()
        target_compile_options(openjtalk_native PRIVATE -Wall -Wextra -g -O0)
    endif()
    target_compile_definitions(openjtalk_native PRIVATE ENABLE_DEBUG_LOG)
else()
    if(MSVC)
        target_compile_options(openjtalk_native PRIVATE /W3 /O2)
    else()
        target_compile_options(openjtalk_native PRIVATE -Wall -O3)
    endif()
    if(ANDROID)
        target_compile_definitions(openjtalk_native PRIVATE ENABLE_DEBUG_LOG)
    endif()
endif()

# Tests
if(BUILD_TESTS AND NOT ANDROID AND NOT IOS)
    add_subdirectory(test)
endif()

# Print build configuration
message(STATUS "Building openjtalk_native")
message(STATUS "  Platform: ${PLATFORM_NAME}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenJTalk root: ${OPENJTALK_ROOT}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Tests: ${BUILD_TESTS}")
