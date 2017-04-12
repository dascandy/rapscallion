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

struct Serializer;
struct Deserializer;
struct Callback {
  virtual void onPacket(Deserializer& d) = 0;
};

class Connection : public std::enable_shared_from_this<Connection> {
public:
  Connection(int fd);
  Connection(const std::string& server, const std::string& port);
  ~Connection();
  void startReceive(Callback& cb);
  void Stop();
  void send(Serializer& s);
  void receive();
private:
  int fd;
  int p[2];
  std::atomic<bool> stopped;
  std::thread receiveThread;
  std::vector<uint8_t> receiveBuffer;
  std::mutex m;
  std::condition_variable cv;
  Callback* cb;
};

#endif


