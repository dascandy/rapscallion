cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(RapSCallion)

find_package(Boost 1.38 REQUIRED)

add_library(RapSCallion SHARED
  include/connection.h
  include/future.h
  include/rpc.h
  include/serializer.h
  include/serverconnection.h
  src/connection.cpp
  src/serializer.cpp
  src/serverconnection.cpp
)
set_target_properties(RapSCallion PROPERTIES OUTPUT_NAME rapscallion)
target_include_directories(RapSCallion
  PUBLIC
    include
)
target_link_libraries(RapSCallion
  PUBLIC
    Boost::boost
)
