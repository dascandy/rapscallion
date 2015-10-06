#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <string>
#include <map>
#include <functional>
#include <thread>
#include <condition_variable>
#include <vector>
#include <atomic>

#ifdef _WIN32
#define SOCKET void*
#else
#define SOCKET int
#endif

struct Serializer;
class IProxy;
class IHasDispatch;

class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection(SOCKET fd);
  Connection(std::string server, std::string port);
  void startReceive();
  ~Connection();
  void Stop(bool inDestructor = false);
  void send(Serializer& s);
  void receive();
  void Handle(Serializer&s); 
  std::map<std::string, std::shared_ptr<IProxy>> proxies;
  template <typename T>
  std::shared_ptr<T> getProxyFor();
  void sendInterfaces();
  SOCKET fd;
  std::atomic<bool> stopped;
  int p[2];
  std::thread receiveThread;
  std::mutex m;
  std::condition_variable cv;
  std::vector<std::string> interfaces;
  bool haveInterfaces = false;
  std::function<void(std::shared_ptr<Connection>)> onStop = [](std::shared_ptr<Connection>){};
  static std::map<std::string, IHasDispatch *> &dispatchers();
};

#endif


