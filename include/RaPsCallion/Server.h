#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <cstdint>

namespace Rapscallion {

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

}


