
#include <fmt/core.h>

#include "file/ConnBuffer.h"
#include "log/Logger.h"
#include "network/Callback.h"
#include "network/EventLoop.h"
#include "network/NetAddress.h"
#include "network/TcpConnection.h"
#include "network/TcpServer.h"
#include "unix/Time.h"
using namespace rnet;
using namespace rnet::Network;
using namespace rnet::File;
using namespace rnet::Unix;

void outputString(const TcpConnectionPtr& ptr, Buffer* buffer, Timestamp time) {
  fmt::print(buffer->retrieveAllAsString());
}

int main() {
  log::Logger::setLogLevel(log::Logger::TRACE);
  EventLoop loop;
  auto listen_addr = InetAddress("127.0.0.1", 8080);
  TcpServer server(&loop, listen_addr, "test_server");
  server.setMessageCallback(outputString);
  loop.runEvery(2.5, [] { fmt::print("hello\n"); });
  loop.runAfter(2.2, [] { fmt::print("this is------------>\n"); });
  server.start();
  loop.loop();
  return 0;
}


