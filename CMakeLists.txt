cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)

# We need this since we are making DCPL a static library which might need
# to be linked by shared libraries.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_compile_options(
  "-O3"
  "-g"
)

include_directories(
  "/usr/local/include"
)

link_directories(
  "/usr/local/lib"
)

project(dcpl)

file(GLOB DCPL_HEADERS "include/dcpl/*.h" "include/dcpl/coro/*.h")
file(GLOB DCPL_SOURCES "src/*.cc" "src/coro/*.cc")

if (UNIX)
  file(GLOB DCPL_OS_HEADERS "include/dcpl/posix/*.h")
  file(GLOB DCPL_OS_SOURCES "src/posix/*.cc")
  list(APPEND DCPL_HEADERS ${DCPL_OS_HEADERS})
  list(APPEND DCPL_SOURCES ${DCPL_OS_SOURCES})
endif ()

if (WIN32)
  file(GLOB DCPL_OS_HEADERS "include/dcpl/windows/*.h")
  file(GLOB DCPL_OS_SOURCES "src/windows/*.cc")
  list(APPEND DCPL_HEADERS ${DCPL_OS_HEADERS})
  list(APPEND DCPL_SOURCES ${DCPL_OS_SOURCES})
endif ()

add_library(dcpl STATIC
  ${DCPL_HEADERS}
  ${DCPL_SOURCES}
)

set_target_properties(dcpl PROPERTIES
  LINKER_LANGUAGE CXX
)

set_target_properties(dcpl PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

target_include_directories(dcpl PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
)

install(TARGETS dcpl
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
  DESTINATION "include"
  FILES_MATCHING
  PATTERN "*.h"
)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  if (NOT DCPL_DISABLE_TESTING)
    add_subdirectory("test")
  endif()
endif()

