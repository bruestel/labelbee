cmake_minimum_required(VERSION 3.16.0)

# The version stamped into the app descriptor, worked out here rather than left to ESP-IDF: its own
# `git describe` fails the configure step in a repository without commits.
set(PROJECT_VER "0.0.0-dev")
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS ${CMAKE_CURRENT_LIST_DIR}/.git)
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty
                    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
                    OUTPUT_VARIABLE LABELBEE_VERSION
                    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    if(LABELBEE_VERSION)
        string(REGEX REPLACE "^v([0-9])" "\\1" LABELBEE_VERSION "${LABELBEE_VERSION}")
        set(PROJECT_VER "${LABELBEE_VERSION}")
    endif()
endif()
message(STATUS "Firmware version: ${PROJECT_VER}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# project_name in the app descriptor: a later OTA handler will refuse images whose name differs.
# Do not rename it once devices run this firmware.
project(labelbee)
