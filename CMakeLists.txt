cmake_minimum_required(VERSION 3.21)

project(gui_sync_visualizer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_QT_GUI "Build the Qt GUI application" ON)
option(ENABLE_TESTS "Build local tests" ON)

set(GDBUS_TEST_ROOT "/home/mouj/projects/gdbus-test" CACHE PATH "Path to the gdbus-test workspace")

add_library(sync_math STATIC
    src/SyncMath.cpp
    src/FileTransferState.cpp
)

target_include_directories(sync_math
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if(ENABLE_TESTS)
    enable_testing()

    add_executable(sync_math_tests
        tests/sync_math_tests.cpp
    )

    target_link_libraries(sync_math_tests
        PRIVATE
            sync_math
    )

    add_test(NAME sync_math_tests COMMAND sync_math_tests)
endif()

if(NOT ENABLE_QT_GUI)
    return()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

foreach(required_path
    "${GDBUS_TEST_ROOT}/Common"
    "${GDBUS_TEST_ROOT}/ClientProject"
    "${GDBUS_TEST_ROOT}/ServiceProject"
    "${GDBUS_TEST_ROOT}/ServiceProject/Library"
)
    if(NOT EXISTS "${required_path}")
        message(FATAL_ERROR "Missing required gdbus-test path: ${required_path}")
    endif()
endforeach()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0 glib-2.0 gobject-2.0)
find_program(GDBUS_CODEGEN_EXECUTABLE gdbus-codegen REQUIRED)

set(GDBUS_COMMON_INCLUDE "${GDBUS_TEST_ROOT}/Common/Include")
set(GDBUS_CLIENT_INCLUDE "${GDBUS_TEST_ROOT}/ClientProject/Include")
set(GDBUS_SERVICE_INCLUDE "${GDBUS_TEST_ROOT}/ServiceProject/Include")
set(GDBUS_SERVICE_LIBRARY_INCLUDE "${GDBUS_TEST_ROOT}/ServiceProject/Library/Include")
set(GDBUS_XML "${GDBUS_TEST_ROOT}/Common/Interfaces/com.example.Training.xml")

set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(GENERATED_BASENAME "${GENERATED_DIR}/training-generated")
set(GENERATED_C "${GENERATED_BASENAME}.c")
set(GENERATED_H "${GENERATED_BASENAME}.h")

add_custom_command(
    OUTPUT
        "${GENERATED_C}"
        "${GENERATED_H}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"
    COMMAND
        "${GDBUS_CODEGEN_EXECUTABLE}"
        --interface-prefix com.example.
        --generate-c-code "${GENERATED_BASENAME}"
        "${GDBUS_XML}"
    DEPENDS
        "${GDBUS_XML}"
    COMMENT "Generating GDBus bindings"
)

add_library(training_common INTERFACE)
target_include_directories(training_common
    INTERFACE
        "${GDBUS_COMMON_INCLUDE}"
)

add_library(training_utils STATIC
    "${GDBUS_TEST_ROOT}/Common/Sources/utils/ScopedBusConnection.cpp"
    "${GDBUS_TEST_ROOT}/Common/Sources/utils/ScopedBusNameOwner.cpp"
)

set_target_properties(training_utils PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(training_utils
    PUBLIC
        "${GDBUS_COMMON_INCLUDE}"
)

target_link_libraries(training_utils
    PUBLIC
        PkgConfig::GLIB
)

add_library(gdbus_generated STATIC "${GENERATED_C}")
set_source_files_properties("${GENERATED_C}" PROPERTIES LANGUAGE C)

target_include_directories(gdbus_generated
    PUBLIC
        "${GENERATED_DIR}"
)

target_link_libraries(gdbus_generated
    PUBLIC
        PkgConfig::GLIB
)

add_library(core STATIC
    "${GDBUS_TEST_ROOT}/ServiceProject/Sources/TrainingService.cpp"
)

set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(core
    PUBLIC
        "${GDBUS_SERVICE_INCLUDE}"
)

target_link_libraries(core
    PUBLIC
        training_common
        training_utils
        gdbus_generated
)

add_library(training SHARED
    "${GDBUS_TEST_ROOT}/ServiceProject/Library/Sources/TrainingLibrary.cpp"
)

target_include_directories(training
    PUBLIC
        "${GDBUS_SERVICE_LIBRARY_INCLUDE}"
)

target_link_libraries(training
    PUBLIC
        training_common
        training_utils
        gdbus_generated
)

add_library(training_client_wrapper STATIC
    "${GDBUS_TEST_ROOT}/ClientProject/Sources/client/TrainingClient.cpp"
)

target_include_directories(training_client_wrapper
    PUBLIC
        "${GDBUS_CLIENT_INCLUDE}"
)

target_compile_definitions(training_client_wrapper
    PRIVATE
        TRAINING_LIBRARY_PATH="$<TARGET_FILE:training>"
)

target_link_libraries(training_client_wrapper
    PUBLIC
        training_common
    PRIVATE
        ${CMAKE_DL_LIBS}
)

add_dependencies(training_client_wrapper training)

add_executable(training_service_gui_demo
    "${GDBUS_TEST_ROOT}/ServiceProject/app/service_main.cpp"
)

target_link_libraries(training_service_gui_demo
    PRIVATE
        core
)

add_executable(sync_visualizer
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.hpp
    src/TestInfoCanvas.cpp
    src/TestInfoCanvas.hpp
    src/TrainingClientBridge.cpp
    src/TrainingClientBridge.hpp
    src/SyncMath.hpp
)

target_link_libraries(sync_visualizer
    PRIVATE
        Qt6::Widgets
        sync_math
        training_client_wrapper
)
