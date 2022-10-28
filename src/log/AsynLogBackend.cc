#include "log/AsynLogBackend.h"

#include <cassert>
#include <chrono>
#include <mutex>
#include <string>

#include "log/LogFile.h"
#include "unix/Time.h"

using namespace rnet;
using namespace rnet::log;

AsyncLogging::AsyncLogging( const std::string& basename, off_t rollSize, int flushInterval )
  : flushInterval( flushInterval ), running_( false ), basename( basename ), rollSize( rollSize ), thread_( std::bind( &::rnet::log::AsyncLogging::ThreadFunction, this ), "Logging" ), latch_( 1 ),
    mutex_(), cond_(), currentBuffer_( new Buffer ), nextBuffer_( new Buffer ), buffers_() {
  currentBuffer_->Bzero();
  nextBuffer_->Bzero();
  buffers_.reserve( 16 );
}

void AsyncLogging::Append( const char* logline, int len ) {
  //   muduo::MutexLockGuard lock(mutex_);
  std::lock_guard lock( mutex_ );
  if ( currentBuffer_->Avail() > len ) {
    currentBuffer_->Append( logline, len );
  }
  else {
    buffers_.push_back( std::move( currentBuffer_ ) );

    if ( nextBuffer_ ) {
      currentBuffer_ = std::move( nextBuffer_ );
    }
    else {
      currentBuffer_.reset( new Buffer );  // Rarely happens
    }
    currentBuffer_->Append( logline, len );
    cond_.notify_one();
  }
}

void AsyncLogging::ThreadFunction() {
  assert( running_ == true );
  latch_.CountDown();
  LogFile   output( basename, rollSize, false );
  BufferPtr newBuffer1( new Buffer );
  BufferPtr newBuffer2( new Buffer );
  newBuffer1->Bzero();
  newBuffer2->Bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve( 16 );
  while ( running_ ) {
    assert( newBuffer1 && newBuffer1->Length() == 0 );
    assert( newBuffer2 && newBuffer2->Length() == 0 );
    assert( buffersToWrite.empty() );

    {
      std::unique_lock lock( mutex_ );
      // if (buffers_.empty())  // unusual usage!
      // {
      //   cond_.waitForSeconds(flushInterval_);
      // }
      cond_.wait_for( lock, std::chrono::seconds{ flushInterval }, [ this ] { return !buffers_.empty(); } );
      buffers_.push_back( std::move( currentBuffer_ ) );
      currentBuffer_ = std::move( newBuffer1 );
      buffersToWrite.swap( buffers_ );
      if ( !nextBuffer_ ) {
        nextBuffer_ = std::move( newBuffer2 );
      }
    }

    assert( !buffersToWrite.empty() );

    if ( buffersToWrite.size() > 25 ) {
      char buf[ 256 ];
      snprintf( buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n", Unix::Timestamp::Now().ToFormattedString().c_str(), buffersToWrite.size() - 2 );
      fputs( buf, stderr );
      output.Append( buf, static_cast< int >( strlen( buf ) ) );
      buffersToWrite.erase( buffersToWrite.begin() + 2, buffersToWrite.end() );
    }

    for ( const auto& buffer : buffersToWrite ) {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.Append( buffer->Data(), buffer->Length() );
    }

    if ( buffersToWrite.size() > 2 ) {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize( 2 );
    }

    if ( !newBuffer1 ) {
      assert( !buffersToWrite.empty() );
      newBuffer1 = std::move( buffersToWrite.back() );
      buffersToWrite.pop_back();
      newBuffer1->Reset();
    }

    if ( !newBuffer2 ) {
      assert( !buffersToWrite.empty() );
      newBuffer2 = std::move( buffersToWrite.back() );
      buffersToWrite.pop_back();
      newBuffer2->Reset();
    }

    buffersToWrite.clear();
    output.Flush();
  }
  output.Flush();
}
