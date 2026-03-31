cmake_minimum_required(VERSION 3.21)

project(gui_sync_visualizer LANGUAGES C CXX)

# FetchContent is used here so the GUI project does not depend on a hardcoded
# local checkout of gdbus-learn.
include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_QT_GUI "Build the Qt GUI application" ON)
option(ENABLE_TESTS "Build local tests" ON)

# The dependency is pinned so the GUI build stays reproducible.
set(GDBUS_LEARN_GIT_REPOSITORY "https://github.com/SweerItTer/gdbus-learn.git" CACHE STRING
    "Git repository for the gdbus-learn dependency.")
set(GDBUS_LEARN_GIT_TAG "94611e503f2cfcec045cf0bf2ba0fa62bebd16cc" CACHE STRING
    "Git revision for the gdbus-learn dependency.")

# Local helper library for coordinate math and file-selection state.
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

    # The unit test target covers the pure C++ helpers without Qt.
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

# Qt MOC is needed because the bridge and windows use QObject signals/slots.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Pull the upstream project sources at configure time.
FetchContent_Declare(
    gdbus_learn_src
    GIT_REPOSITORY "${GDBUS_LEARN_GIT_REPOSITORY}"
    GIT_TAG "${GDBUS_LEARN_GIT_TAG}"
    GIT_SHALLOW TRUE
)
FetchContent_GetProperties(gdbus_learn_src)
if(NOT gdbus_learn_src_POPULATED)
    FetchContent_Populate(gdbus_learn_src)
endif()

# Everything below is built from the fetched gdbus-learn source tree.
set(GDBUS_TEST_ROOT "${gdbus_learn_src_SOURCE_DIR}")

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

# Re-run gdbus-codegen locally so this project can build the same interface
# bindings without depending on another build directory.
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

# These support libraries come from gdbus-learn and are rebuilt in this project.
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

# The demo service executable lets the GUI run standalone for local verification.
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

# The GUI links against the same client wrapper used by the CLI sample.
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

# Main GUI target.
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
