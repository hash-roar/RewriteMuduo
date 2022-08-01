#include <fmt/core.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>
#include <utility>

#include "log/Logger.h"
#include "network/NetAddress.h"
#include "network/Socket.h"
#include "unix/Thread.h"

using namespace rnet;
using namespace rnet::Network;
void process_request(Socket &socket) {
  std::cout << "sock fd:" << socket.fd() << "\n";
}

int main() {
  printf("dsafa\n");
  //   write(stderr->_fileno, "feafa", 6);
  auto listen_addr = InetAddress("127.0.0.1", 4999);
  auto sock = socket(AF_INET, SOCK_STREAM, NULL);
  auto listen_sock = Socket{sock};
  listen_sock.setReusePort(true);
  listen_sock.setReuseAddr(true);
  listen_sock.bindAddress(listen_addr);
  listen_sock.listen();

  while (true) {
    auto peeraddr = InetAddress{};
    auto fd = listen_sock.accept(&peeraddr);
    if (fd < 0) {
      LOG_ERROR << "accept error";
      continue;
    }

    fmt::print("accept peer addr:{}", peeraddr.toIpPort());
    Socket client_sock{fd};
    auto func = std::bind(process_request, std::move(client_sock));
    func();

    std::thread thread(process_request, std::move(client_sock));
    thread.join();
    // thread.detach();
  }

  return 0;
}