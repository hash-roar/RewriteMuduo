#pragma once
#include <functional>
#include <memory>

#include "file/ConnBuffer.h"
#include "unix/Time.h"

namespace rnet::file {
class Buffer;
}  // namespace rnet::File
namespace rnet::network {

class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using TimerCallback = std::function<void()>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback =
    std::function<void(const TcpConnectionPtr &, size_t)>;

// the data has been read to (buf, len)
using MessageCallback = std::function<void(
    const TcpConnectionPtr &, file::Buffer *, rnet::Unix::Timestamp)>;

using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                           file::Buffer *, Unix::Timestamp)>;

void DefaultConnectionCallback(const TcpConnectionPtr &conn);
void DefaultMessageCallback(const TcpConnectionPtr &conn, file::Buffer *buffer,
                            Unix::Timestamp receiveTime);

}  // namespace rnet::Network