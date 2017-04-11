#include "serverconnection.h"
#include "connection.h"
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

ServerConnection::ServerConnection(std::string port)
{
  pipe(p);
  stop = false;
  struct addrinfo hints, *result;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = IPPROTO_TCP;

  getaddrinfo(NULL, port.c_str(), &hints, &result);
  if (!result)
    throw 42;
  fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (bind(fd, result->ai_addr, (int)result->ai_addrlen) != 0)
    throw 42;
  if (listen(fd, 5) != 0)
    throw 42;
  freeaddrinfo(result);

  receiveThread = std::thread([this]{ run(); });
}

ServerConnection::~ServerConnection() {
  write(p[1], "X", 1);
  receiveThread.join();
  close(fd);
  close(p[0]);
  close(p[1]);
}

void ServerConnection::run() {
  fd_set fds;
  while (true) {
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    FD_SET(p[0], &fds);
    select(std::max(fd, p[0]) + 1, &fds, NULL, NULL, NULL);
    if (FD_ISSET(p[0], &fds))
      return;

    struct sockaddr_in in;
    memset(&in, 0, sizeof(in));
    socklen_t len = sizeof(in);
    int cfd = accept(fd, (struct sockaddr*)&in, &len);
    std::shared_ptr<Connection> conn = std::make_shared<Connection>(cfd);
    connections.push_back(conn);
    onConnection(conn);
  }
}


