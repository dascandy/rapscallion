#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <cstdint>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

struct Serializer {
  Serializer(std::vector<uint8_t> *buffer)
  : buffer(buffer)
  {}
  Serializer() {
    buffer = new std::vector<uint8_t>();
  }
  ~Serializer() {
    delete buffer;
  }
  void addBytes(const uint8_t *buffer, size_t bytesRead, std::function<void(Serializer&)> onReceive);
  std::vector<uint8_t> *buffer;
  size_t offset = 0;
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
  static type read(Serializer& s); \
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
  static optional<T> read(Serializer& s) {
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
  static std::vector<T> read(Serializer& s) {
    std::vector<T> t;
    size_t size = reader<size_t>::read(s);
    t.reserve(size);
    for (size_t n = 0; n < size; ++n) {
      t.push_back(reader<T>::read(s));
    }
    return t;
  }
};

namespace std {
  template <typename T>
  class shared_ptr;
}
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
  static std::shared_ptr<T> read(Serializer& s) {
    bool isNotNull = reader<bool>::read(s);
    if (isNotNull) {
      return std::make_shared<T>(reader<T>::read(s));
    } else {
      return std::shared_ptr<T>();
    }
  }
};

template <typename T>
void write(Serializer& s, T value) {
  writer<T>::write(s, value);
}

template <typename T>
T read(Serializer& s) {
  return reader<T>::read(s);
}

template <typename T>
void read(Serializer& s, T& value) {
  value = reader<T>::read(s);
}

#endif


