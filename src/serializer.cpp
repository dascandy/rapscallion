#include "serializer.h"
#include <cstdlib>

void writer<size_t>::write(Serializer& s, const size_t &val) {
  size_t value = val;
  while (value >= 0x80) {
    s.buffer->push_back((uint8_t)(value & 0x7F) | 0x80);
    value >>= 7;
  }
  s.buffer->push_back((uint8_t)value);
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
  s.buffer->insert(s.buffer->end(), value.begin(), value.end());
}
void writer<bool>::write(Serializer& s, const bool &b) {
  writer<size_t>::write(s, b ? 1 : 0);
}

size_t reader<size_t>::read(Serializer& s) {
  size_t val = 0;
  size_t offs = 0;
  if (s.offset >= s.buffer->size()) 
    throw parse_exception("buffer underrun");
  while ((*s.buffer)[s.offset] & 0x80) {
    val |= ((size_t)(*s.buffer)[s.offset] & 0x7F) << offs;
    offs += 7;
    s.offset++;
    if (s.offset >= s.buffer->size()) 
      throw parse_exception("buffer underrun");
  }
  val |= (*s.buffer)[s.offset] << offs;
  s.offset++;
  return val;
}
long reader<long>::read(Serializer& s) {
  size_t val = reader<size_t>::read(s);
  long value = (long)(val >> 1);
  if (val & 1) value = -value;
  return value;
}
int reader<int>::read(Serializer& s) {
  return reader<long>::read(s);
}
std::string reader<std::string>::read(Serializer& s) {
  size_t length = reader<size_t>::read(s);
  size_t offs = s.offset;
  s.offset += length;
  if (s.offset > s.buffer->size())
    throw parse_exception("buffer underrun");
  return std::string((const char *)s.buffer->data()+offs, length);
}
bool reader<bool>::read(Serializer& s) {
  return reader<size_t>::read(s) > 0;
}

void Serializer::addBytes(const uint8_t *newbuffer, size_t bytesRead) {
  bool lengthKnown = false;
  for (uint8_t b : *buffer) {
    if ((b & 0x80) == 0) {
      lengthKnown = true;
      break;
    }
  }
  if (!lengthKnown) {
    // Pre-read buffer until we know the desired length
    while (bytesRead) {
      buffer->push_back(*newbuffer++);
      bytesRead--;
      if ((buffer->back() & 0x80) == 0x00) {
        lengthKnown = true;
        break;
      }
    }
  }
  if (!lengthKnown) {
    // Buffer used and still no length known
    return;
  }
  offset = 0;
  size_t length = read<size_t>(*this);
  buffer->reserve(length+offset);
  if (buffer->size() + bytesRead >= length + offset) {
    size_t bytesToGetFromNewbuffer = (length + offset) - (buffer->size());
    buffer->insert(buffer->end(), newbuffer, newbuffer + bytesToGetFromNewbuffer);
    newbuffer += bytesToGetFromNewbuffer;
    bytesRead -= bytesToGetFromNewbuffer;
//    onReceive(*this);
    buffer->clear();
    addBytes(newbuffer, bytesRead);
  } else {
    buffer->insert(buffer->end(), newbuffer, newbuffer + bytesRead);
  }
}


