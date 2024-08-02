cmake_minimum_required(VERSION 3.10)

# Project name
project(SafeComProject C)

# Specify the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/safecom.c
    src/safecom_impl.c
    src/rass.c
    src/sic.c
    src/iscdisp.c
    src/oscom.c
    src/fec.c
    src/md4.c
    src/pdu.c
    src/sm.c
    src/main.c
)

# Create an executable
add_executable(SafeComExecutable ${SOURCES})
