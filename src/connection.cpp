#include "connection.h"
#include <thread>
#include <vector>
#include "serializer.h"
#include <cstring>
#include "rpc.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
inline void close(SOCKET fd) {
  closesocket(fd);
}
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#define SOCKET int
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifndef MSG_MORE
#define MSG_MORE 0
#endif

bool writesocket(SOCKET fd, const void *buf, size_t bytes, bool lastWrite = true) {
  const char *sb = (const char *)buf;
  do {
    int written = send(fd, sb, (int)bytes, MSG_NOSIGNAL | (lastWrite ? 0 : MSG_MORE));
    if (written < 0) {
      return false;
    } else {
      sb += written;
      bytes -= written;
    }
  } while (bytes);
  return true;
}

Connection::Connection(SOCKET fd)
: fd(fd)
{
  pipe(p);
  sendInterfaces();
}

Connection::Connection(std::string server, std::string port)
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
  sendInterfaces();
}

void Connection::sendInterfaces() {
  std::vector<std::string> ifs;
  for (auto p : dispatchers()) {
    if (p.first[0] == 'd') {
      ifs.push_back(p.first.c_str() + 1);
    }
  } 
  Serializer s;
  write(s, std::string(""));
  write(s, ifs);
  // Copy of logic from send(), as this code must run when interfaces have not been received yet (obviously).
  Serializer ss;
  write(ss, s.buffer->size());
  ::writesocket(fd, ss.buffer->data(), ss.buffer->size(), true);
  ::writesocket(fd, s.buffer->data(), s.buffer->size());
}

void Connection::startReceive() {
  receiveThread = std::move(std::thread([this]{ this->receive(); }));
}

Connection::~Connection() 
{
  Stop(true);
  if (std::this_thread::get_id() != receiveThread.get_id())
    receiveThread.join();
  ::close(fd);
  ::close(p[0]);
  ::close(p[1]);
}

void Connection::Stop(bool inDestructor) {
  if (stopped) 
    return;
  stopped = true;
  if (!haveInterfaces) {
    haveInterfaces = true;
    cv.notify_all();
  }
  write(p[1], "X", 1);
  for (const auto &psb : proxies) {
    if (psb.second) 
      psb.second->signalDisconnect();
  }
  if (!inDestructor)
  {
    std::shared_ptr<Connection> p = shared_from_this();
    async([p]{
      p->onStop(p); 
    });
  }
}

void Connection::send(Serializer& s) {
  if (stopped) return;
  std::unique_lock<std::mutex> lock(m);
  cv.wait(lock, [this]{ return haveInterfaces; });
  Serializer ss;
  write(ss, s.buffer->size());
  if (!::writesocket(fd, ss.buffer->data(), ss.buffer->size(), false) ||
      !::writesocket(fd, s.buffer->data(), s.buffer->size(), true))
    Stop();
}

void Connection::receive() {
  std::shared_ptr<Connection> sharedthis = shared_from_this();
  uint8_t buffer[1024];
  Serializer s;

  fd_set fds;
  while (true) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    FD_SET(p[0], &fds);
    select(std::max(fd, p[0]) + 1, &fds, NULL, NULL, NULL);
    if (FD_ISSET(p[0], &fds))
      return;

    int bytesRead = ::recv(fd, buffer, 1024, 0);
    if (bytesRead > 0)
      s.addBytes(buffer, bytesRead, [sharedthis](Serializer&s){ sharedthis->Handle(s); });
    else {
      std::unique_lock<std::mutex> lock(m);
      Stop();
    }
  }
}

void Connection::Handle(Serializer& s) {
  std::string intf = read<std::string>(s);
  if (intf == "") {
    std::unique_lock<std::mutex> lock(m);
    interfaces = read<std::vector<std::string>>(s);
    haveInterfaces = true;
    cv.notify_all();
  } else if (intf[0] == 'd') {
    IHasDispatch *disp = dispatchers()[intf];
    if (!disp)
      printf("Received call for unknown interface %s\n", intf.c_str());
    else
      disp->dispatch(s, shared_from_this());
  } else {
    std::shared_ptr<IProxy> psb = proxies[intf.substr(1)];
    if (!psb) {
      printf("No proxy for %s\n", intf.c_str());
      return;
    }
    psb->dispatch(s);
  }
}

std::map<std::string, IHasDispatch *> &Connection::dispatchers() {
  static std::map<std::string, IHasDispatch *> _r;
  return _r;
}


