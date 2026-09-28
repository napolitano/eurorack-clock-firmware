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
    sim/panel_led_visual.cpp
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

add_library(clock-simulator-panel-layout STATIC
    sim/panel_layout.cpp
    sim/panel_layout_ini.cpp
)
target_include_directories(clock-simulator-panel-layout PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-simulator-panel-layout PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-storybook-contract STATIC
    sim/tutorial/story_contract.cpp
)
target_include_directories(clock-storybook-contract PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-storybook-contract PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-storybook-model STATIC
    sim/tutorial/story_model.cpp
    sim/tutorial/story_yaml.cpp
    sim/tutorial/story_parser.cpp
    sim/tutorial/story_validator.cpp
    sim/tutorial/interaction_profile_loader.cpp
    sim/tutorial/story_theme.cpp
)
target_link_libraries(clock-storybook-model PUBLIC clock-storybook-contract)
target_include_directories(clock-storybook-model PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-storybook-model PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-storybook-simulator-port STATIC
    sim/tutorial/story_simulator_port.cpp
)
target_link_libraries(clock-storybook-simulator-port PUBLIC
    clock-storybook-contract
    clock-simulator-runtime
)
target_include_directories(clock-storybook-simulator-port PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-storybook-simulator-port PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-storybook-runner STATIC
    sim/tutorial/story_runner.cpp
)
target_link_libraries(clock-storybook-runner PUBLIC
    clock-storybook-model
    clock-storybook-simulator-port
)
target_include_directories(clock-storybook-runner PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-storybook-runner PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-storybook-presentation STATIC
    sim/tutorial/panel_presentation.cpp
    sim/tutorial/panel_dynamic_layer.cpp
)

add_library(clock-storybook-renderer STATIC
    sim/tutorial/tutorial_surface.cpp
    sim/tutorial/story_text_renderer.cpp
    sim/tutorial/tutorial_renderer.cpp
)
target_link_libraries(clock-storybook-renderer PUBLIC
    clock-storybook-presentation
)
target_include_directories(clock-storybook-renderer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-storybook-renderer PRIVATE ${CLOCK_WARNING_FLAGS})
target_link_libraries(clock-storybook-presentation PUBLIC
    clock-storybook-runner
    clock-simulator-panel-layout
)
target_include_directories(clock-storybook-presentation PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
)
target_compile_options(clock-storybook-presentation PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-storybook-contract-tests
    sim/storybook_contract_tests.cpp
)
target_link_libraries(clock-storybook-contract-tests PRIVATE clock-storybook-contract)
target_compile_options(clock-storybook-contract-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-storybook-simulator-port-tests
    sim/storybook_simulator_port_tests.cpp
)
target_link_libraries(clock-storybook-simulator-port-tests PRIVATE clock-storybook-simulator-port)
target_compile_options(clock-storybook-simulator-port-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-storybook-parser-tests
    sim/storybook_parser_tests.cpp
)
target_link_libraries(clock-storybook-parser-tests PRIVATE clock-storybook-model)
target_compile_definitions(clock-storybook-parser-tests PRIVATE
    CLOCK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_options(clock-storybook-parser-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-storybook-runner-tests
    sim/storybook_runner_tests.cpp
)
target_link_libraries(clock-storybook-runner-tests PRIVATE clock-storybook-runner)
target_compile_definitions(clock-storybook-runner-tests PRIVATE
    CLOCK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_options(clock-storybook-runner-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-storybook-panel-presentation-tests
    sim/storybook_panel_presentation_tests.cpp
)
target_link_libraries(clock-storybook-panel-presentation-tests PRIVATE
    clock-storybook-presentation
)
target_compile_definitions(clock-storybook-panel-presentation-tests PRIVATE
    CLOCK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_options(clock-storybook-panel-presentation-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-storybook-renderer-tests
    sim/storybook_renderer_tests.cpp
)
target_link_libraries(clock-storybook-renderer-tests PRIVATE
    clock-storybook-renderer
)
target_compile_definitions(clock-storybook-renderer-tests PRIVATE
    CLOCK_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_options(clock-storybook-renderer-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_library(clock-vcv-runtime-adapter STATIC
    vcv/clock_vcv_runtime.cpp
)
target_link_libraries(clock-vcv-runtime-adapter PUBLIC clock-simulator-runtime)
target_compile_definitions(clock-vcv-runtime-adapter PUBLIC
    CLOCK_HOST_TEST=1
    CLOCK_SIMULATOR=1
)
target_include_directories(clock-vcv-runtime-adapter PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/vcv"
    "${CMAKE_CURRENT_SOURCE_DIR}/sim"
    "${CMAKE_CURRENT_SOURCE_DIR}/sim/framework"
)
target_compile_options(clock-vcv-runtime-adapter PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-vcv-runtime-tests
    vcv/tests/clock_vcv_runtime_tests.cpp
)
target_link_libraries(clock-vcv-runtime-tests PRIVATE clock-vcv-runtime-adapter)
target_compile_options(clock-vcv-runtime-tests PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-manual-screenshot-generator test/tools/manual_screenshot_generator.cpp)
target_link_libraries(clock-manual-screenshot-generator PRIVATE clock-simulator-firmware)
target_compile_options(clock-manual-screenshot-generator PRIVATE ${CLOCK_WARNING_FLAGS})

add_executable(clock-simulator-tests
    sim/simulator_tests.cpp
    sim/simulator_contract_tests.cpp
)
target_link_libraries(clock-simulator-tests PRIVATE
    clock-simulator-runtime
    clock-simulator-panel-layout
)
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
        sim/panel_renderer.cpp
        sim/panel_renderer_scope.cpp
        sim/simulator_input.cpp
    )
    target_link_libraries(clock-simulator PRIVATE clock-simulator-runtime clock-simulator-panel-layout SDL3::SDL3)
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
add_test(NAME storybook_contract_tests COMMAND clock-storybook-contract-tests)
add_test(NAME storybook_simulator_port_tests COMMAND clock-storybook-simulator-port-tests)
add_test(NAME storybook_parser_tests COMMAND clock-storybook-parser-tests)
add_test(NAME storybook_runner_tests COMMAND clock-storybook-runner-tests)
add_test(NAME storybook_panel_presentation_tests COMMAND clock-storybook-panel-presentation-tests)
add_test(NAME storybook_renderer_tests COMMAND clock-storybook-renderer-tests)
add_test(NAME simulator_runtime_tests COMMAND clock-simulator-tests)
add_test(NAME vcv_runtime_adapter_tests COMMAND clock-vcv-runtime-tests)
