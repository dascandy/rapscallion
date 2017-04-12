#include "serializer.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>

void writer<size_t>::write(Serializer& s, const size_t &val) {
  size_t value = val;
  while (value >= 0x80) {
    s.addByte((uint8_t)(value & 0x7F) | 0x80);
    value >>= 7;
  }
  s.addByte((uint8_t)value);
}
void writer<long>::write(Serializer& s, const long &v) {
  size_t val = (std::abs(v) << 1) | (v < 0 ? 1 : 0);
  writer<size_t>::write(s, val);
}
void writer<int>::write(Serializer& s, const int &v) {
  writer<long>::write(s, v);
}
void writer<std::string>::write(Serializer& s, const std::string& value) {
  writer<size_t>::write(s, value.size());
  for (auto& c : value) s.addByte(c);
}
void writer<bool>::write(Serializer& s, const bool &b) {
  writer<size_t>::write(s, b ? 1 : 0);
}

namespace {
  struct float_repr {
    // Can fit an exponent of 32-2=20 bits, can support octuple-precision IEEE754 floats
    std::int_least32_t exponent;
    // Can support double precision IEEE754 floats (quadruple requires 112 bits, octuple 236 bits)
    std::uint_least64_t fraction;
    // One bit reserved for sign bit
    static constexpr unsigned fraction_bits = 63 - 1;
    bool is_negative;
    //! Is either infinity or NaN
    bool is_non_number;
  };
  auto bitreverse(std::uint_least64_t b) -> decltype(b)
  {
    b = ((b & 0b1111111111111111111111111111111100000000000000000000000000000000ULL) >> 32) | ((b & 0b0000000000000000000000000000000011111111111111111111111111111111ULL) << 32);
    b = ((b & 0b1111111111111111000000000000000011111111111111110000000000000000ULL) >> 16) | ((b & 0b0000000000000000111111111111111100000000000000001111111111111111ULL) << 16);
    b = ((b & 0b1111111100000000111111110000000011111111000000001111111100000000ULL) >>  8) | ((b & 0b0000000011111111000000001111111100000000111111110000000011111111ULL) <<  8);
    b = ((b & 0b1111000011110000111100001111000011110000111100001111000011110000ULL) >>  4) | ((b & 0b0000111100001111000011110000111100001111000011110000111100001111ULL) <<  4);
    b = ((b & 0b1100110011001100110011001100110011001100110011001100110011001100ULL) >>  2) | ((b & 0b0011001100110011001100110011001100110011001100110011001100110011ULL) <<  2);
    b = ((b & 0b1010101010101010101010101010101010101010101010101010101010101010ULL) >>  1) | ((b & 0b0101010101010101010101010101010101010101010101010101010101010101ULL) <<  1);
    return b;
  }
}

template <>
struct writer<float_repr> {
  static void write(Serializer& s, float_repr const &b) {
    // Using multiplication to avoid bit shifting a signed integer
    const decltype(b.exponent) exponent_and_special
      = b.exponent * 2 + b.is_non_number * 1;
    const decltype(b.fraction) reversed_fraction_and_sign
      = bitreverse(b.fraction) * 2 + b.is_negative * 1;
    writer<decltype(+exponent_and_special)>::write(s, exponent_and_special);
    writer<decltype(b.fraction)>::write(s, reversed_fraction_and_sign);
  }
};
template <>
struct reader<float_repr> {
  static float_repr read(Deserializer& s) {
    float_repr result;
    const auto exponent_and_special = reader<decltype(result.exponent)>::read(s);
    result.is_non_number = !!(exponent_and_special / 1 % 2);
    result.exponent      =    exponent_and_special / 2;
    const auto reversed_fraction_and_sign = reader<decltype(result.fraction)>::read(s);
    result.is_negative   = !!(reversed_fraction_and_sign / 1 % 2);
    result.fraction      = bitreverse(reversed_fraction_and_sign / 2);
    return result;
  }
};

void writer<double>::write(Serializer& s, double const &b) {
  switch (std::fpclassify(b)) {
    case FP_ZERO:
      writer<float_repr>::write(s, { 0, 0, !!std::signbit(b), false });
      break;
    case FP_INFINITE:
      writer<float_repr>::write(s, { 0, 0, !!std::signbit(b), true });
      break;
    case FP_NAN:
      // The bit reversal is to ensure the most efficient encoding can be used
      writer<float_repr>::write(s, { 0, bitreverse(static_cast<decltype(float_repr::fraction)>(1)), false, true });
      break;
    case FP_NORMAL:
    case FP_SUBNORMAL:
      float_repr repr;
      repr.is_non_number = false;
      repr.is_negative = !!std::signbit(b);
      // Make the fraction a positive integer
      repr.fraction = std::ldexp(std::abs(std::frexp(b, &repr.exponent)), repr.fraction_bits);
      writer<decltype(repr)>::write(s, repr);
      break;
  }
}
double reader<double>::read(Deserializer& s) {
  const auto repr = reader<float_repr>::read(s);
  if (repr.is_non_number) {
    if (repr.fraction == 0) {
      return repr.is_negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    } else {
      return std::numeric_limits<double>::quiet_NaN();
    }
  }

  if (repr.exponent == 0 && repr.fraction == 0) {
    return repr.is_negative ? -0.0 : 0.0;
  }

  return (repr.is_negative ? -1.0 : 1.0) * std::ldexp(static_cast<double>(repr.fraction), repr.exponent - repr.fraction_bits);
}

void writer<float>::write(Serializer& s, float const &b) {
  writer<double>::write(s, b);
}
float reader<float>::read(Deserializer& s) {
  return reader<double>::read(s);
}

size_t reader<size_t>::read(Deserializer& s) {
  size_t val = 0;
  size_t offs = 0;
  size_t b = s.getByte();
  while (b & 0x80) {
    val |= (b & 0x7F) << offs;
    offs += 7;
    b = s.getByte();
  }
  val |= b << offs;
  return val;
}
long reader<long>::read(Deserializer& s) {
  size_t val = reader<size_t>::read(s);
  long value = (long)(val >> 1);
  if (val & 1) value = -value;
  return value;
}
int reader<int>::read(Deserializer& s) {
  return reader<long>::read(s);
}
std::string reader<std::string>::read(Deserializer& s) {
  size_t length = reader<size_t>::read(s);
  const uint8_t* ptr = s.ptr;
  const uint8_t* end = ptr + length;
  if (end > s.end) 
    throw parse_exception("buffer underrun");
  s.ptr = end;
  return std::string(reinterpret_cast<const char*>(ptr), length);
}
bool reader<bool>::read(Deserializer& s) {
  return reader<size_t>::read(s) > 0;
}


