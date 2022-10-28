#pragma once

#include <netdb.h>
#include <netinet/in.h>

#include <string_view>

#include "base/Common.h"
#include "network/SocketOps.h"
namespace rnet::network {
class InetAddress : public Copyable {
 public:
  /// Constructs an endpoint with given port number.
  /// Mostly used in TcpServer listening.
  explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false,
                       bool ipv6 = false);

  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(std::string_view ip, uint16_t port, bool ipv6 = false);

  /// Constructs an endpoint with given struct @c sockaddr_in
  /// Mostly used when accepting new connections
  explicit InetAddress(const struct sockaddr_in& addr) : addr_(addr) {}

  explicit InetAddress(const struct sockaddr_in6& addr) : addr6_(addr) {}

  sa_family_t Family() const { return addr_.sin_family; }
  std::string ToIp() const;
  std::string ToIpPort() const;
  uint16_t Port() const;

  // default copy/assignment are Okay

  const struct sockaddr* GetSockAddr() const {
    return network::sockets::sockaddr_cast(&addr6_);
  }
  void SetSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }

  uint32_t Ipv4NetEndian() const;
  uint16_t PortNetEndian() const { return addr_.sin_port; }

  // resolve hostname to IP address, not changing port or sin_family
  // return true on success.
  // thread safe
  static bool Resolve(std::string_view hostname, InetAddress* result);
  // static std::vector<InetAddress> resolveAll(const char* hostname, uint16_t
  // port = 0);

  // set IPv6 ScopeID
  void SetScopeId(uint32_t scope_id);

 private:
  union {
    struct sockaddr_in addr_;
    struct sockaddr_in6 addr6_;
  };
};
}  // namespace rnet::Network