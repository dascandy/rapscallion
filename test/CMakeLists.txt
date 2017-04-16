project(RaPsCallion.Test CXX)

add_executable(${PROJECT_NAME}
  catch-main.cpp
  serializer.cpp
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Catch::Catch
    RaPsCallion
)
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
