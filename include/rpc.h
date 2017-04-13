#ifndef RPC_H
#define RPC_H

namespace rapscallion {

struct IResult {
  virtual void fill(Deserializer& d) = 0;
};

struct server {
  
};

struct client {
  client(const std::string& server, uint16_t port)
  : c(server, port)
  {
    Serializer s;
    s.write((uint32_t)RAPSCALLION_VERSION);
    c.send(s);
  }
  template <typename RV, typename... Args>
  handle<RV> Invoke(size_t functionId, Args... args) {
    size_t myId = maxId++;
    Serializer s;
    s.write(myId, functionId, args...);
    c.send(s);
    std::shared_ptr<result<RV>> result = std::make_shared<result<RV>>(this, myId);
    results[myId] = result;
    return handle<RV>(result);
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
