#pragma once
#include <functional>
#include <memory>

#include "file/ConnBuffer.h"
#include "unix/Time.h"

namespace rnet::File {
class Buffer;
}  // namespace rnet::File
namespace rnet::Network {

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
    const TcpConnectionPtr &, File::Buffer *, rnet::Unix::Timestamp)>;

using MessageCallback = std::function<void(const TcpConnectionPtr &,
                                           File::Buffer *, Unix::Timestamp)>;

void defaultConnectionCallback(const TcpConnectionPtr &conn);
void defaultMessageCallback(const TcpConnectionPtr &conn, File::Buffer *buffer,
                            Unix::Timestamp receiveTime);

}  // namespace rnet::Network