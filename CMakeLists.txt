cmake_minimum_required(VERSION 3.2)
project(asco)

include (cmake/utils.cmake)

set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++2a -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(./)
include_directories(third_party/json)

set(LIB_SRC
    asco/log.cc
    asco/util.cc
    asco/file.cc
    asco/thread.cc
    asco/coroutine.cc
    asco/scheduler.cc
    asco/async_io.cc
    asco/timer.cc
    asco/config.cc
)



set(LIBS
    asco
    uring
    pthread
)

option(BUILD_SHARED_LIBS "Build flexy as a shared lib" ON)
option(BUILD_STATIC_LIBS "Build flext as a static lib" OFF)


if (BUILD_STATIC_LIBS)
add_library(asco STATIC ${LIB_SRC})
elseif (BUILD_SHARED_LIBS)
add_library(asco SHARED ${LIB_SRC})
endif(BUILD_STATIC_LIBS)

redefine_file_macro(asco)

asco_add_executable(test_log "tests/test_log.cc" "${LIBS}")
asco_add_executable(test_coroutine "tests/test_coroutine.cc" "${LIBS}")
asco_add_executable(test_scheduler "tests/test_scheduler.cc" "${LIBS}")
asco_add_executable(test_async_io "tests/test_async_io.cc" "${LIBS}")
asco_add_executable(test_timer "tests/test_timer.cc" "${LIBS}")
asco_add_executable(test_config "tests/test_config.cc" "${LIBS}")

asco_add_executable(example1 "examples/example1.cc" "${LIBS}")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
