#include <boost/asio.hpp>

namespace Rapscallion {

struct RpcHost {
  RpcHost(boost::asio::io_service &io_service, uint16_t port)
  : server(io_service, port, [](std::shared_ptr<tcp::socket> s){ addConnection(s); })
  {}
  void addConnection(std::shared_ptr<tcp::socket> socket) {
    std::lock_guard<std::mutex> l(m);
    std::shared_ptr<Connection> conn = std::make_shared<Connection>(socket, [](const char* data, size_t size){ ... do something });
    connections.push_back(conn);
    for (auto& interface : interfaces) {
      sendInterface(conn, interface);
    }
  }
  template <typename T>
  void Register(T* handler) {
    std::lock_guard<std::mutex> l(m);
    std::unique_ptr<InterfaceStub> iface = std::make_unique<T::Stub>(handler);
    RegisterStub(new T::Stub(handler));
    for (auto& conn : connections) {
      sendInterface(conn, interface);
    }
  }
  void sendInterface(std::shared_ptr<Connection>& conn, std::unique_ptr<InterfaceStub>& interface) {
    
  }
  std::mutex m;
  std::vector<std::unique_ptr<InterfaceStub>> interfaces;
  std::vector<std::shared_ptr<Connection> connections;
};

struct RpcClient {
  RpcClient(std::shared_ptr<Connection> connection) 
  : connection(std::move(connection))
  {}
  ~RpcClient() {
    for (auto& proxy : proxies) {
      proxy->SignalDisconnect();
    }
  }
  template <typename T>
  T* Get() {
    T* proxy = new T::Proxy(connection);
    RegisterProxy(proxy);
    return proxy;
  }
  std::vector<InterfaceProxy*> proxies;
  std::shared_ptr<Connection> connection;
};

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
  tcp::acceptor acceptor_;

  Server(boost::asio::io_service &io_service, uint16_t port, std::function<std::shared_ptr<Connection>> onConnect)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
    , onConnect(onConnect)
  {
    accept_one();
  }

  void accept_one() {
    std::shared_ptr<tcp::socket> socket = std::make_shared<tcp::socket>(acceptor_.get_io_service());
    acceptor_.async_accept(socket, [socket](boost::system::error_code& ec) {
      if (!ec) {
        onConnect(socket);
        accept_one();
      }
    });
  }
};


