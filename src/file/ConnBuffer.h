#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "network/Endian.h"
namespace rnet::file {
class Buffer {
  public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize  = 1024;

    explicit Buffer( size_t initialSize = kInitialSize ) : buffer_( kCheapPrepend + initialSize ), readerIndex_( kCheapPrepend ), writerIndex_( kCheapPrepend ) {
        assert( ReadableBytes() == 0 );
        assert( WritableBytes() == initialSize );
        assert( PrependableBytes() == kCheapPrepend );
    }

    // implicit copy-ctor, move-ctor, dtor and assignment are fine
    // NOTE: implicit move-ctor is added in g++ 4.6

    void Swap( Buffer& rhs ) {
        buffer_.swap( rhs.buffer_ );
        std::swap( readerIndex_, rhs.readerIndex_ );
        std::swap( writerIndex_, rhs.writerIndex_ );
    }

    size_t ReadableBytes() const {
        return writerIndex_ - readerIndex_;
    }

    size_t WritableBytes() const {
        return buffer_.size() - writerIndex_;
    }

    size_t PrependableBytes() const {
        return readerIndex_;
    }

    const char* Peek() const {
        return Begin() + readerIndex_;
    }

    const char* FindCrlf() const {
        // FIXME: replace with memmem()?
        const char* crlf = std::search( Peek(), BeginWrite(), kCRLF, kCRLF + 2 );
        return crlf == BeginWrite() ? nullptr : crlf;
    }

    const char* FindCrlf( const char* start ) const {
        assert( Peek() <= start );
        assert( start <= BeginWrite() );
        // FIXME: replace with memmem()?
        const char* crlf = std::search( start, BeginWrite(), kCRLF, kCRLF + 2 );
        return crlf == BeginWrite() ? nullptr : crlf;
    }

    const char* FindEol() const {
        const void* eol = memchr( Peek(), '\n', ReadableBytes() );
        return static_cast< const char* >( eol );
    }

    const char* FindEol( const char* start ) const {
        assert( Peek() <= start );
        assert( start <= BeginWrite() );
        const void* eol = memchr( start, '\n', BeginWrite() - start );
        return static_cast< const char* >( eol );
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void Retrieve( size_t len ) {
        assert( len <= ReadableBytes() );
        if ( len < ReadableBytes() ) {
            readerIndex_ += len;
        }
        else {
            RetrieveAll();
        }
    }

    void RetrieveUntil( const char* end ) {
        assert( Peek() <= end );
        assert( end <= BeginWrite() );
        Retrieve( end - Peek() );
    }

    void RetrieveInt64() {
        Retrieve( sizeof( int64_t ) );
    }

    void RetrieveInt32() {
        Retrieve( sizeof( int32_t ) );
    }

    void RetrieveInt16() {
        Retrieve( sizeof( int16_t ) );
    }

    void RetrieveInt8() {
        Retrieve( sizeof( int8_t ) );
    }

    void RetrieveAll() {
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }

    std::string RetrieveAllAsString() {
        return RetrieveAsString( ReadableBytes() );
    }

    std::string RetrieveAsString( size_t len ) {
        assert( len <= ReadableBytes() );
        std::string result( Peek(), len );
        Retrieve( len );
        return result;
    }

    std::string_view ToStringView() const {
        return std::string_view( Peek(), static_cast< int >( ReadableBytes() ) );
    }

    void Append( const std::string_view& str ) {
        Append( str.data(), str.size() );
    }

    void Append( const char* /*restrict*/ data, size_t len ) {
        EnsureWritableBytes( len );
        std::copy( data, data + len, BeginWrite() );
        HasWritten( len );
    }

    void Append( const void* /*restrict*/ data, size_t len ) {
        Append( static_cast< const char* >( data ), len );
    }

    void EnsureWritableBytes( size_t len ) {
        if ( WritableBytes() < len ) {
            MakeSpace( len );
        }
        assert( WritableBytes() >= len );
    }

    char* BeginWrite() {
        return Begin() + writerIndex_;
    }

    const char* BeginWrite() const {
        return Begin() + writerIndex_;
    }

    void HasWritten( size_t len ) {
        assert( len <= WritableBytes() );
        writerIndex_ += len;
    }

    void Unwrite( size_t len ) {
        assert( len <= ReadableBytes() );
        writerIndex_ -= len;
    }

    ///
    /// Append int64_t using network endian
    ///
    void AppendInt64( int64_t x ) {
        int64_t be64 = network::HostToNetwork64( x );
        Append( &be64, sizeof be64 );
    }

    ///
    /// Append int32_t using network endian
    ///
    void AppendInt32( int32_t x ) {
        int32_t be32 = network::HostToNetwork32( x );
        Append( &be32, sizeof be32 );
    }

    void AppendInt16( int16_t x ) {
        int16_t be16 = network::HostToNetwork16( x );
        Append( &be16, sizeof be16 );
    }

    void AppendInt8( int8_t x ) {
        Append( &x, sizeof x );
    }

    ///
    /// Read int64_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int64_t ReadInt64() {
        int64_t result = PeekInt64();
        RetrieveInt64();
        return result;
    }

