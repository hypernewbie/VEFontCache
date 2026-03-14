cmake_minimum_required(VERSION 3.21)

project(VEFontCache LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(VEFC_BUILD_DEMO "Build the OpenGL demo." ON)
option(VEFC_BUILD_TESTS "Build the unit tests." ON)
option(VEFC_ENABLE_HARFBUZZ "Enable HarfBuzz-backed shaping support." ON)
option(VEFC_ENABLE_FREETYPE "Enable FreeType CPU rasterisation support." ON)

include(FetchContent)

if(VEFC_ENABLE_FREETYPE)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-13-3
    )
    if(POLICY CMP0077)
        cmake_policy(SET CMP0077 NEW)
    endif()
    FetchContent_MakeAvailable(freetype)
endif()

if(VEFC_ENABLE_HARFBUZZ)
    FetchContent_Declare(
        harfbuzz
        GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
        GIT_TAG 11.2.1
    )
    set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(harfbuzz)
endif()

function(vefc_apply_common_options target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if(MSVC OR (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"))
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
endfunction()

function(vefc_apply_features target use_harfbuzz use_freetype)
    if(${use_harfbuzz})
        if(NOT TARGET harfbuzz)
            message(FATAL_ERROR "Target ${target} requested HarfBuzz, but VEFC_ENABLE_HARFBUZZ is OFF.")
        endif()

        target_compile_definitions(${target} PRIVATE VE_FONTCACHE_HARFBUZZ)
        target_include_directories(${target} PRIVATE ${harfbuzz_SOURCE_DIR}/src)
        target_link_libraries(${target} PRIVATE harfbuzz)
    endif()

    if(${use_freetype})
        if(NOT TARGET freetype)
            message(FATAL_ERROR "Target ${target} requested FreeType, but VEFC_ENABLE_FREETYPE is OFF.")
        endif()

        target_compile_definitions(${target} PRIVATE VE_FONTCACHE_FREETYPE_RASTERISATION)
        target_include_directories(${target} PRIVATE
            ${freetype_SOURCE_DIR}/include
            ${freetype_BINARY_DIR}/include
        )
        target_link_libraries(${target} PRIVATE freetype)
    endif()
endfunction()

if(VEFC_BUILD_DEMO)
    add_executable(demo
        demo/demo.cpp
    )
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT demo)
    set_target_properties(demo PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo"
    )
    target_include_directories(demo PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/demo/include
        ${CMAKE_CURRENT_SOURCE_DIR}/demo/src
    )
    target_link_libraries(demo PRIVATE glu32)
    vefc_apply_common_options(demo)
    vefc_apply_features(demo ${VEFC_ENABLE_HARFBUZZ} ${VEFC_ENABLE_FREETYPE})

    if(WIN32)
        add_executable(vefc_demo_dx11
            demo/demo_dx11.cpp
        )
        set_target_properties(vefc_demo_dx11 PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo"
        )
        target_link_libraries(vefc_demo_dx11 PRIVATE
            d3d11
            d3dcompiler
            dxgi
            user32
        )
        vefc_apply_common_options(vefc_demo_dx11)
        vefc_apply_features(vefc_demo_dx11 ${VEFC_ENABLE_HARFBUZZ} ${VEFC_ENABLE_FREETYPE})
    endif()
endif()

if(VEFC_BUILD_TESTS)
    enable_testing()

    set(VEFC_TEST_SOURCES
        tests/test_main.cpp
    )

    function(vefc_add_test_target target use_harfbuzz use_freetype)
        add_executable(${target} ${VEFC_TEST_SOURCES})
        set_target_properties(${target} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo"
        )
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        vefc_apply_common_options(${target})
        vefc_apply_features(${target} ${use_harfbuzz} ${use_freetype})
        add_test(
            NAME ${target}
            COMMAND ${target}
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo"
        )
    endfunction()

    vefc_add_test_target(vefc_tests_base OFF OFF)

    if(VEFC_ENABLE_HARFBUZZ)
        vefc_add_test_target(vefc_tests_hb ON OFF)
    endif()

    if(VEFC_ENABLE_FREETYPE)
        vefc_add_test_target(vefc_tests_ft OFF ON)
    endif()

    if(VEFC_ENABLE_HARFBUZZ AND VEFC_ENABLE_FREETYPE)
        vefc_add_test_target(vefc_tests_hb_ft ON ON)
    endif()
endif()
