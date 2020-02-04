set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/folly/CMake"
  "${CMAKE_CURRENT_SOURCE_DIR}/folly/build/fbcode_builder/CMake"
  ${CMAKE_MODULE_PATH})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  include(FollyCompilerMSVC)
else()
  include(FollyCompilerUnix)
endif()

# CMP0075 Include file check macros honor CMAKE_REQUIRED_LIBRARIES
if(POLICY CMP0075)
  cmake_policy(SET CMP0075 NEW)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
set(FOLLY_HAVE_PTHREAD "${CMAKE_USE_PTHREADS_INIT}")
list(APPEND CMAKE_REQUIRED_LIBRARIES Threads::Threads)
list(APPEND FOLLY_LINK_LIBRARIES Threads::Threads)

find_package(Boost 1.51.0 MODULE
  COMPONENTS
    context
    filesystem
    program_options
    regex
    system
    thread
  REQUIRED
)
list(APPEND FOLLY_LINK_LIBRARIES ${Boost_LIBRARIES})
list(APPEND FOLLY_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIRS})

find_package(DoubleConversion MODULE REQUIRED)
list(APPEND FOLLY_LINK_LIBRARIES ${DOUBLE_CONVERSION_LIBRARY})
list(APPEND FOLLY_INCLUDE_DIRECTORIES ${DOUBLE_CONVERSION_INCLUDE_DIR})

find_package(Gflags MODULE)
set(FOLLY_HAVE_LIBGFLAGS ${LIBGFLAGS_FOUND})
list(APPEND FOLLY_LINK_LIBRARIES ${LIBGFLAGS_LIBRARY})
list(APPEND FOLLY_INCLUDE_DIRECTORIES ${LIBGFLAGS_INCLUDE_DIR})
list(APPEND CMAKE_REQUIRED_LIBRARIES ${LIBGFLAGS_LIBRARY})
list(APPEND CMAKE_REQUIRED_INCLUDES ${LIBGFLAGS_INCLUDE_DIR})

find_package(Glog MODULE)
set(FOLLY_HAVE_LIBGLOG ${GLOG_FOUND})
list(APPEND FOLLY_LINK_LIBRARIES ${GLOG_LIBRARY})
list(APPEND FOLLY_INCLUDE_DIRECTORIES ${GLOG_INCLUDE_DIR})

include(FollyConfigChecks)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/folly/CMake/folly-config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/folly/folly-config.h
)

add_library(folly
  ${CMAKE_CURRENT_BINARY_DIR}/folly/folly-config.h
  folly/folly/Conv.cpp
  folly/folly/File.cpp
  folly/folly/FileUtil.cpp
  folly/folly/Format.cpp
  folly/folly/ScopeGuard.cpp
  folly/folly/experimental/TestUtil.cpp
  folly/folly/hash/SpookyHashV2.cpp
  folly/folly/io/IOBufQueue.cpp
  folly/folly/io/IOBuf.cpp
  folly/folly/io/RecordIO.cpp
  folly/folly/lang/Assume.cpp
  folly/folly/net/NetOps.cpp
  folly/folly/portability/SysUio.cpp
  folly/folly/system/MemoryMapping.cpp
)
target_compile_definitions(folly PUBLIC -DFOLLY_HAVE_WEAK_SYMBOLS=1)
target_compile_options(folly PRIVATE -frtti -fexceptions)
target_include_directories(
  folly
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/folly"
    "${CMAKE_CURRENT_BINARY_DIR}/"
  PRIVATE
    "${FOLLY_INCLUDE_DIRECTORIES}"
)
target_link_libraries(
  folly
  PUBLIC
    jemalloc
  PRIVATE
    "${FOLLY_LINK_LIBRARIES}")
