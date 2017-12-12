add_executable(app
  main.cpp
)

target_link_librarieS(app
  PRIVATE
    RaPsCallion
    boost_system
    boost_thread
    -pthread
)

