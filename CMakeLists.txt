cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(RaPsCallion CXX)

enable_language(C) # Older CMake versions need this while detecting Boost
find_package(Boost 1.38 REQUIRED)
find_package(Threads REQUIRED)

add_library(RaPsCallion SHARED
  include/connection.h
  include/future.h
  include/rpc.h
  include/serializer.h
  include/serverconnection.h
  src/connection.cpp
  src/serializer.cpp
  src/serverconnection.cpp
)
set_target_properties(RaPsCallion PROPERTIES OUTPUT_NAME rapscallion)
target_include_directories(RaPsCallion
  PUBLIC
    include
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries(RaPsCallion
  PRIVATE
    Threads::Threads
)
