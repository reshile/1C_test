#pragma once

#include "headers.hpp"
#include "message.hpp"
#include "thread_safe_queue.hpp"
#include <boost/asio/connect.hpp>
#include <boost/system/detail/error_code.hpp>

using boost::asio::ip::tcp;


class Connection : public std::enable_shared_from_this<Connection> {
 public:
  enum Authority : uint32_t {
    Server,
    Client,
  };

 public: 
  Connection(boost::asio::io_context& context, tcp::socket socket, ThreadSafeQueue<ConnectionMessage>& input_messages, Authority owner) :
    context_(context),
    socket_(std::move(socket)),
    input_messages_(input_messages),
    owner_(owner) {}

  ~Connection() {}

  void SendMessage(const Message& msg) {
    boost::asio::post(context_, 
      [this, msg](){

        // it is possible that when adding new message
        // to output queue, there is already write message
        // task in context runner, that is why need
        // to check if output messages for emptiness
        bool is_empty = output_messages_.empty();
        output_messages_.push_back(msg);

        // create task of writing (sending) message
        // in case there is no other conflicting
        // WriteMessage task
        if (is_empty) {
          WriteMessage();
        }
      });
  }
  // For clients only
  void ConnectToServer(tcp::resolver::results_type& endpoint) {
    if (owner_ == Server) {
      return;
    }

    boost::asio::async_connect(socket_, endpoint,
      [this](const boost::system::error_code& er, tcp::endpoint endpoint){
        if (!er) {
          // Put a reading task into context
          // wait asynchronously for servers incoming messages
          ReadMessage();
        } else {
          std::cerr << "Error when connecting to server: " << er.message() << '\n';
        }
      });
  }

  // For server only
  void ConnectToClient(uint32_t user_id = 0) {
    if (owner_ == Client) {
      return;
    }

    // Async wait for client messages
    if (IsConnected()) {
      id = user_id;
      ReadMessage();
    }
  }

  // For both clients and server
  void Disconnect() {
    if (IsConnected()) {
      boost::asio::post(context_,
        [this](){
          socket_.close();
        });
    }
  }

  bool IsConnected() const {
    return socket_.is_open();
  }

 private:
  void ReadMessage() {
    boost::asio::async_read(socket_, boost::asio::buffer(&buffer, sizeof(Message)),
      [this](const boost::system::error_code& ec, size_t bytes_read){
        if (!ec) {
          AddMessageToInput();
        } else {
          std::cout << "Error on read operation: " << ec.message() << '\n';
          socket_.close();
        }
      });
  }

  void WriteMessage() {
    boost::asio::async_write(socket_, boost::asio::buffer(&output_messages_.front(), sizeof(Message)),
      [this](const boost::system::error_code& ec, size_t bytes_write){
        if (!ec) {
          output_messages_.pop_front();

          // In case there is something write
          // replace old task with new one
          if (!output_messages_.empty()) {
            WriteMessage();
          }
        } else {
          std::cerr << "Error on write operation: " << ec.message() << '\n';
        }
      });
  }

  void AddMessageToInput() {
    if (owner_ == Client) {
      input_messages_.push_back({nullptr, buffer});
    } else {
      input_messages_.push_back({this->shared_from_this(), buffer});
    }

    // one read operation was handled, so need to
    // put same task to pool of tasks, i.e. context
    ReadMessage();
  }

 private:
  boost::asio::ip::tcp::socket socket_;
  boost::asio::io_context& context_;
  ThreadSafeQueue<Message> output_messages_;
  ThreadSafeQueue<ConnectionMessage>& input_messages_;
  Authority owner_;

  Message buffer;
  uint32_t id{};
};
