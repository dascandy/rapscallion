#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <functional>

namespace Rapscallion {

struct Connection
  : public boost::enable_shared_from_this<Connection>
{ 
  static constexpr size_t buffersize = 16384;
  Connection(boost::asio::ip::tcp::socket&& socket, std::function<void(const char*, size_t)> onRead)
    : socket(std::move(socket))
    , onRead(onRead)
  {
  }

  boost::asio::ip::tcp::socket& getSocket() {
    return socket;
  }

  void start() {
    socket.async_read_some(boost::asio::buffer(buffer, buffersize), [this](const boost::system::error_code& error, size_t transferred) {
      handle_read(error, transferred);
    });
  }

  void write(const char* buffer, size_t size) {
    // We cannot send packets larger than the buffer size, as we cannot guarantee no interleaving happens.
    if (size > buffersize) throw std::runtime_error("Packet too large for connection");
    std::lock_guard<std::mutex> l(writeMutex);

    // TODO: wait on cond var for connection to have enough space for this as a single write in the buffer


    memcpy(currentWrite + writeoff, buffer, size);
    writeoff += size;
    if (!writeActive) {
      queueWrite(l);
    }
  }

private:
  void handle_read(const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
      onRead(buffer, bytes_transferred);
      socket.async_read_some(boost::asio::buffer(buffer, buffersize), 
        [this, self = shared_from_this()](const boost::system::error_code& err, size_t transferred){ 
          handle_read(err, transferred);
        }
      );
    }
  }

  void queueWrite(const std::lock_guard<std::mutex>&) {
    boost::asio::async_write(socket, boost::asio::buffer(currentWrite, writeoff), 
      [this, self = shared_from_this()](const boost::system::error_code& err, size_t written) { 
        handle_write(err); 
      }
    );
    currentWrite = (currentWrite == writebuffer1) ? writebuffer2 : writebuffer1;
    writeoff = 0;
    writeActive = true;
    // signal condvar all
  }

  void handle_write(const boost::system::error_code& error) {
    if (!error) {
      std::lock_guard<std::mutex> l(writeMutex);
      if (writeoff > 0) {
        queueWrite(l);
      } else {
        writeActive = false;
      }
    }
  }

private:
  boost::asio::ip::tcp::socket socket;
  char buffer[buffersize];
  char writebuffer1[buffersize];
  char writebuffer2[buffersize];
  size_t writeoff = 0;
  char* currentWrite = writebuffer1;
  std::mutex writeMutex;
  bool writeActive = false;
  std::function<void(const char*, size_t)> onRead;
};

}

