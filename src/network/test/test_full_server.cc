
#include <fmt/core.h>

#include "network/EventLoop.h"
#include "network/NetAddress.h"
#include "network/TcpServer.h"
using namespace rnet;
using namespace rnet::Network;

int main() {
  EventLoop loop;
  auto listen_addr = InetAddress("127.0.0.1", 8080);
  TcpServer server(&loop, listen_addr, "test_server");
  server.start();
  loop.loop();
  return 0;
}