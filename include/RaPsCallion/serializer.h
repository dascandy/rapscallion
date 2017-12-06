#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "serializer_fwd.hpp"

namespace rapscallion {

namespace Serializer_ADL {
struct Serializer {
  std::vector<uint8_t> buffer;
  Serializer() {
    buffer.resize(8);
  }
  void addByte(uint8_t b) { buffer.push_back(b); }
  std::pair<uint8_t *, size_t> data() {
    size_t len = buffer.size() - 8;
    size_t offs = 7;
    int mask = 0;
    while (len > 0x7F) {
      buffer[offs--] = mask | (len & 0x7F);
      len >>= 7;
      mask = 0x80;
    }
    buffer[offs] = len | mask;
    return std::make_pair(buffer.data() + offs, buffer.size() - offs);
  }
};
}

namespace Deserializer_ADL {
struct Deserializer {
  Deserializer(const std::vector<uint8_t> &buffer, size_t offset)
  : ptr(buffer.data() + offset)
  {
    uint64_t size = PacketSize(buffer, offset);
    while (*ptr & 0x80) ptr++;
    ptr++;
    end = ptr + size;
    if (end > buffer.data() + buffer.size())
      throw std::runtime_error("Packet exceeds buffer size");
  }

  Deserializer(const Serializer& s)
    : ptr(s.buffer.data() + 8)
    , end(s.buffer.data() + s.buffer.size())
  {
    s.buffer.at(7);
  }

  size_t getByte() {
    if (ptr == end) throw std::runtime_error("Exceeded packet size");
    return *ptr++;
  }
  static int64_t PacketSize(const std::vector<uint8_t>& vec, size_t offs) {
    int64_t len = 0;
    while (offs < vec.size()) {
      len = (len << 7) | (vec[offs] & 0x7F);
      if ((vec[offs] & 0x80) == 0)
        return len;
      offs++;
    }
    return -1;
  }
  const uint8_t *ptr, *end;
};
}

struct parse_exception : public std::exception {
  parse_exception(const char *err) : err(err)
  {}
  const char *err;
  const char *what() const throw() { return err; }
};

#define DECLARE_READER_WRITER(type) \
template <> \
struct serializer<type> { \
  static type read(Deserializer& s); \
  static void write(Serializer& s, type b); \
};

template <typename T>
struct serializer;
DECLARE_READER_WRITER(std::uint_least64_t)
DECLARE_READER_WRITER(int)
DECLARE_READER_WRITER(long)
DECLARE_READER_WRITER(bool)
DECLARE_READER_WRITER(double)
DECLARE_READER_WRITER(float)
template <>
struct serializer<std::string> {
  static std::string read(Deserializer& s);
  static void write(Serializer& s, const std::string& b);
};

template <typename T>
class optional;
template <typename T>
struct serializer<optional<T> > {
  static void write(Serializer &s, const optional<T> &opt) {
    serializer<bool>::write(s, (opt.value != NULL));
    if (opt.value) {
      serializer<T>::write(s, *opt.value);
    }
  }
  static optional<T> read(Deserializer& s) {
    optional<T> val;
    bool isNotNull = serializer<bool>::read(s);
    if (isNotNull) {
      val.set(serializer<T>::read(s));
    }
    return val;
  }
};

template <typename T>
struct serializer<std::vector<T> > {
  static void write(Serializer& s, const std::vector<T>& value) {
    serializer<std::uint_least64_t>::write(s, value.size());
    for (const T &v : value) {
      serializer<T>::write(s, v);
    }
  }
  static std::vector<T> read(Deserializer& s) {
    std::vector<T> t;
    const auto size = serializer<std::uint_least64_t>::read(s);
    t.reserve(size);
    for (decltype(+size) n = 0; n < size; ++n) {
      t.push_back(serializer<T>::read(s));
    }
    return t;
  }
};

template <typename T>
struct serializer<std::shared_ptr<T>> {
  static void write(Serializer& s, const std::shared_ptr<T> &p) {
    serializer<bool>::write(s, (p.get() != NULL));
    if (p.get()) {
      serializer<T>::write(s, *p.get());
    }
  }
  static std::shared_ptr<T> read(Deserializer& s) {
    bool isNotNull = serializer<bool>::read(s);
    if (isNotNull) {
      return std::make_shared<T>(serializer<T>::read(s));
    } else {
      return std::shared_ptr<T>();
    }
  }
};

}

#endif
