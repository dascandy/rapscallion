#ifndef RPC_H
#define RPC_H

namespace rpc {

struct IResult {
  virtual void fill(Deserializer& d) = 0;
};

struct server {
  server(uint16_t port)
  : sc(port)
  {
    sc.onConnection([this](std::shared_ptr<Connection> conn) {
      addConnection(conn);
    });
  }
  void addConnection(std::shared_ptr<Connection> conn) {
    // TODO: do something with this connection
  }
  ServerConnection sc;
};

struct client {
  client(const std::string& server, uint16_t port)
  : c(server, port)
  {}
  template <typename RV, typename... Args>
  rpc::handle<RV> Invoke(size_t functionId, Args... args) {
    size_t myId = maxId++;
    Serializer s;
    s.write(myId, functionId, args...);
    c.send(s);
    std::shared_ptr<rpc::result<RV>> result = std::make_shared<rpc::result<RV>>(this, myId);
    results[myId] = result;
    return rpc::handle<RV>(result);
  }
  void request_value(size_t requestId) {
    auto it = results.find(requestId);
    if (it == results.end()) return; // Invalid request id - assert?
    std::shared_ptr<IResult> handle = it->second.lock();
    if (!handle) {
      // result declared irrelevant. Who's asking for it now then? Assert?
      return;
    }
    requested.insert(std::make_pair(requestId, handle));
    Serializer s;
    s.write(requestId);
    s.write(0);
    c.send(s);
  }
  void drop_request(size_t requestId) {
    auto it = results.find(requestId);
    if (it == results.end()) return; // Invalid request id - assert?
    Serializer s;
    s.write(requestId);
    s.write(1);
    c.send(s);
    results.erase(it);
    requested.erase(requestId);
  }
  Connection c;
  std::map<size_t, std::weak_ptr<IResult>> results;
  std::map<size_t, std::shared_ptr<IResult>> requested;
  size_t maxId = 1;
};

}

#endif