    ///
    /// Read int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int32_t ReadInt32() {
        int32_t result = PeekInt32();
        RetrieveInt32();
        return result;
    }

    int16_t ReadInt16() {
        int16_t result = PeekInt16();
        RetrieveInt16();
        return result;
    }

    int8_t ReadInt8() {
        int8_t result = PeekInt8();
        RetrieveInt8();
        return result;
    }

    ///
    /// Peek int64_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int64_t)
    int64_t PeekInt64() const {
        assert( ReadableBytes() >= sizeof( int64_t ) );
        int64_t be64 = 0;
        ::memcpy( &be64, Peek(), sizeof be64 );
        return network::NetworkToHost64( be64 );
    }

    ///
    /// Peek int32_t from network endian
    ///
    /// Require: buf->readableBytes() >= sizeof(int32_t)
    int32_t PeekInt32() const {
        assert( ReadableBytes() >= sizeof( int32_t ) );
        int32_t be32 = 0;
        ::memcpy( &be32, Peek(), sizeof be32 );
        return network::NetworkToHost32( be32 );
    }

    int16_t PeekInt16() const {
        assert( ReadableBytes() >= sizeof( int16_t ) );
        int16_t be16 = 0;
        ::memcpy( &be16, Peek(), sizeof be16 );
        return network::NetworkToHost16( be16 );
    }

    int8_t PeekInt8() const {
        assert( ReadableBytes() >= sizeof( int8_t ) );
        int8_t x = *Peek();
        return x;
    }

    ///
    /// Prepend int64_t using network endian
    ///
    void PrependInt64( int64_t x ) {
        int64_t be64 = network::HostToNetwork64( x );
        Prepend( &be64, sizeof be64 );
    }

    ///
    /// Prepend int32_t using network endian
    ///
    void PrependInt32( int32_t x ) {
        int32_t be32 = network::HostToNetwork32( x );
        Prepend( &be32, sizeof be32 );
    }

    void PrependInt16( int16_t x ) {
        int16_t be16 = network::HostToNetwork16( x );
        Prepend( &be16, sizeof be16 );
    }

    void PrependInt8( int8_t x ) {
        Prepend( &x, sizeof x );
    }

    void Prepend( const void* /*restrict*/ data, size_t len ) {
        assert( len <= PrependableBytes() );
        readerIndex_ -= len;
        const char* d = static_cast< const char* >( data );
        std::copy( d, d + len, Begin() + readerIndex_ );
    }

    void Shrink( size_t reserve ) {
        // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
        Buffer other;
        other.EnsureWritableBytes( ReadableBytes() + reserve );
        other.Append( ToStringView() );
        Swap( other );
    }

    size_t InternalCapacity() const {
        return buffer_.capacity();
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t ReadFd( int fd, int* savedErrno );

  private:
    char* Begin() {
        return &*buffer_.begin();
    }

    const char* Begin() const {
        return &*buffer_.begin();
    }

    void MakeSpace( size_t len ) {
        if ( WritableBytes() + PrependableBytes() < len + kCheapPrepend ) {
            // FIXME: move readable data
            buffer_.resize( writerIndex_ + len );
        }
        else {
            // move readable data to the front, make space inside buffer
            assert( kCheapPrepend < readerIndex_ );
            size_t readable = ReadableBytes();
            std::copy( Begin() + readerIndex_, Begin() + writerIndex_, Begin() + kCheapPrepend );
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
            assert( readable == ReadableBytes() );
        }
    }

  private:
    std::vector< char > buffer_;
    size_t              readerIndex_;
    size_t              writerIndex_;

    static const char kCRLF[];
};
}  // namespace rnet::file