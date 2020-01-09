cmake_minimum_required(VERSION 3.5)
project(basic_llvm_test)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -mavx2 -march=native")

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE STRING "Choose the type of build" FORCE)
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LINK_LIBS
  support core irreader analysis executionengine instcombine object orcJIT
  runtimedyld scalaropts transformutils native ipo orcjit)

add_executable(basic_llvm_test basic_llvm_test.cc)
target_link_libraries(basic_llvm_test ${LLVM_LINK_LIBS})
