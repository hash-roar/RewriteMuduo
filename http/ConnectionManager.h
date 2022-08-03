#pragma once
#include <unordered_set>

#include "HttpConnection.h"
namespace http {
class ConnectionManager {
 public:
  using Connections = std::unordered_set<ConnectionPtr>;
  ConnectionManager() = default;
  ~ConnectionManager() = default;

  void start(ConnectionPtr p);
  void stop(ConnectionPtr p);

  void stopAllConnection();

 private:
  Connections connections_;
};
}  // namespace http