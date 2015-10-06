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
    SOCKET cfd = accept(fd, (struct sockaddr*)&in, &len);
    std::shared_ptr<Connection> conn = std::make_shared<Connection>(cfd);
    connections.push_back(conn);
    onConnection(conn);
  }
}

#include "future.h"
#include "optional.h"
#include "rpc.h"
#include "Test.h"

class TestInterface {
public:
  virtual future<void> TestCall(std::vector<int> a, optional<size_t> b, std::shared_ptr<std::string> c) = 0;
};

class TestInterfaceProxy : public ProxyBase<TestInterface> {
public:
  TestInterfaceProxy(std::shared_ptr<Connection> conn)
  : ProxyBase<TestInterface>(conn)
  {
  }
  PROXY_FUNC3(TestCall, void, std::vector<int>, optional<size_t>, std::shared_ptr<std::string>);
};

class TestInterfaceDispatch : public DispatchBase<TestInterface> {
public:
  TestInterfaceDispatch(TestInterface *conn)
  : DispatchBase<TestInterface>(conn)
  {
    DISPATCH_PROLOG(TestCall) 
    std::vector<int> a1 = read<std::vector<int>>(s); 
    optional<size_t> a2 = read<optional<size_t>>(s); 
    std::shared_ptr<std::string> a3 = read<std::shared_ptr<std::string>>(s); 
    future<void> val = this->cb->TestCall(a1, a2, a3); 
    DISPATCH_EPILOGv(void);
  }
};

REGISTER(TestInterface)

class ConcreteTestInterface : public TestInterface {
public:
  ConcreteTestInterface() {
    DispatchBase<TestInterface>::Provide(this);
  }
  future<void> TestCall(std::vector<int> a, optional<size_t> b, std::shared_ptr<std::string> c) override {
    std::vector<int> t = {1, 2, 16384};
    ASSERT_EQ(a, t);
    ASSERT_EQ(b.get(), 42u);
    ASSERT_STREQ(*c.get(), "TestValue");
    return async([]{});
  }
} intf;

TEST(CanConnectAndSendReceiveData) {
  char port[6];
  uint16_t p = 1283;
  ServerConnection *server = NULL;
  while (!server) {
    p++;
    sprintf(port, "%d", p);
    try {
      server = new ServerConnection(port);  
    } catch (...) {}
  }
  std::shared_ptr<Connection> otherHalf;
  server->onConnection = [&otherHalf](std::shared_ptr<Connection> c){ c->startReceive(); otherHalf = c; };
  std::shared_ptr<Connection> client = std::make_shared<Connection>("localhost", port);
  client->startReceive();
  std::shared_ptr<TestInterface> proxy = client->getProxyFor<TestInterface>();
  future<void> done = proxy->TestCall({1, 2, 16384}, optional<size_t>(42), std::make_shared<std::string>("TestValue"));
  done.get();
  delete server;
}
/*
TEST(CanConnectAndSendReceiveStressTest) {
  char port[6];
  uint16_t p = 1283;
  ServerConnection *server = NULL;
  while (!server) {
    p++;
    sprintf(port, "%d", p);
    try {
      server = new ServerConnection(port);  
    } catch (...) {}
  }
  std::shared_ptr<Connection> otherHalf;
  server->onConnection = [&otherHalf](std::shared_ptr<Connection> c){ c->startReceive(); otherHalf = c; };
  std::shared_ptr<Connection> client = std::make_shared<Connection>("localhost", port);
  client->startReceive();
  std::shared_ptr<TestInterface> proxy = client->getProxyFor<TestInterface>();
  std::vector<future<void>> v;
  for (size_t i = 0; i < 1048576; i++) {
    v.push_back(proxy->TestCall({1, 2, 16384}, optional<size_t>(42), std::make_shared<std::string>("TestValue")));
  }
  when<all>(v).then([](const future<std::vector<future<void>>>&){}).get();
  delete server;
}
*/

