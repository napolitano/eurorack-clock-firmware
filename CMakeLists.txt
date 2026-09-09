# CLOCK native simulator build.
# Author: Axel Napolitano
# License: PolyForm-Noncommercial-1.0.0
cmake_minimum_required(VERSION 3.20)
project(clock_native_simulator LANGUAGES C CXX)

option(CLOCK_BUILD_SIMULATOR "Build the native host simulator" ON)
option(CLOCK_SIMULATOR_WITH_SDL "Build the interactive SDL3 simulator frontend" ON)
option(CLOCK_SIMULATOR_FETCH_SDL "Fetch SDL3 when no system package should be used" OFF)

if(NOT CLOCK_BUILD_SIMULATOR)
    return()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    set(CLOCK_WARNING_FLAGS
        /W4
        /WX
        /permissive-
    )
else()
    set(CLOCK_WARNING_FLAGS
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wsign-conversion
        -Werror
    )
endif()

file(GLOB_RECURSE CLOCK_FIRMWARE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/clock_core/src/*.cpp"
)
list(FILTER CLOCK_FIRMWARE_SOURCES EXCLUDE REGEX "/src/main\\.cpp$")
list(FILTER CLOCK_FIRMWARE_SOURCES EXCLUDE REGEX "/src/native_smoke_main\\.cpp$")

add_library(clock-simulator-firmware STATIC ${CLOCK_FIRMWARE_SOURCES})
target_compile_definitions(clock-simulator-firmware PUBLIC
    CLOCK_HOST_TEST=1
    CLOCK_SIMULATOR=1
)
target_include_directories(clock-simulator-firmware PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/clock_core/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
    "${CMAKE_CURRENT_SOURCE_DIR}/sim/framework"
)
target_compile_options(clock-simulator-firmware PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-simulator-runtime STATIC
    sim/simulator_runtime.cpp
    sim/simulator_runtime_inputs.cpp
    sim/virtual_input_signal.cpp
    sim/simulator_persistence.cpp
    sim/scope_timeline.cpp
    sim/scope_session.cpp
)
target_link_libraries(clock-simulator-runtime PUBLIC clock-simulator-firmware)
target_compile_definitions(clock-simulator-runtime PUBLIC
    CLOCK_HOST_TEST=1
    CLOCK_SIMULATOR=1
)
target_include_directories(clock-simulator-runtime PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
    "${CMAKE_CURRENT_SOURCE_DIR}/sim/framework"
)
target_compile_options(clock-simulator-runtime PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-simulator-headless sim/headless_main.cpp)
target_link_libraries(clock-simulator-headless PRIVATE clock-simulator-runtime)
target_compile_options(clock-simulator-headless PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-manual-screenshot-generator test/tools/manual_screenshot_generator.cpp)
target_link_libraries(clock-manual-screenshot-generator PRIVATE clock-simulator-firmware)
target_compile_options(clock-manual-screenshot-generator PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-simulator-tests
    sim/simulator_tests.cpp
    sim/simulator_contract_tests.cpp
    sim/panel_layout.cpp
    sim/panel_layout_ini.cpp
)
target_link_libraries(clock-simulator-tests PRIVATE clock-simulator-runtime)
target_compile_options(clock-simulator-tests PRIVATE ${CLOCK_WARNING_FLAGS})

if(CLOCK_SIMULATOR_WITH_SDL)
    if(CLOCK_SIMULATOR_FETCH_SDL)
        include(FetchContent)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.4.16
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(SDL3)
    else()
        find_package(SDL3 REQUIRED CONFIG COMPONENTS SDL3)
    endif()

    add_executable(clock-simulator
        sim/main.cpp
        sim/panel_layout.cpp
        sim/panel_layout_ini.cpp
        sim/panel_renderer.cpp
        sim/panel_renderer_scope.cpp
        sim/simulator_input.cpp
    )
    target_link_libraries(clock-simulator PRIVATE clock-simulator-runtime SDL3::SDL3)
    target_compile_options(clock-simulator PRIVATE ${CLOCK_WARNING_FLAGS})

    add_test(
        NAME simulator_headless_screenshot
        COMMAND clock-simulator
            --layout "${CMAKE_CURRENT_SOURCE_DIR}/sim/panel_layout.ini"
            --state "${CMAKE_CURRENT_BINARY_DIR}/clock-screenshot-test-state.bin"
            --screenshot "${CMAKE_CURRENT_BINARY_DIR}/clock-simulator-test.bmp"
            --screenshot-after-ms 250
            --display-scale 2
            --no-developer
    )

    if(WIN32 AND TARGET SDL3::SDL3-shared)
        add_custom_command(
            TARGET clock-simulator POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                    $<TARGET_FILE:SDL3::SDL3-shared>
                    $<TARGET_FILE_DIR:clock-simulator>
            VERBATIM
        )
    endif()
endif()

enable_testing()
add_test(NAME simulator_headless_smoke COMMAND clock-simulator-headless)
add_test(NAME simulator_runtime_tests COMMAND clock-simulator-tests)
