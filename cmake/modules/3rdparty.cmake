set(CATCH_DIR ${CMAKE_CURRENT_BINARY_DIR}/3rdparty/Catch/include)
file(MAKE_DIRECTORY "${CATCH_DIR}/catch")
file(DOWNLOAD
  "https://github.com/philsquared/Catch/releases/download/v1.9.1/catch.hpp"
  "${CATCH_DIR}/catch/catch.hpp"
  EXPECTED_HASH SHA512=129f4ed4cf7c32ee8f6ead1d4d1e8d4243929a62a857e921637dd39043066350f1674af0502a4d27cc3b29d64951558fe73e18d48d96a34e7a4dff9506ba0b48
)

add_library(Catch::Catch INTERFACE IMPORTED)
set_target_properties(Catch::Catch PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CATCH_DIR}"
)
