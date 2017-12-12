#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstring>

namespace Rapscallion {

class Serializer {
private:
  mutable std::vector<uint8_t> buffer;
  mutable size_t offs = 9;
public:
  Serializer() {
    buffer.resize(8);
  }
  void addByte(uint8_t b) { buffer.push_back(b); }
  uint8_t *data() const { finalize(); return buffer.data() + offs; }
  size_t size() const { finalize(); return buffer.size() - offs; }
private:
  void finalize() const {
    if (offs != 9) return;
    size_t len = buffer.size() - 8;
    offs = 7;
    int mask = 0;
    while (len > 0x7F) {
      buffer[offs--] = mask | (len & 0x7F);
      len >>= 7;
      mask = 0x80;
    }
    buffer[offs] = len | mask;
  }
};

class Deserializer {
public:
  Deserializer() {}
  Deserializer(const Serializer& s)
    : buffer(s.data(), s.data() + s.size())
  {
    HasFullPacket();
  }
  size_t getByte() {
    if (offs == size) throw std::runtime_error("Exceeded packet size");
    return buffer[offs++];
  }
  uint8_t *getByteRange(size_t byteCount) {
    if (offs + byteCount > size) throw std::runtime_error("Exceeded packet size");
    size_t oldOffs = offs;
    offs += byteCount;
    return buffer.data() + oldOffs;
  }
  void AddBytes(const uint8_t *bytes, size_t addedSize) {
    buffer.insert(buffer.end(), bytes, bytes + addedSize);
  }
  bool HasFullPacket() {
    offs = 0; size = 0;
    while (offs < buffer.size()) {
      size = (size << 7) | (buffer[offs] & 0x7F);
      if ((buffer[offs] & 0x80) == 0) {
        offs++;
        size += offs;
        return (buffer.size() + offs >= size);
      }
      offs++;
    }
    return false;
  }
  void RemovePacket() {
    memmove(buffer.data(), buffer.data() + size, buffer.size() - size);
    buffer.resize(buffer.size() - size);
  }
  size_t size = 0;
  size_t offs = 0;
  std::vector<uint8_t> buffer;
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

