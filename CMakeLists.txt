cmake_minimum_required (VERSION 2.8.11)
project (Oven)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_definitions(-DNOMINMAX -D_UNICODE)

include_directories("${CMAKE_SOURCE_DIR}/src")

add_library (base STATIC
  src/base/base64.h
  src/base/base64.cpp 
  src/base/command_line.h
  src/base/command_line.cpp 
)

add_library (system STATIC
  src/system/child_process.h
  src/system/child_process.cpp 
  src/system/desktop.h
  src/system/desktop.cpp 
  src/system/error.h
  src/system/error.cpp 
  src/system/iocp.h
  src/system/iocp.cpp 
  src/system/job.h
  src/system/job.cpp 
  src/system/pipe.h
  src/system/pipe.cpp
  src/system/scoped_handle.h
  src/system/scoped_handle.cpp 
)

add_executable (oven
  src/execution_result.h
  src/execution_result.cpp 
  src/oven.cpp
)

target_link_libraries (oven base system)
