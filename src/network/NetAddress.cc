#include "network/NetAddress.h"

#include <cassert>
#include <string>
#include <string_view>

#include "log/Logger.h"
#include "network/Endian.h"

using namespace rnet;
using namespace rnet::network;

static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in6),
              "InetAddress is same size as sockaddr_in6");
static_assert(offsetof(sockaddr_in, sin_family) == 0, "sin_family offset 0");
static_assert(offsetof(sockaddr_in6, sin6_family) == 0, "sin6_family offset 0");
static_assert(offsetof(sockaddr_in, sin_port) == 2, "sin_port offset 2");
static_assert(offsetof(sockaddr_in6, sin6_port) == 2, "sin6_port offset 2");

InetAddress::InetAddress(uint16_t portArg, bool loopbackOnly, bool ipv6) {
  static_assert(offsetof(InetAddress, addr6_) == 0, "addr6_ offset 0");
  static_assert(offsetof(InetAddress, addr_) == 0, "addr_ offset 0");
  if (ipv6) {
    MemZero(&addr6_, sizeof addr6_);
    addr6_.sin6_family = AF_INET6;
    in6_addr ip = loopbackOnly ? in6addr_loopback : in6addr_any;
    addr6_.sin6_addr = ip;
    addr6_.sin6_port = network::HostToNetwork16(portArg);
  } else {
    MemZero(&addr_, sizeof addr_);
    addr_.sin_family = AF_INET;
    in_addr_t ip = loopbackOnly ? kInaddrLoopback : kInaddrAny;
    addr_.sin_addr.s_addr = network::HostToNetwork32(ip);
    addr_.sin_port = network::HostToNetwork16(portArg);
  }
}

InetAddress::InetAddress(std::string_view ip, uint16_t portArg, bool ipv6) {
  if (ipv6 || strchr(ip.data(), ':')) {
    MemZero(&addr6_, sizeof addr6_);
    network::sockets::fromIpPort(ip.data(), portArg, &addr6_);
  } else {
    MemZero(&addr_, sizeof addr_);
    network::sockets::FromIpPort(ip.data(), portArg, &addr_);
  }
}

std::string InetAddress::ToIpPort() const {
  char buf[64] = "";
  network::sockets::ToIpPort(buf, sizeof buf, GetSockAddr());
  return buf;
}

std::string InetAddress::ToIp() const {
  char buf[64] = "";
  network::sockets::ToIp(buf, sizeof buf, GetSockAddr());
  return buf;
}

uint32_t InetAddress::Ipv4NetEndian() const {
  assert(Family() == AF_INET);
  return addr_.sin_addr.s_addr;
}

uint16_t InetAddress::port() const { return networkToHost16(portNetEndian()); }

// host name resolve buffer
static thread_local char tResolveBuffer[64 * 1024];

bool InetAddress::Resolve(std::string_view hostname, InetAddress* out) {
  assert(out != nullptr);
  struct hostent hent;
  struct hostent* he = nullptr;
  int herrno = 0;
  MemZero(&hent, sizeof(hent));

  int ret = gethostbyname_r(hostname.data(), &hent, tResolveBuffer,
                            sizeof tResolveBuffer, &he, &herrno);
  if (ret == 0 && he != nullptr) {
    assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
    out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
    return true;
  } else {
    if (ret) {
      LOG_SYSERR << "InetAddress::resolve";
    }
    return false;
  }
}

void InetAddress::SetScopeId(uint32_t scope_id) {
  if (family() == AF_INET6) {
    addr6_.sin6_scope_id = scope_id;
  }
}