#pragma once

#include "headers.hpp"
#include "thread_safe_queue.hpp"
#include "message.hpp"
#include "connection.hpp"

using boost::asio::ip::tcp;


// Specific client class will be
// defined to abstract from
// connections and to think only
// about message send and recieve
class ClientInterface {
 private:
  ThreadSafeQueue<ConnectionMessage> input_messages_;

 protected:
  boost::asio::io_context context_;
  std::thread context_thread_;
  std::unique_ptr<Connection> connection_;

 public:
  ClientInterface() {}

  ThreadSafeQueue<ConnectionMessage>& GetInputMessages() {
    return input_messages_;
  }
  
  bool Connect(const std::string& host, uint16_t port) {
    try {

      tcp::resolver resolver(context_);
      tcp::resolver::results_type endpoint = resolver.resolve(host, std::to_string(port));

      // no need to save socket in client interface
      // as it is managed by connection class
      connection_ = std::make_unique<Connection>(context_, tcp::socket(context_), input_messages_, Connection::Authority::Client); 

      connection_->ConnectToServer(endpoint);

      // there must be a task before running
      // context, and that task is async read
      // after ConnectToServer called
      context_thread_ = std::thread([this](){
                                      context_.run();
                                    });  

      // if ConnectToServer failed, then
      // after leaving the scope connection_
      // will be destroyed
    } catch (std::exception& ex) {
      std::cerr << "Error on connect: " << ex.what() << '\n';
      return false;
    }
    return true;
  }

  void Disconnect() {
    if (ConnectionValid()) {
      connection_->Disconnect();
    }

    // stop context and thread
    // that manages context
    context_.stop();

    if (context_thread_.joinable()) {
      context_thread_.join();
    }

    // get rid of unique_ptr
    connection_.release();
  }

  bool ConnectionValid() {
    if (connection_ != nullptr) {
      return connection_->IsConnected();
    }
    return false;
  }

  void SendMessage(const Message& msg) {
    if (ConnectionValid()) {
      connection_->SendMessage(msg);
    }
  }

};

