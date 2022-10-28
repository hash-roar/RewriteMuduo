#pragma once
#include <netinet/tcp.h>

#include "base/Common.h"

namespace rnet::network {
class InetAddress;

///
/// Wrapper of socket file descriptor.
///
/// It closes the sockfd when destruct.
/// It's thread safe, all operations are down to OS.
class Socket : Noncopyable {
  static const int nonValidSocketFd = -1;

 public:
  explicit Socket(int sockfd) : sockfd_(sockfd) {}

  Socket(Socket&& other) {
    sockfd_ = other.sockfd_;
    other.sockfd_ = nonValidSocketFd;
  }
  void operator=(Socket&& other) {
    sockfd_ = other.sockfd_;
    other.sockfd_ = nonValidSocketFd;
  }
  // Socket(Socket&&) // move constructor in C++11
  ~Socket();

  int Fd() const { return sockfd_; }
  // return true if success.
  bool GetTcpInfo(struct tcp_info*) const;
  bool GetTcpInfoString(char* buf, int len) const;

  /// abort if address in use
  void BindAddress(const InetAddress& localaddr);
  /// abort if address in use
  void Listen();

  /// On success, returns a non-negative integer that is
  /// a descriptor for the accepted socket, which has been
  /// set to non-blocking and close-on-exec. *peeraddr is assigned.
  /// On error, -1 is returned, and *peeraddr is untouched.
  int Accept(InetAddress* peeraddr);

  void ShutdownWrite();

  ///
  /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
  ///
  void SetTcpNoDelay(bool on);

  ///
  /// Enable/disable SO_REUSEADDR
  ///
  void SetReuseAddr(bool on);

  ///
  /// Enable/disable SO_REUSEPORT
  ///
  void SetReusePort(bool on);

  ///
  /// Enable/disable SO_KEEPALIVE
  ///
  void SetKeepAlive(bool on);

 private:
  int sockfd_;
};
}  // namespace rnet::Network