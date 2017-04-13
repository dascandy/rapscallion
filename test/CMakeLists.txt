project(RaPsCallion.Test CXX)

foreach(TEST
    call
    serializer
)
  add_executable(${PROJECT_NAME}.${TEST}
    ${TEST}.cpp
  )
  set_target_properties(${PROJECT_NAME}.${TEST} PROPERTIES OUTPUT_NAME ${TEST})
  target_link_libraries(${PROJECT_NAME}.${TEST}
    PRIVATE
      RaPsCallion
  )
  add_test(NAME ${TEST} COMMAND ${PROJECT_NAME}.${TEST})
endforeach()
