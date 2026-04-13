cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

project(VEFontCache LANGUAGES C CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(VEFC_BUILD_DEMO "Build the OpenGL demo." ON)
option(VEFC_BUILD_DX11 "Build the Direct3D 11 demo." ON)
option(VEFC_BUILD_TESTS "Build the unit tests." ON)
option(VEFC_ENABLE_HARFBUZZ "Enable HarfBuzz-backed shaping support." ON)
option(VEFC_ENABLE_FREETYPE "Enable FreeType CPU rasterisation support." ON)
option(VEFC_BUILD_VRHI "Build VRHI backend demo." OFF)
set(VRHI_VERSION "v0.1.4" CACHE STRING "VRHI release version to fetch")

include(FetchContent)

if(VEFC_ENABLE_FREETYPE)
    set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        freetype
        GIT_REPOSITORY https://github.com/freetype/freetype.git
        GIT_TAG VER-2-13-3
    )
    if(POLICY CMP0077)
        cmake_policy(SET CMP0077 NEW)
    endif()
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
    FetchContent_MakeAvailable(freetype)
    unset(CMAKE_POLICY_DEFAULT_CMP0091)
endif()

if(VEFC_ENABLE_HARFBUZZ)
    set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
    FetchContent_Declare(
        harfbuzz
        GIT_REPOSITORY https://github.com/harfbuzz/harfbuzz.git
        GIT_TAG 11.2.1
    )
    set(HB_BUILD_SUBSET OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(harfbuzz)
    unset(CMAKE_POLICY_DEFAULT_CMP0091)
endif()

if(VEFC_BUILD_DEMO OR VEFC_BUILD_VRHI)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

if(VEFC_BUILD_VRHI)
    find_package(Vulkan REQUIRED)

    if(WIN32)
        set(VRHI_PLATFORM "windows-x64")
    elseif(UNIX AND NOT APPLE)
        set(VRHI_PLATFORM "linux-x64")
    elseif(APPLE)
        set(VRHI_PLATFORM "macos-arm64")
    endif()

    FetchContent_Declare(
        vrhi
        URL "https://github.com/hypernewbie/vrhi/releases/download/${VRHI_VERSION}/vrhi-${VRHI_VERSION}-${VRHI_PLATFORM}.zip"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/vrhi-src"
    )
    FetchContent_Declare(
        vrhi_debug
        URL "https://github.com/hypernewbie/vrhi/releases/download/${VRHI_VERSION}/vrhi-${VRHI_VERSION}-${VRHI_PLATFORM}-debug.zip"
        SOURCE_DIR "${CMAKE_BINARY_DIR}/_deps/vrhi-debug-src"
    )
    FetchContent_MakeAvailable(vrhi vrhi_debug)

    set(VEFC_VRHI_RELEASE_ROOT "${CMAKE_BINARY_DIR}/_deps/vrhi-src")
    set(VEFC_VRHI_DEBUG_ROOT   "${CMAKE_BINARY_DIR}/_deps/vrhi-debug-src")
    set(VEFC_VRHI_INCLUDE_DIR "${VEFC_VRHI_RELEASE_ROOT}/include")
    set(VEFC_VRHI_RELEASE_LIB_DIR "${VEFC_VRHI_RELEASE_ROOT}/lib")
    set(VEFC_VRHI_DEBUG_LIB_DIR   "${VEFC_VRHI_DEBUG_ROOT}/lib")

    function(vefc_vrhi_find_library out_var lib_dir)
        cmake_parse_arguments(ARG "" "" "NAMES" ${ARGN})
        if(NOT ARG_NAMES)
            message(FATAL_ERROR "vefc_vrhi_find_library requires at least one library name.")
        endif()

        unset(_vefc_vrhi_library CACHE)
        unset(_vefc_vrhi_library)
        find_library(_vefc_vrhi_library
            NAMES ${ARG_NAMES}
            HINTS "${lib_dir}"
            NO_DEFAULT_PATH
        )
        if(NOT _vefc_vrhi_library)
            message(FATAL_ERROR "Could not find any of [${ARG_NAMES}] in ${lib_dir}")
        endif()

        set(${out_var} "${_vefc_vrhi_library}" PARENT_SCOPE)
    endfunction()

    function(vefc_add_configured_imported_library target_name release_path debug_path)
        add_library(${target_name} UNKNOWN IMPORTED)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION_DEBUG "${debug_path}"
            IMPORTED_LOCATION_RELEASE "${release_path}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${release_path}"
            IMPORTED_LOCATION_MINSIZEREL "${release_path}"
        )
    endfunction()

    foreach(_vefc_vrhi_lib IN ITEMS vrhi nvrhi nvrhi_vk vk-bootstrap rtxmu SPIRV-Tools SPIRV-Tools-opt)
        string(REPLACE "-" "_" _vefc_vrhi_var "${_vefc_vrhi_lib}")
        vefc_vrhi_find_library(VEFC_VRHI_${_vefc_vrhi_var}_RELEASE "${VEFC_VRHI_RELEASE_LIB_DIR}" NAMES "${_vefc_vrhi_lib}")
        vefc_vrhi_find_library(VEFC_VRHI_${_vefc_vrhi_var}_DEBUG "${VEFC_VRHI_DEBUG_LIB_DIR}" NAMES "${_vefc_vrhi_lib}")
        vefc_add_configured_imported_library(
            vefc_vrhi_${_vefc_vrhi_var}
            "${VEFC_VRHI_${_vefc_vrhi_var}_RELEASE}"
            "${VEFC_VRHI_${_vefc_vrhi_var}_DEBUG}"
        )
    endforeach()

    get_filename_component(VULKAN_SDK_LIB_DIR "${Vulkan_LIBRARY}" DIRECTORY)
    set(VEFC_SLANG_HINT_DIRS
        "${VULKAN_SDK_LIB_DIR}"
        "${VULKAN_SDK_LIB_DIR}/../lib"
        "${VULKAN_SDK_LIB_DIR}/../lib64"
        "${VULKAN_SDK_LIB_DIR}/../Bin"
        "${VULKAN_SDK_LIB_DIR}/../bin"
        "${VULKAN_SDK_LIB_DIR}/../macOS/lib"
    )
    list(REMOVE_DUPLICATES VEFC_SLANG_HINT_DIRS)

    find_library(SLANG_LIBRARY
        NAMES slang
        HINTS ${VEFC_SLANG_HINT_DIRS}
        NO_DEFAULT_PATH
    )
    if(NOT SLANG_LIBRARY)
        message(FATAL_ERROR "Could not find Slang in Vulkan SDK hints: ${VEFC_SLANG_HINT_DIRS}")
    endif()

    set(VEFC_SLANG_RPATH_DIRS)
    get_filename_component(SLANG_LIBRARY_DIR "${SLANG_LIBRARY}" DIRECTORY)
    list(APPEND VEFC_SLANG_RPATH_DIRS "${SLANG_LIBRARY_DIR}")

    find_library(SLANG_RT_LIBRARY
        NAMES slang-rt
        HINTS ${VEFC_SLANG_HINT_DIRS}
        NO_DEFAULT_PATH
    )
    if(SLANG_RT_LIBRARY)
        get_filename_component(SLANG_RT_LIBRARY_DIR "${SLANG_RT_LIBRARY}" DIRECTORY)
        list(APPEND VEFC_SLANG_RPATH_DIRS "${SLANG_RT_LIBRARY_DIR}")
    endif()
    list(REMOVE_DUPLICATES VEFC_SLANG_RPATH_DIRS)

    if(WIN32)
        find_file(SLANG_DLL
            NAMES slang.dll
            HINTS "${VULKAN_SDK_LIB_DIR}/../Bin" "${VULKAN_SDK_LIB_DIR}/../bin"
            NO_DEFAULT_PATH
        )
        find_file(SLANG_RT_DLL
            NAMES slang-rt.dll
            HINTS "${VULKAN_SDK_LIB_DIR}/../Bin" "${VULKAN_SDK_LIB_DIR}/../bin"
            NO_DEFAULT_PATH
        )
        if(NOT SLANG_DLL OR NOT SLANG_RT_DLL)
            message(FATAL_ERROR "Could not find slang.dll and slang-rt.dll in Vulkan SDK runtime directories.")
        endif()
    endif()

    message(STATUS "VRHI Release: ${VEFC_VRHI_RELEASE_ROOT}")
    message(STATUS "VRHI Debug:   ${VEFC_VRHI_DEBUG_ROOT}")
    message(STATUS "Using Slang: ${SLANG_LIBRARY}")
