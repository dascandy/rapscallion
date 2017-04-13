cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(RaPsCallion CXX)
enable_testing()

if(CMAKE_VERSION VERSION_LESS "3.4")
  # Older CMake versions need this while detecting Threads (for header detection)
  enable_language(C)
endif()

find_package(Boost 1.38 REQUIRED)
find_package(Threads REQUIRED)

# The Boost::<C> IMPORT targets aren't added until CMake 3.5
if(CMAKE_VERSION VERSION_LESS "3.5" AND NOT TARGET Boost::boost)
  add_library(Boost::boost INTERFACE IMPORTED)
  set_target_properties(Boost::boost PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
endif()

add_library(RaPsCallion SHARED
  include/RaPsCallion/connection.h
  include/RaPsCallion/future.h
  include/RaPsCallion/rpc.h
  include/RaPsCallion/serializer.h
  include/RaPsCallion/serverconnection.h
  src/connection.cpp
  src/serializer.cpp
  src/serverconnection.cpp
)
set_target_properties(RaPsCallion PROPERTIES OUTPUT_NAME rapscallion)
target_include_directories(RaPsCallion
  PUBLIC
    include
  PRIVATE
    include/RaPsCallion
)
target_link_libraries(RaPsCallion
  PRIVATE
    Boost::boost
    Threads::Threads
)

add_subdirectory(test)
