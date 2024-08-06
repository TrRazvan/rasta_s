cmake_minimum_required(VERSION 3.10)

# Project name
project(rastaS C)

# Specify the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable in-source builds to prevent source tree corruption
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR '
        FATAL: In-source builds are not allowed.
            You should create a separate directory for build files.
    ')
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

LIST(APPEND CMAKE_PREFIX_PATH "/usr/local/include")
find_path(CMOCKA_INCLUDE_DIR
        NAMES
        cmocka.h
        PATHS
        ${CMOCKA_ROOT_DIR}/include
        )

find_library(CMOCKA_LIBRARY
        NAMES
        cmocka cmocka_shared
        PATHS
        ${CMOCKA_ROOT_DIR}/include
        )

if (CMOCKA_LIBRARY)
    set(CMOCKA_LIBRARIES
        ${CMOCKA_LIBRARIES}
        ${CMOCKA_LIBRARY}
        )
endif (CMOCKA_LIBRARY)

# Create an executable
add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${PROJECT_NAME} common mock safecom)

# Include directories
include_directories(
    common/include
    mock/include
    safecom/include
    ${CMOCKA_INCLUDE_DIR})

# Subdirectories 
add_subdirectory(common)
add_subdirectory(safecom)
add_subdirectory(mock)
add_subdirectory(test)

