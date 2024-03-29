cmake_minimum_required(VERSION 3.10)

project(MCServerUpdater VERSION 1.0.0 LANGUAGES C)

include_directories(deps/cjson
                    deps/curl/include)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING_STATIC OFF CACHE BOOL "" FORCE)
set(ENABLE_CJSON_TEST OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/cjson)
set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
set(GEN_FILES ON CACHE BOOL "" FORCE)
add_subdirectory(deps/mbedtls)
set(MBEDTLS_INCLUDE_DIRS deps/mbedtls/include CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARY mbedtls CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY mbedx509 CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY mbedcrypto CACHE STRING "" FORCE)
set(CURL_ENABLE_EXPORT_TARGET FALSE CACHE BOOL "" FORCE)
set(CURL_ENABLE_SSL TRUE CACHE BOOL "" FORCE)
set(CURL_USE_MBEDTLS TRUE CACHE BOOL "" FORCE)
add_subdirectory(deps/curl)

add_executable(MCServerUpdater MCServerUpdater.c)
target_link_libraries(MCServerUpdater PRIVATE cjson libcurl mbedtls)
set_directory_properties(PROPERTIES VS_STARTUP_PROJECT MCServerUpdater)
