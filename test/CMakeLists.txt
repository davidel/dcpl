cmake_minimum_required(VERSION 3.10)

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)
FetchContent_Declare(
  GoogleTest
  URL https://github.com/google/googletest/archive/54501746a6cfd366b6e7bab3c902e0f8d5826521.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(GoogleTest)

enable_testing()

project(dcpl_test)

add_executable(dcpl_test
  "test.cc"
)

set_target_properties(dcpl_test PROPERTIES
  LINKER_LANGUAGE CXX
)

target_link_libraries(dcpl_test PUBLIC
  dcpl
  gtest_main
  pthread
)

include(GoogleTest)
gtest_discover_tests(dcpl_test)

