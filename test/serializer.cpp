#include <catch/catch.hpp>
#include <cmath>
#include <limits>
#include <Serializer.h>

namespace Rapscallion {
namespace test {

struct byte_view
{
  constexpr byte_view() = default;

  byte_view(const Deserializer& d)
    : first(reinterpret_cast<const char*>(d.buffer.data()))
    , last (reinterpret_cast<const char*>(d.buffer.data() + d.buffer.size()))
  {}

  template <size_t N>
  constexpr byte_view(const char (&arr)[N])
    : first(arr)
    , last(&arr[N - 1])
  {}

  constexpr const char* begin() const { return first;         }
  constexpr const char* end()   const { return last;          }
  constexpr size_t      size()  const { return last - first;  }
  constexpr bool        empty() const { return first == last; }

private:
  const char* first = nullptr;
  const char* last  = nullptr;
};

bool operator==(const byte_view& lhs, const byte_view& rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

std::ostream& operator<<(std::ostream& os, const byte_view& v) {
  const auto oldFlags = os.setf(std::ios_base::hex, std::ios_base::basefield);
  const auto oldFill = os.fill('0');
  os << '"';
  for (const auto& c : v) {
    os << "\\x" << std::setw(2) << static_cast<unsigned>(static_cast<unsigned char>(c));
  }
  os << '"';
  os.setf(oldFlags);
  os.fill(oldFlags);
  return os;
}

void test(const std::string& inStr, const double in, const std::ptrdiff_t expected_size = -1, const byte_view& expected_serialization = byte_view()) {
  GIVEN("the real number " + inStr) {
    WHEN("we serialize it") {
      Serializer s;
      serializer<decltype(+in)>::write(s, in);
      Deserializer d(s);

      THEN("the serialized output meets expectations") {
        const byte_view serialized(d);
        if (expected_size >= 0)
          CHECK(serialized.size() == expected_size);

        // largest encoding of IEEE754 binary64 float:
        //  (11 exponent bits + exponent sign + NaN/inf flag) = 13 bits / 7 bit/byte = 2 byte
        //  (52 bit fraction + sign) = 53 bits / 7 bit/byte = 8 byte
        // + = 10 byte
        // plus one byte for the prefixed size
        CHECK(serialized.size() <= 11);

        if (!expected_serialization.empty())
          CHECK(serialized == expected_serialization);

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

void test(const std::string& inStr, const double in, const byte_view& expected_serialization) {
  return test(inStr, in, expected_serialization.size(), expected_serialization);
}

}
}

SCENARIO("Serializing floating point numbers", "[serialization]") {
  using namespace Rapscallion::test;

#define STRIFY1ST(num, ...) #num
#define TEST(...) \
  test(STRIFY1ST(__VA_ARGS__, void), __VA_ARGS__)

  // Smallest encodings, only requiring flags
  TEST( std::numeric_limits<double>::infinity(), "\x01\x02");
  TEST(-std::numeric_limits<double>::infinity(), "\x01\x03");
  TEST(std::numeric_limits<double>::quiet_NaN(), "\x01\x00");
  TEST( 0.0, "\x01\x04");
  TEST(-0.0, "\x01\x05");

  TEST( 0.03125, "\x02\x15\x01");
  TEST(-0.03125, "\x02\x17\x01");
  TEST( 0.0625 , "\x02\x11\x01");
  TEST(-0.0625 , "\x02\x13\x01");
  TEST( 0.125  , "\x02\x0d\x01");
  TEST(-0.125  , "\x02\x0f\x01");
  TEST( 0.25   , "\x02\x09\x01");
  TEST(-0.25   , "\x02\x0b\x01");
  TEST( 0.5    , "\x02\x06\x01");
  TEST(-0.5    , "\x02\x08\x01");
  TEST( 1.0    , "\x02\x0a\x01");
  TEST(-1.0    , "\x02\x0c\x01");
  TEST( 2.0    , "\x02\x0e\x01");
  TEST(-2.0    , "\x02\x10\x01");

  TEST(M_PI, 10);
  TEST(-M_PI, 10);
  TEST((float)M_PI, 6);
  TEST(-(float)M_PI, 6);
  TEST(std::numeric_limits<double>::epsilon(), 4);
  TEST(std::numeric_limits<double>::min(), 4);
  TEST(std::numeric_limits<double>::max(), 11);
  TEST(std::numeric_limits<double>::denorm_min(), 4);

#undef STRIFY
#undef TEST
}
