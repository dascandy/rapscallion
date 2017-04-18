#include <catch/catch.hpp>
#include <cmath>
#include <limits>
#include <RaPsCallion/serializer.h>

namespace rapscallion {
namespace test {

void test(const std::string& inStr, const double in, const std::ptrdiff_t expected_size = -1) {
  GIVEN("the real number " + inStr) {
    WHEN("we serialize it") {
      Serializer s;
      serializer<decltype(+in)>::write(s, in);
      Deserializer d(s);

      THEN("the output meets size expectations") {
        const auto size = d.end - d.ptr;
        if (expected_size >= 0)
          CHECK(size == expected_size);

        // largest encoding of IEEE754 binary64 float:
        //  (11 exponent bits + exponent sign + NaN/inf flag) = 13 bits / 7 bit/byte = 2 byte
        //  (52 bit fraction + sign) = 53 bits / 7 bit/byte = 8 byte
        // + = 10 byte
        CHECK(size <= 10);

        AND_WHEN("we deserialize it again") {
          const auto out = serializer<decltype(+in)>::read(d);
          THEN("the deserialized output is equal to the original input") {
            if (std::isnan(in)) {
              CHECK(std::isnan(out));
            } else {
              CHECK(out == in);
              CHECK(std::signbit(out) == std::signbit(in));
            }
          }
        }
      }
    }
  }
}

}
}

SCENARIO("Serializing floating point numbers", "[serialization]") {
  using namespace rapscallion::test;

#define STRIFY1ST(num, ...) #num
#define TEST(...) \
  test(STRIFY1ST(__VA_ARGS__, void), __VA_ARGS__)

  // Smallest encodings, only requiring flags
  TEST(std::numeric_limits<double>::infinity(), 1);
  TEST(-std::numeric_limits<double>::infinity(), 1);
  TEST(std::numeric_limits<double>::quiet_NaN(), 1);
  TEST(0.0, 1);
  TEST(-0.0, 1);

  TEST(0.03125, 2);
  TEST(-0.03125, 2);
  TEST(0.0625, 2);
  TEST(-0.0625, 2);
  TEST(0.125, 2);
  TEST(-0.125, 2);
  TEST(0.25, 2);
  TEST(-0.25, 2);
  TEST(0.5, 2);
  TEST(-0.5, 2);
  TEST(1.0, 2);
  TEST(-1.0, 2);
  TEST(2.0, 2);
  TEST(-2.0, 2);
  TEST(M_PI, 9);
  TEST(-M_PI, 9);
  TEST((float)M_PI, 5);
  TEST(-(float)M_PI, 5);
  TEST(std::numeric_limits<double>::epsilon());
  TEST(std::numeric_limits<double>::min());
  TEST(std::numeric_limits<double>::max(), 10);
  TEST(std::numeric_limits<double>::denorm_min());

#undef STRIFY
#undef TEST
}
