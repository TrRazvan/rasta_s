set(LIB_NAME common)

option(STATIC_LIB "build static or shared lib" ON)
message("-- library ${LIB_NAME}: STATIC_LIB=${STATIC_LIB}")
if (${STATIC_LIB})
    add_library(${LIB_NAME} STATIC)
else()
    add_library(${LIB_NAME} SHARED)
endif()

target_sources(${LIB_NAME} PRIVATE 
    src/log.c
)

target_include_directories(${LIB_NAME} PRIVATE src)

# Additional properties.
#set_property(TARGET ${LIB_NAME} PROPERTY COMPILE_WARNING_AS_ERROR ON)
#target_compile_options(${LIB_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)