cmake_minimum_required(VERSION 3.20)
project(beast_cpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BEAST_WITH_ONNX "Enable ONNX Runtime neural network evaluation" OFF)
set(BEAST_ARCH_FLAGS "" CACHE STRING "Extra CPU architecture compile flags")

set(BEAST_SOURCES
    src/main.cpp
    src/engine/engine.cpp
    src/engine/uci_protocol.cpp
    src/infra/search_options.cpp
    src/heuristics/heuristic.cpp
    src/heuristics/classical_heuristic.cpp
    src/heuristics/random_heuristic.cpp
    src/heuristics/neural_network.cpp
    src/neural_networks/net_input_factory.cpp
    src/neural_networks/net_input_v1.cpp
    src/neural_networks/net_input_v2.cpp
)

add_executable(beast_cpp ${BEAST_SOURCES})

target_include_directories(
    beast_cpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/chess-library/include
)

if (MSVC)
    target_compile_options(beast_cpp PRIVATE /O2 /W4)
else()
    target_compile_options(beast_cpp PRIVATE -O3 -Wall -Wextra -Wpedantic)
endif()

if (BEAST_ARCH_FLAGS)
    if (WIN32)
        separate_arguments(BEAST_ARCH_FLAGS_LIST WINDOWS_COMMAND "${BEAST_ARCH_FLAGS}")
    else()
        separate_arguments(BEAST_ARCH_FLAGS_LIST UNIX_COMMAND "${BEAST_ARCH_FLAGS}")
    endif()
    target_compile_options(beast_cpp PRIVATE ${BEAST_ARCH_FLAGS_LIST})
endif()

if (BEAST_WITH_ONNX)
    set(ONNXRUNTIME_HINTS "")
    if (DEFINED ENV{ONNXRUNTIME_ROOT})
        list(APPEND ONNXRUNTIME_HINTS "$ENV{ONNXRUNTIME_ROOT}")
    endif()

    set(BUNDLED_ONNXRUNTIME_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../external/onnxruntime/onnxruntime-win-x64-1.20.1")
    if (EXISTS "${BUNDLED_ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h")
        list(APPEND ONNXRUNTIME_HINTS "${BUNDLED_ONNXRUNTIME_ROOT}")
    endif()

    find_path(
        ONNXRUNTIME_INCLUDE_DIR
        onnxruntime_cxx_api.h
        HINTS ${ONNXRUNTIME_HINTS}
        PATH_SUFFIXES include
    )
    find_library(
        ONNXRUNTIME_LIB
        NAMES onnxruntime
        HINTS ${ONNXRUNTIME_HINTS}
        PATH_SUFFIXES lib lib64
    )

    if (NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIB)
        message(FATAL_ERROR "ONNX Runtime not found. Set ONNXRUNTIME_ROOT to an install prefix.")
    endif()

    target_include_directories(beast_cpp PRIVATE ${ONNXRUNTIME_INCLUDE_DIR})
    target_link_libraries(beast_cpp PRIVATE ${ONNXRUNTIME_LIB})
    target_compile_definitions(beast_cpp PRIVATE BEAST_WITH_ONNX=1)

    if (WIN32)
        find_file(
            ONNXRUNTIME_DLL
            NAMES onnxruntime.dll
            HINTS ${ONNXRUNTIME_HINTS}
            PATH_SUFFIXES lib bin
        )
        if (ONNXRUNTIME_DLL)
            add_custom_command(
                TARGET beast_cpp
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ONNXRUNTIME_DLL} $<TARGET_FILE_DIR:beast_cpp>
            )
        endif()
    endif()
endif()
