#pragma once

#include "headers.hpp"
#include "thread_safe_queue.hpp"
#include "message.hpp"
#include "connection.hpp"
#include <boost/numeric/conversion/detail/is_subranged.hpp>
#include <boost/system/system_error.hpp>
#include <limits>

using boost::asio::ip::tcp;

class ServerInterface {
 protected:
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::io_context context_;
  std::thread context_thread_;

  std::deque<std::shared_ptr<Connection>> connections_;
  ThreadSafeQueue<ConnectionMessage> input_messages_;

  uint32_t user_cnt = 0;
 public:
  ServerInterface(uint16_t port) :
    acceptor_(context_, tcp::endpoint(tcp::v4(), port)) {}

  ~ServerInterface() {
    Stop();
  }

  bool Start() {
    try {
      AcceptConnection();

      // run contex after connection establised
      context_thread_ = std::thread(
        [this](){
          context_.run();
        });
    } catch (std::exception& ex) {
      std::cerr << "Failed to start server: " << ex.what() << '\n';
      return false;
    }
    return true;
 }

  void Stop() {
    context_.stop();

    if (context_thread_.joinable()) {
     context_thread_.join();
    }
 }
 
 void AcceptConnection() {
   acceptor_.async_accept(
     [this](boost::system::error_code& ec, tcp::socket socket){
       if (!ec) {
         std::shared_ptr<Connection> connection = std::make_shared<Connection>(context_, std::move(socket), input_messages_, Connection::Authority::Server);

         connections_.push_back(std::move(connection));        
         // give a task to wait async to read incoming messages
         connections_.back()->ConnectToClient(user_cnt++);

         std::cout << "Connection succeeded -- id: " << user_cnt << '\n';
       } else {
         std::cerr << "Connection failed: " << ec.message() << '\n';
       }

       // register new connections async
       AcceptConnection();
     }
   );
 }


  void SendMessage(std::shared_ptr<Connection> client, const Message& msg) {
    if (client != nullptr && client->IsConnected()) {
      client->SendMessage(msg);
    } else {
      client.reset();
      
      // erease-remove idiom
      connections_.erase(
        std::remove(connections_.begin(), connections_.end(), client), connections_.end()
      );
    }
  }

  void BroadCast(const Message& msg) {
    bool found_null = false;

    for (auto& client : connections_) {
      if (client != nullptr && client->IsConnected()) {
        client->SendMessage(msg);
      } else {
        client.reset();

        found_null = true;
      }
    }

    // remove unavailable clients
    if (found_null) {
      connections_.erase(
        std::remove(connections_.begin(), connections_.end(), nullptr), connections_.end()
      );
    }
  }

  void ReadMessages(size_t cnt = std::numeric_limits<size_t>::max()) {
    size_t read_cnt = 0;

    while (read_cnt < cnt && !input_messages_.empty()) {
      ConnectionMessage msg =input_messages_.pop_front();

      HandleMessage(msg.remote, msg.msg);

      ++read_cnt;
    }
  }

  void HandleMessage(std::shared_ptr<Connection> client, Message& msg) {
    // leave for inherited implementation
  }

};
