
#pragma once

#include <arpa/inet.h>

namespace rnet::network::sockets {
///
/// Creates a non-blocking socket file descriptor,
/// abort if any error.
int CreateNonblockingOrDie(sa_family_t family);

int Connect(int sockfd, const struct sockaddr* addr);
void BindOrDie(int sockfd, const struct sockaddr* addr);
void ListenOrDie(int sockfd);

// 获取非阻塞套接字
int Accept(int sockfd, struct sockaddr_in6* addr);
ssize_t Read(int sockfd, void* buf, size_t count);
ssize_t Readv(int sockfd, const struct iovec* iov, int iovcnt);
ssize_t Write(int sockfd, const void* buf, size_t count);
void Close(int sockfd);
void ShutdownWrite(int sockfd);

void ToIpPort(char* buf, size_t size, const struct sockaddr* addr);
void ToIp(char* buf, size_t size, const struct sockaddr* addr);

void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in* addr);
void FromIpPort(const char* ip, uint16_t port, struct sockaddr_in6* addr);

int GetSocketError(int sockfd);

const struct sockaddr* SockaddrCast(const struct sockaddr_in* addr);
const struct sockaddr* SockaddrCast(const struct sockaddr_in6* addr);
struct sockaddr* SockaddrCast(struct sockaddr_in6* addr);
const struct sockaddr_in* SockaddrInCast(const struct sockaddr* addr);
const struct sockaddr_in6* SockaddrIn6Cast(const struct sockaddr* addr);

struct sockaddr_in6 GetLocalAddr(int sockfd);
struct sockaddr_in6 GetPeerAddr(int sockfd);
bool IsSelfConnect(int sockfd);

} // namespace rnet::Network::Sockets