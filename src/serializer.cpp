#include "serializer.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>

namespace rapscallion {

void serializer<std::uint_least64_t>::write(Serializer& s, std::uint_least64_t value) {
  while (value >= 0x80) {
    s.addByte((uint8_t)(value & 0x7F) | 0x80);
    value >>= 7;
  }
  s.addByte((uint8_t)value);
}
void serializer<long>::write(Serializer& s, const long v) {
  std::uint_least64_t val = (std::abs(v) << 1) | (v < 0 ? 1 : 0);
  serializer<std::uint_least64_t>::write(s, val);
}
void serializer<int>::write(Serializer& s, const int v) {
  serializer<long>::write(s, v);
}
void serializer<std::string>::write(Serializer& s, const std::string& value) {
  serializer<std::uint_least64_t>::write(s, value.size());
  for (auto& c : value) s.addByte(c);
}
void serializer<bool>::write(Serializer& s, const bool b) {
  serializer<std::uint_least64_t>::write(s, b ? 1 : 0);
}

namespace {
  enum Type {
    NaN,
    Infinity,
    Zero,
    Normal
  };
  struct float_repr {
    // Can fit an exponent of 32-2=20 bits, can support octuple-precision IEEE754 floats
    std::int_least32_t exponent;
    // Can support double precision IEEE754 floats (quadruple requires 112 bits, octuple 236 bits)
    std::uint_least64_t fraction;
    static constexpr unsigned fraction_bits = 64;
    bool is_negative;
    Type type;
  };
  auto bitreverse(std::uint_least64_t b) -> decltype(b)
  {
    b = ((b & 0xFFFFFFFF00000000ULL) >> 32) | ((b & 0x00000000FFFFFFFFULL) << 32);
    b = ((b & 0xFFFF0000FFFF0000ULL) >> 16) | ((b & 0x0000FFFF0000FFFFULL) << 16);
    b = ((b & 0xFF00FF00FF00FF00ULL) >>  8) | ((b & 0x00FF00FF00FF00FFULL) <<  8);
    b = ((b & 0xF0F0F0F0F0F0F0F0ULL) >>  4) | ((b & 0x0F0F0F0F0F0F0F0FULL) <<  4);
    b = ((b & 0xCCCCCCCCCCCCCCCCULL) >>  2) | ((b & 0x3333333333333333ULL) <<  2);
    b = ((b & 0xAAAAAAAAAAAAAAAAULL) >>  1) | ((b & 0x5555555555555555ULL) <<  1);
    return b;
  }
}

template <>
struct serializer<float_repr> {
  static void write(Serializer& s, float_repr const &b) {
    // Using multiplication to avoid bit shifting a signed integer
    switch(b.type) {
      default:
        {
          static_assert(Type::Zero     != 0, "Cannot encode the sign of zero in the sign bit of zero on 2's complement systems");
          static_assert(Type::Infinity != 0, "Cannot encode the sign of infinity in the sign bit of zero on 2's complement systems");
          serializer<decltype(+b.exponent)>::write(s, b.is_negative ? -b.type : b.type);
        }
        break;
      case Normal:
        {
          const decltype(b.exponent) exponent
            = b.exponent * 2 + (b.exponent < 0 ? -b.is_negative : b.is_negative);
          const decltype(b.fraction) reversed_fraction
            = bitreverse(b.fraction);
          serializer<decltype(+exponent)>::write(s, exponent < 0 ? exponent - 2 : exponent + 3);
          serializer<decltype(+b.fraction)>::write(s, reversed_fraction);
        }
        break;
    };
  }
  static float_repr read(Deserializer& s) {
    float_repr result;
    auto exponent = serializer<decltype(result.exponent)>::read(s);
    if (exponent < 3 && exponent >= -2) {
      result.is_negative   = (exponent < 0);
      result.type          = (Type)std::abs(exponent);
    } else {
      exponent = (exponent < 0) ? exponent + 2 : exponent - 3;
      result.is_negative   = !!((exponent/ 1) % 2);
      result.type = Normal;
      result.exponent      = exponent / 2;
      const auto reversed_fraction = serializer<decltype(result.fraction)>::read(s);
      result.fraction      = bitreverse(reversed_fraction);
    }
    return result;
  }
};

void serializer<double>::write(Serializer& s, double const b) {
  switch (std::fpclassify(b)) {
    case FP_ZERO:
      serializer<float_repr>::write(s, { 0, 0, !!std::signbit(b), Zero });
      break;
    case FP_INFINITE:
      serializer<float_repr>::write(s, { 0, 0, !!std::signbit(b), Infinity });
      break;
    case FP_NAN:
      // The bit reversal is to ensure the most efficient encoding can be used
      serializer<float_repr>::write(s, { 0, 0, false, NaN });
      break;
    case FP_NORMAL:
    case FP_SUBNORMAL:
      float_repr repr;
      repr.type = Normal;
      repr.is_negative = !!std::signbit(b);
      // Make the fraction a positive integer
      repr.fraction = std::ldexp(std::abs(std::frexp(b, &repr.exponent)), repr.fraction_bits);
      serializer<decltype(repr)>::write(s, repr);
      break;
  }
}

double serializer<double>::read(Deserializer& s) {
  const auto repr = serializer<float_repr>::read(s);
  switch(repr.type) {
    case Zero:
      return (repr.is_negative ? -0.0 : 0.0);
    case Infinity:
      return repr.is_negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    case NaN:
      return std::numeric_limits<double>::quiet_NaN();
    default:
      return (repr.is_negative ? -1.0 : 1.0) * std::ldexp(static_cast<double>(repr.fraction), repr.exponent - repr.fraction_bits);
  }
}

void serializer<float>::write(Serializer& s, float const b) {
  serializer<double>::write(s, b);
}
float serializer<float>::read(Deserializer& s) {
  return serializer<double>::read(s);
}

std::uint_least64_t serializer<std::uint_least64_t>::read(Deserializer& s) {
  std::uint_least64_t val = 0,
    offs = 0,
    b = s.getByte();
  while (b & 0x80) {
    val |= (b & 0x7F) << offs;
    offs += 7;
    b = s.getByte();
  }
  val |= b << offs;
  return val;
}
long serializer<long>::read(Deserializer& s) {
  const auto val = serializer<std::uint_least64_t>::read(s);
  auto value = static_cast<long>(val >> 1);
  if (val & 1) value = -value;
  return value;
}
int serializer<int>::read(Deserializer& s) {
  return serializer<long>::read(s);
}
std::string serializer<std::string>::read(Deserializer& s) {
  const auto length = serializer<std::uint_least64_t>::read(s);
  const uint8_t* ptr = s.ptr;
  const uint8_t* end = ptr + length;
  if (end > s.end) 
    throw parse_exception("buffer underrun");
  s.ptr = end;
  return std::string(reinterpret_cast<const char*>(ptr), length);
}
bool serializer<bool>::read(Deserializer& s) {
  return serializer<std::uint_least64_t>::read(s) > 0;
}

}
