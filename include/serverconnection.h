#ifndef SERVERCONNECTION_H
#define SERVERCONNECTION_H

#include <vector>
#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <thread>

class Connection;

class ServerConnection {
public:
  ServerConnection(std::string port);
  ~ServerConnection();
  void run();
  std::function<void(std::shared_ptr<Connection>)> onConnection = [](std::shared_ptr<Connection>){};
private:
  std::atomic<bool> stop;
  int fd;
  int p[2];
  std::vector<std::shared_ptr<Connection>> connections;
  std::thread receiveThread;
};

#endif