endif()

function(vefc_apply_common_options target)
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    if(WIN32)
        target_compile_definitions(${target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    endif()

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
    vefc_apply_common_options(demo)
    vefc_apply_features(demo ${VEFC_ENABLE_HARFBUZZ} ${VEFC_ENABLE_FREETYPE})
    target_link_libraries(demo PRIVATE glfw)
    if(WIN32)
        target_link_libraries(demo PRIVATE opengl32)
    elseif(APPLE)
        find_package(OpenGL REQUIRED)
        target_link_libraries(demo PRIVATE OpenGL::GL)
    elseif(UNIX)
        find_package(OpenGL REQUIRED)
        target_link_libraries(demo PRIVATE OpenGL::GL)
    endif()
endif()

if(WIN32 AND VEFC_BUILD_DX11)
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

if(VEFC_BUILD_VRHI)
    add_executable(vefc_demo_vrhi
        demo/demo_vrhi.cpp
    )
    set_target_properties(vefc_demo_vrhi PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo"
    )

    target_include_directories(vefc_demo_vrhi PRIVATE
        ${VEFC_VRHI_INCLUDE_DIR}
    )

    vefc_apply_common_options(vefc_demo_vrhi)
    vefc_apply_features(vefc_demo_vrhi ${VEFC_ENABLE_HARFBUZZ} ${VEFC_ENABLE_FREETYPE})

    target_link_libraries(vefc_demo_vrhi PRIVATE
        vefc_vrhi_vrhi
        vefc_vrhi_nvrhi
        vefc_vrhi_nvrhi_vk
        vefc_vrhi_vk_bootstrap
        vefc_vrhi_rtxmu
        vefc_vrhi_SPIRV_Tools
        vefc_vrhi_SPIRV_Tools_opt
        ${SLANG_LIBRARY}
        Vulkan::Vulkan
        glfw
    )

    if(UNIX)
        set_target_properties(vefc_demo_vrhi PROPERTIES
            BUILD_RPATH "${VEFC_SLANG_RPATH_DIRS}"
        )
    endif()

    if(WIN32)
        target_link_libraries(vefc_demo_vrhi PRIVATE user32 gdi32)

        add_custom_command(TARGET vefc_demo_vrhi POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SLANG_DLL}
                $<TARGET_FILE_DIR:vefc_demo_vrhi>/
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${SLANG_RT_DLL}
                $<TARGET_FILE_DIR:vefc_demo_vrhi>/
        )
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(vefc_demo_vrhi PRIVATE dl pthread xcb X11 Xrandr Xi)
    endif()
endif()

set(VEFC_DEMO_BACKEND_TEST_TARGETS)
if(TARGET demo)
    list(APPEND VEFC_DEMO_BACKEND_TEST_TARGETS demo)
endif()
if(TARGET vefc_demo_dx11)
    list(APPEND VEFC_DEMO_BACKEND_TEST_TARGETS vefc_demo_dx11)
endif()
if(TARGET vefc_demo_vrhi)
    list(APPEND VEFC_DEMO_BACKEND_TEST_TARGETS vefc_demo_vrhi)
endif()

set(VEFC_TESTING_ENABLED OFF)
if(VEFC_BUILD_TESTS OR VEFC_DEMO_BACKEND_TEST_TARGETS)
    enable_testing()
    set(VEFC_TESTING_ENABLED ON)
endif()

if(VEFC_TESTING_ENABLED)
    function(vefc_add_demo_backend_test test_name target_name)
        if(NOT TARGET ${target_name})
            return()
        endif()

        add_test(
            NAME ${test_name}
            COMMAND ${target_name} --test
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/demo"
        )
        set_tests_properties(${test_name} PROPERTIES
            LABELS "demo-backend"
            SKIP_RETURN_CODE 125
        )
    endfunction()

    vefc_add_demo_backend_test(vefc_demo_gl_test demo)
    vefc_add_demo_backend_test(vefc_demo_dx11_test vefc_demo_dx11)
    vefc_add_demo_backend_test(vefc_demo_vrhi_test vefc_demo_vrhi)

    if(CMAKE_CONFIGURATION_TYPES)
        add_custom_target(vefc_demo_backend_tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L demo-backend -C $<CONFIG>
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            DEPENDS ${VEFC_DEMO_BACKEND_TEST_TARGETS}
            USES_TERMINAL
        )
    else()
        add_custom_target(vefc_demo_backend_tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L demo-backend
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            DEPENDS ${VEFC_DEMO_BACKEND_TEST_TARGETS}
            USES_TERMINAL
        )
    endif()
endif()

if(VEFC_BUILD_TESTS)

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
