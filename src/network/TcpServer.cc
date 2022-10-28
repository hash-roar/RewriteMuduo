#include "network/TcpServer.h"

#include <cstdint>

#include "log/Logger.h"
#include "network/Acceptor.h"
#include "network/EventLoop.h"
#include "network/LoopThreadPool.h"
#include "network/TcpConnection.h"

using namespace rnet;
namespace rnet::network {

// 新连接创建:
// tcpServer 将newConnection 回调函数注册给acceptor,
// acceptor
// 在获得channel的读事件时,尝试系统调用accept获取文件描述符并调用callback
// 在callback 里完成连接对象的创建和初始化
TcpServer::TcpServer( EventLoop* loop, const InetAddress& listenAddr, const std::string& nameArg, Option option )
  : loop_( CHECK_NOTNULL( loop ) ), ipPort_( listenAddr.ToIpPort() ), name_( nameArg ), acceptor_( new Acceptor( loop, listenAddr, option == kReusePort ) ),
    threadPool_( new EventLoopThreadPool( loop, name_ ) ), connectionCallback_( defaultConnectionCallback ), messageCallback_( defaultMessageCallback ), nextConnId_( 1 ) {
  acceptor_->setNewConnectionCallback( std::bind( &TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2 ) );
}

// server 关闭时关闭所有存在的连接.
TcpServer::~TcpServer() {
  loop_->AssertInLoopThread();
  LOG_TRACE << "TcpServer::~TcpServer [" << name << "] destructing";

  for ( auto& item : connections_ ) {
    TcpConnectionPtr conn( item.second );
    // 直接释放智能指针,而不用移动语义然后在容器中擦除,效率更高
    // 此时全局只有一个智能指针隐式绑定在bind生成的对象内,一旦仿函数执行完毕,对象销毁,智能指针计数变为0,tcp
    // connection销毁
    item.second.reset();
    conn->getLoop()->runInLoop( std::bind( &TcpConnection::connectDestroyed, conn ) );
  }
}
void TcpServer::SetThreadNum( int numThreads ) {
  assert( 0 <= numThreads );
  threadPool_->setThreadNum( numThreads );
}

void TcpServer::Start() {
  int32_t dummy = 0;
  if ( started_.compare_exchange_strong( dummy, 1 ) ) {
    threadPool_->start( threadInitCallback_ );

    assert( !acceptor_->listening() );
    loop_->runInLoop( std::bind( &Acceptor::listen, acceptor_.get() ) );
  }
}

// 新连接创建核心方法
// 首先挑选一个io loop
// 构建连接,并在io loop中完成最终初始化(注册epoll事件等)
void TcpServer::NewConnection( int sockfd, const InetAddress& peerAddr ) {
  loop_->AssertInLoopThread();
  EventLoop* ioLoop = threadPool_->getNextLoop();
  char       buf[ 64 ];
  snprintf( buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_ );
  ++nextConnId_;
  std::string connName = name_ + buf;

  LOG_INFO << "TcpServer::newConnection [" << name_ << "] - new connection [" << connName << "] from " << peerAddr.ToIpPort();
  InetAddress LocalAddr( Sockets::getLocalAddr( sockfd ) );
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn( new TcpConnection( ioLoop, connName, sockfd, localAddr, peerAddr ) );
  connections_[ connName ] = conn;
  conn->setConnectionCallback( connectionCallback_ );
  conn->setMessageCallback( messageCallback_ );
  conn->setWriteCompleteCallback( writeCompleteCallback_ );
  conn->setCloseCallback( std::bind( &TcpServer::removeConnection, this,
                                     std::placeholders::_1 ) );  // FIXME: unsafe
  ioLoop->runInLoop( std::bind( &TcpConnection::connectEstablished, conn ) );
}

// 这是tcp connection调用的函数,用于主动tcp connection的主动关闭连接
void TcpServer::RemoveConnection( const TcpConnectionPtr& conn ) {
  // FIXME: unsafe
  loop_->runInLoop( std::bind( &TcpServer::removeConnectionInLoop, this, conn ) );
}

// 在io线程调用connectDestroyed,完成析构最后一步
void TcpServer::RemoveConnectionInLoop( const TcpConnectionPtr& conn ) {
  loop_->AssertInLoopThread();
  LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name();
  size_t n = connections_.erase( conn->name() );
  assert( n == 1 );
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop( std::bind( &TcpConnection::connectDestroyed, conn ) );
}

}  // namespace rnet::network