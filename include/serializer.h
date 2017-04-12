#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct Deserializer {
  Deserializer(const std::vector<uint8_t> &buffer, size_t offset)
  : ptr(buffer.data() + offset)
  {
    uint64_t size = PacketSize(buffer, offset);
    while (*ptr & 0x80) ptr++;
    ptr++;
    end = ptr + size;
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

struct Serializer {
  Serializer() {
    buffer.resize(8);
  }
  std::vector<uint8_t> buffer;
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

struct parse_exception : public std::exception {
  parse_exception(const char *err) : err(err)
  {}
  const char *err;
  const char *what() const throw() { return err; }
};

#define DECLARE_READER_WRITER(type) \
template <> \
struct reader<type> { \
  static type read(Deserializer& s); \
}; \
template <> \
struct writer<type> { \
  static void write(Serializer& s, type const &b); \
};

template <typename T>
struct writer;
template <typename T>
struct reader;
DECLARE_READER_WRITER(size_t)
DECLARE_READER_WRITER(int)
DECLARE_READER_WRITER(long)
DECLARE_READER_WRITER(std::string)
DECLARE_READER_WRITER(bool)

template <typename T>
class optional;
template <typename T>
struct writer<optional<T> > {
  static void write(Serializer &s, const optional<T> &opt) {
    writer<bool>::write(s, (opt.value != NULL));
    if (opt.value) {
      writer<T>::write(s, *opt.value);
    }
  }
};
template <typename T>
struct reader<optional<T> > {
  static optional<T> read(Deserializer& s) {
    optional<T> val;
    bool isNotNull = reader<bool>::read(s);
    if (isNotNull) {
      val.set(reader<T>::read(s));
    }
    return val;
  }
};

template <typename T>
struct writer<std::vector<T> > {
  static void write(Serializer& s, const std::vector<T>& value) {
    writer<size_t>::write(s, value.size());
    for (const T &v : value) {
      writer<T>::write(s, v);
    }
  }
};
template <typename T>
struct reader<std::vector<T> > {
  static std::vector<T> read(Deserializer& s) {
    std::vector<T> t;
    size_t size = reader<size_t>::read(s);
    t.reserve(size);
    for (size_t n = 0; n < size; ++n) {
      t.push_back(reader<T>::read(s));
    }
    return t;
  }
};

template <typename T>
struct writer<std::shared_ptr<T>> {
  static void write(Serializer& s, const std::shared_ptr<T> &p) {
    writer<bool>::write(s, (p.get() != NULL));
    if (p.get()) {
      writer<T>::write(s, *p.get());
    }
  }
};
template <typename T>
struct reader<std::shared_ptr<T>> {
  static std::shared_ptr<T> read(Deserializer& s) {
    bool isNotNull = reader<bool>::read(s);
    if (isNotNull) {
      return std::make_shared<T>(reader<T>::read(s));
    } else {
      return std::shared_ptr<T>();
    }
  }
};

#endif


