cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(RaPsCallion CXX)
enable_testing()

include(3rdparty)

if(CMAKE_VERSION VERSION_LESS "3.4")
  # Older CMake versions need this while detecting Threads (for header detection)
  enable_language(C)
endif()

find_package(Boost 1.38 REQUIRED COMPONENTS thread)
if (NOT WIN32)
  add_definitions(-Wall -Wextra -Wpedantic -Werror -Wshadow)
endif()

# The Boost::<C> IMPORT targets aren't added until CMake 3.5
if(NOT TARGET Boost::boost)
  add_library(Boost::boost INTERFACE IMPORTED)
  set_target_properties(Boost::boost PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
endif()

add_library(RaPsCallion SHARED
  include/RaPsCallion/Connection.h
  include/RaPsCallion/Dispatcher.h
  include/RaPsCallion/future.h
  include/RaPsCallion/InterfaceDispatcher.h
  include/RaPsCallion/InterfaceProxy.h
  include/RaPsCallion/Proxy.h
  include/RaPsCallion/RpcClient.h
  include/RaPsCallion/RpcHandle.h
  include/RaPsCallion/RpcHost.h
  include/RaPsCallion/Serializer.h
  include/RaPsCallion/Server.h
  src/RpcHandle.cpp
  src/Serializer.cpp
)
set_target_properties(RaPsCallion PROPERTIES OUTPUT_NAME rapscallion)
target_include_directories(RaPsCallion
  PUBLIC
    include/RaPsCallion
)
target_link_libraries(RaPsCallion
  PRIVATE
    Boost::boost
    Boost::thread
)
target_compile_definitions(RaPsCallion
  PUBLIC
    BOOST_THREAD_PROVIDES_FUTURE
    BOOST_THREAD_PROVIDES_EXECUTORS
    BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
    BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
)

add_subdirectory(test)
add_subdirectory(app)
