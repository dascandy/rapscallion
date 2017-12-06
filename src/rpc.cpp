#include "serializer.hpp"
#include <boost/asio.hpp>

namespace Rapscallion {

struct Connection {
  : public boost::enable_shared_from_this<Connection>
{ 
  constexpr size_t buffersize = 16384;
  Connection(tcp::socket&& socket, std::function<void(const char*, size_t)> onRead)
    : socket(std::move(socket))
    , onRead(onRead)
  {
  }

  tcp::socket& getSocket() {
    return socket_;
  }

  void start() {
    socket_.async_read_some(boost::asio::buffer(data_, max_length),
        boost::bind(&session::handle_read, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void write(const char* buffer, size_t size) {
    // We cannot send packets larger than the buffer size, as we cannot guarantee no interleaving happens.
    if (size > buffersize) throw runtime_error("Packet too large for connection");
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
      OnRead(buffer, bytes_transferred);
      socket_.async_read_some(boost::asio::buffer(buffer, buffersize), 
        [self = std::shared_from_this()](const boost::system::error_code& err, size_t transferred){ 
          handle_read(err, transferred);
        }
      );
    }
  }

  void queueWrite(const std::lock_guard<std::mutex>&) {
    boost::asio::async_write(socket_, boost::asio::buffer(currentWrite, bytes_transferred), 
      [self = std::shared_from_this()](boost::system::error_code& err) { 
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
  tcp::socket socket;
  char buffer[buffersize];
  char writebuffer1[buffersize];
  char writebuffer2[buffersize];
  size_t writeoff = 0;
  char* currentWrite = writebuffer1;
  std::mutex writeMutex;
};

struct Server {
  boost::asio::io_service& io_service;
  tcp::acceptor acceptor_;
  Server(uint16_t port, std::function<std::shared_ptr<Connection>> onConnect)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
    , onConnect(onConnect)
  {
    accept_one();
  }

  void accept_one() {
    std::shared_ptr<Connection> c = std::make_shared<Connection>(acceptor_.get_io_service());
    acceptor_.async_accept(c.getSocket(), [c](boost::system::error_code& ec) {
      if (!ec) {
        onConnect(c);
      }
      accept_one();
    });
  }

  void start_accept()
  {
    std::shared_ptr<Connection> c = std::make_shared<Connection>(acceptor_.get_io_service());
    acceptor_.async_accept(new_connection->socket(), [c = std::move(c)](boost::system::error_code ec){
      if (!error)
      {
        new_connection->start();
      }

      start_accept();
    });
  }
};


#include "connection.h"
#include <thread>
#include <vector>
#include "serializer.h"
#include <cstring>

#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_MORE
#define MSG_MORE 0
#endif

namespace rapscallion {

namespace {
  static uint8_t header[8] = { 0x0A, 0xCA, 0x11, 0x10, 0x01, 0x00, 0x00, 0x00 };

  bool writesocket(int fd, const void *buf, size_t bytes) {
    const char *sb = (const char *)buf;
    do {
      int written = send(fd, sb, (int)bytes, MSG_NOSIGNAL | 0);
      if (written < 0) {
        return false;
      } else {
        sb += written;
        bytes -= written;
      }
    } while (bytes);
    return true;
  }
}

Connection::Connection(int fd)
: fd(fd)
{
  pipe(p);
  writesocket(fd, header, sizeof(header));
}

Connection::Connection(const std::string& server, const std::string& port)
{
  pipe(p);
  struct addrinfo hints, *addrinfos;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  getaddrinfo(server.c_str(), port.c_str(), &hints, &addrinfos);
  fd = socket(addrinfos->ai_family, addrinfos->ai_socktype, addrinfos->ai_protocol);
  int nodelay = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&nodelay, sizeof(nodelay));
  connect(fd, addrinfos->ai_addr, addrinfos->ai_addrlen);
  freeaddrinfo(addrinfos);
  writesocket(fd, header, sizeof(header));
}

void Connection::startReceive(Callback& cb) {
  this->cb = &cb;
  std::shared_ptr<Connection> sharedthis = shared_from_this();
  std::thread readerThread([sharedthis]{ sharedthis->receive(); });
  readerThread.detach();
}

Connection::~Connection() 
{
  ::close(fd);
  ::close(p[0]);
  ::close(p[1]);
}

void Connection::Stop() {
  if (stopped) 
    return;
  stopped = true;
  write(p[1], "X", 1);
}

void Connection::send(Serializer& s) {
  if (stopped) return;
  std::unique_lock<std::mutex> lock(m);
  const auto& buffer = s.data();
  if (!writesocket(fd, buffer.first, buffer.second))
    Stop();
}

void Connection::receive() {
  uint8_t buffer[1024];
  bool gotHeader = false;
  fd_set fds;
  try {
    while (true) {
      FD_ZERO(&fds);
      FD_SET(fd, &fds);
      FD_SET(p[0], &fds);
      select(std::max(fd, p[0]) + 1, &fds, NULL, NULL, NULL);
      if (FD_ISSET(p[0], &fds))
        return;

      int bytesRead = ::recv(fd, buffer, 1024, 0);
      if (bytesRead <= 0) {
        std::unique_lock<std::mutex> lock(m);
        Stop();
        return;
      }

      receiveBuffer.insert(receiveBuffer.end(), buffer, buffer+bytesRead);
      size_t offset = 0;
      if (!gotHeader) {
        if (receiveBuffer.size() >= 8) {
          if (memcmp(receiveBuffer.data(), header, sizeof(header)) != 0) {
            // Incompatible client, disconnect
            return;
          }
          offset = 8;
        }
      }

      if (gotHeader) {
        while (Deserializer::PacketSize(receiveBuffer, offset) >= (int64_t)(receiveBuffer.size() - offset)) {
          Deserializer s(receiveBuffer, offset);
          offset += Deserializer::PacketSize(receiveBuffer, offset);
          cb->onPacket(s);
        }
      }
    }
  } catch (...) {}
}

}
