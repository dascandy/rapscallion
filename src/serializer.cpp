#include "serializer.h"
#include <cstdlib>

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


