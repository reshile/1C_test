#pragma once

#include "headers.hpp"

enum MessageType : uint32_t {
  Broadcast,
  Greater,
  Equals,
  Less,
  Response,
};

struct Message {
  MessageType ms_type{};
  int64_t number{};
};

class Connection;

// Will be handled by connection object
struct ConnectionMessage {
  std::shared_ptr<Connection> remote = nullptr;
  Message msg;
};
