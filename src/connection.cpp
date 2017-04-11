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

Connection::Connection(int fd)
: fd(fd)
{
  pipe(p);
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
}

void Connection::startReceive() {
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
  if (!::writesocket(fd, buffer.first, buffer.second))
    Stop();
}

void Connection::receive() {
  uint8_t buffer[1024];

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
      while (Deserializer::PacketSize(receiveBuffer, offset) >= receiveBuffer.size() - offset) {
        Deserializer s(receiveBuffer, offset);
        offset += Deserializer::PacketSize(receiveBuffer, offset);
        // TODO: handle packet
      }
    }
  } catch (...) {}
}


