cmake_minimum_required(VERSION 3.16)

project(Haversine)
set(EXE_NAME "run")

set(SOURCES
    src/main.c
)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug, Release, RelWithDebInfo, MinSizeRel)" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BIN_DIR debug)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(BIN_DIR release)
endif()

add_executable(${EXE_NAME} ${SOURCES})
set_target_properties(${EXE_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
    COMPILE_FLAGS "/EHa /nologo /FC /Zo /WX /W4 /Gm- /wd5208 /wd4505 /wd4201 /DPLT_WIN /D_CRT_SECURE_NO_WARNINGS"
    COMPILE_FLAGS_DEBUG "/Od /MTd /Z7 /Zo /DDEBUG"
    COMPILE_FLAGS_RELEASE "/Od /Oi /MT /DRELEASE"
    LINK_FLAGS "/nologo"
    LINK_FLAGS_DEBUG "/DEBUG:FULL"
)

message(STATUS "Build ${CMAKE_BUILD_TYPE}")
message(STATUS "Bin Dir ${BIN_DIR}")

add_custom_target(exec
    COMMAND cd .. && ${CMAKE_CURRENT_BINARY_DIR}/${BIN_DIR}/${EXE_NAME}
)
