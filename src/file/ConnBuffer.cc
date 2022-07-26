#include "file/ConnBuffer.h"

#include "base/Common.h"
#include "network/SocketOps.h"

namespace rnet::file {
const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::ReadFd( int fd, int* savedErrno ) {
  // saved an ioctl()/FIONREAD call to tell how much to read
  char         extrabuf[ 65536 ];
  struct iovec vec[ 2 ];
  const size_t writable = WritableBytes();
  vec[ 0 ].iov_base     = Begin() + writerIndex_;
  vec[ 0 ].iov_len      = writable;
  vec[ 1 ].iov_base     = extrabuf;
  vec[ 1 ].iov_len      = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int     iovcnt = ( writable < sizeof extrabuf ) ? 2 : 1;
  const ssize_t n      = network::sockets::Readv( fd, vec, iovcnt );
  if ( n < 0 ) {
    *savedErrno = errno;
  }
  else if ( implicit_cast< size_t >( n ) <= writable ) {
    writerIndex_ += n;
  }
  else {
    writerIndex_ = buffer_.size();
    Append( extrabuf, n - writable );
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}
}  // namespace rnet::file