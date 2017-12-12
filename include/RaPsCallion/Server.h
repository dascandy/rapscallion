#pragma once

#include <boost/asio.hpp>
#include <functional>
#include <cstdint>

namespace Rapscallion {

struct Server {
  boost::asio::ip::tcp::acceptor acceptor_;
  std::function<void(boost::asio::ip::tcp::socket)> onConnect_;

  Server(boost::asio::io_service &io_service, uint16_t port, std::function<void(boost::asio::ip::tcp::socket)> onConnect)
    : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    , onConnect_(onConnect)
  {
    accept_one();
  }

  void accept_one() {
    std::shared_ptr<boost::asio::ip::tcp::socket> socket = std::make_shared<boost::asio::ip::tcp::socket>(acceptor_.get_io_service());
    acceptor_.async_accept(*socket.get(), [this, socket](const boost::system::error_code& ec) {
      if (!ec) {
        onConnect_(std::move(*socket));
        accept_one();
      }
    });
  }
};

}


