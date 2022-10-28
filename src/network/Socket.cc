#include "network/Socket.h"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <cstdio>

#include "log/Logger.h"
#include "network/NetAddress.h"
#include "network/SocketOps.h"

using namespace rnet;
using namespace rnet::network;

rnet::network::Socket::~Socket() {
  if (sockfd_ != nonValidSocketFd) {
    LOG_ERROR << "close socket:" << sockfd_ << "\n";
    Sockets::close(sockfd);
  }
}

bool rnet::network::Socket::GetTcpInfo(struct tcp_info* tcpi) const {
  socklen_t len = sizeof(*tcpi);
  MemZero(tcpi, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool rnet::network::Socket::GetTcpInfoString(char* buf, int len) const {
  struct tcp_info tcpi;
  bool ok = GetTcpInfo(&tcpi);
  if (ok) {
    snprintf(
        buf, len,
        "unrecovered=%u "
        "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
        "lost=%u retrans=%u rtt=%u rttvar=%u "
        "sshthresh=%u cwnd=%u total_retrans=%u",
        tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
        tcpi.tcpi_rto,          // Retransmit timeout in usec
        tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
        tcpi.tcpi_snd_mss, tcpi.tcpi_rcv_mss,
        tcpi.tcpi_lost,     // Lost packets
        tcpi.tcpi_retrans,  // Retransmitted packets out
        tcpi.tcpi_rtt,      // Smoothed round trip time in usec
        tcpi.tcpi_rttvar,   // Medium deviation
        tcpi.tcpi_snd_ssthresh, tcpi.tcpi_snd_cwnd,
        tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
  }
  return ok;
}

void rnet::network::Socket::BindAddress(const InetAddress& addr) {
  Sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void rnet::network::Socket::Listen() { Sockets::listenOrDie(sockfd); }

int rnet::network::Socket::Accept(InetAddress* peeraddr) {
  struct sockaddr_in6 addr;
  MemZero(&addr, sizeof addr);
  int connfd = Sockets::accept(sockfd_, &addr);
  if (connfd >= 0) {
    peeraddr->setSockAddrInet6(addr);
  }
  return connfd;
}

void Socket::shutdownWrite() { Sockets::shutdownWrite(sockfd_); }

void Socket::setTcpNoDelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval,
               static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReuseAddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
               static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::setReusePort(bool on) {
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval,
                         static_cast<socklen_t>(sizeof optval));
  if (ret < 0 && on) {
    LOG_SYSERR << "SO_REUSEPORT failed.";
  }
#else
  if (on) {
    LOG_ERROR << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::setKeepAlive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval,
               static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}
