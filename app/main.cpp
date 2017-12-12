#include "Proxy.h"
#include "Dispatcher.h"
#include "RpcHost.h"
#include "RpcClient.h"
#include <thread>

using Rapscallion::RpcHost;
using Rapscallion::RpcClient;

struct IconAssets2Dispatcher;
struct IconAssets2Proxy;

struct IconAssets2 {
public:
  typedef IconAssets2Dispatcher Dispatcher;
  typedef IconAssets2Proxy Proxy;
  virtual future<std::string> getURI(std::string name) = 0;
};

struct IconAssets2Dispatcher : public Rapscallion::DispatcherBase<IconAssets2> {
  IconAssets2Dispatcher(IconAssets2* inst) 
  : DispatcherBase<IconAssets2>(inst)
  {
    DISPATCH_FUNC1(getURI, std::string, std::string);
  }
};

struct IconAssets2Proxy : public Rapscallion::ProxyBase<IconAssets2> {
  IconAssets2Proxy(RpcClient& conn)
  : Rapscallion::ProxyBase<IconAssets2>(conn)
  {}
  PROXY_FUNC1(getURI, std::string, std::string)
};

struct MyIconAssets : IconAssets2 {
  MyIconAssets() {
    printf("%s\n", __PRETTY_FUNCTION__);

  }
  future<std::string> getURI(std::string name) override {
    printf("Somebody called MyIconAssets' function. Arg == %s\n", name.c_str());
    return boost::make_ready_future<std::string>("Hello back!");
  }
};

void server(boost::asio::io_service &service, MyIconAssets& mia) {
  RpcHost host(service, 4400);
  host.Register(&mia);
  service.run();
}

int main() {
  MyIconAssets mia;
  boost::asio::io_service io_service;
  std::thread([&mia, &io_service](){ server(io_service, mia); }).detach();
  boost::asio::ip::tcp::resolver resolver(io_service);
  boost::asio::ip::tcp::resolver::query query("localhost", "4400");
  boost::asio::ip::tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);
  std::shared_ptr<boost::asio::ip::tcp::socket> socket = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
  boost::asio::connect(*socket, endpoint_iterator);
  RpcClient client(socket);
  IconAssets2* ia2 = client.Get<IconAssets2>();
  auto f = ia2->getURI("Test remote!");
  auto f2 = mia.getURI("Test local!");
  std::cout << f.get() << "\n";
  std::cout << f2.get() << "\n";
}


