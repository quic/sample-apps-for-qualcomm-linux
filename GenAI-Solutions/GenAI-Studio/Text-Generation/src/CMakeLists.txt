#==============================================================================
#
#  Copyright (c) Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#
#==============================================================================

cmake_minimum_required(VERSION 3.18.2)
project(llamachat)

# Set the application name
set(APP "llamachat")

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)

# Add the executable
add_executable(${APP} Main.cpp PromptHandler.cpp ChatApp.cpp)

# Set chipset and QNN paths
set(CHIPSET "QCS9075")
set(QNN_ROOT "/app/Text-Generation/src/qairt/2.38.0.250901/")
set(QNN_INCLUDE_DIR "${QNN_ROOT}/include/Genie")
set(QNN_LIB_PREFIX "${QNN_ROOT}/lib")
set(_dtuple_POSTFIX "oe-linux-gcc11.2")
set(CROW_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/crow)
set(ASIO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/asio/asio/)


# Get the real path of the QNN library
get_filename_component(QNN_LIB_PATH "${QNN_LIB_PREFIX}/aarch64-${_dtuple_POSTFIX}/libGenie.so" REALPATH BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Add the QNN library as an imported target
add_library(QNN SHARED IMPORTED)
set_target_properties(QNN PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${QNN_INCLUDE_DIR}
    IMPORTED_LOCATION ${QNN_LIB_PATH}
)

# Include directories
include_directories(${QNN_INCLUDE_DIR})
include_directories(${CROW_SDK_ROOT}/include)
include_directories(${ASIO_ROOT}/include)

# Link the QNN library to the application
target_link_libraries(${APP} PUBLIC QNN)

