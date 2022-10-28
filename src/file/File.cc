#include "file/File.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

#include "unix/Thread.h"

using namespace rnet;
using namespace rnet::file;

AppendFile::AppendFile( std::string_view filename )
  : fp_( ::fopen( filename.data(), "ae" ) ),  // 'e' for O_CLOEXEC
    writtenBytes_( 0 ) {
  assert( fp_ );
  ::setbuffer( fp_, buffer_, sizeof buffer_ );
  // posix_fadvise POSIX_FADV_DONTNEED ?
}

AppendFile::~AppendFile() {
  ::fclose( fp_ );
}

void AppendFile::Append( const char* logline, const size_t len ) {
  size_t written = 0;

  while ( written != len ) {
    size_t remain = len - written;
    size_t n      = Write( logline + written, remain );
    if ( n != remain ) {
      int err = ferror( fp_ );
      if ( err ) {
        fprintf( stderr, "AppendFile::append() failed %s\n", thread::GetErrnoMessage( err ) );
        break;
      }
    }
    written += n;
  }

  writtenBytes_ += written;
}

void AppendFile::Flush() {
  ::fflush( fp_ );
}

size_t AppendFile::Write( const char* logline, size_t len ) {
  // #undef fwrite_unlocked
  return ::fwrite_unlocked( logline, 1, len, fp_ );
}

FileReader::FileReader( std::string_view filename ) : fd_( ::open( std::string{ filename }.c_str(), O_RDONLY | O_CLOEXEC ) ), err_( 0 ) {
  buf_[ 0 ] = '\0';
  if ( fd_ < 0 ) {
    err_ = errno;
  }
}

FileReader::~FileReader() {
  if ( fd_ >= 0 ) {
    ::close( fd_ );  // FIXME: check EINTR
  }
}

std::string FileReader::GetFileContent( std::string_view file_name ) {
  namespace fs                  = std::filesystem;
  constexpr size_t kReadBufSize = 1024 * 5;
  fs::path         path{ file_name };

  // open file
  std::ifstream f;
  f.open( path );
  if ( !f.is_open() ) {
    throw std::exception{};
  }

  size_t      fileSize = fs::file_size( path );
  std::string content( fileSize, '0' );
  size_t      cur     = 0;
  size_t      readNum = ( fileSize - cur ) / kReadBufSize;
  while ( readNum > 0 ) {
    f.read( content.data() + cur, readNum );
  }

  return content;
}
// return errno
template < typename String > int FileReader::ReadToString( int maxSize, String* content, int64_t* fileSize, int64_t* modifyTime, int64_t* createTime ) {
  static_assert( sizeof( off_t ) == 8, "_FILE_OFFSET_BITS = 64" );  // NOLINT
  assert( content != nullptr );
  int err = err_;
  if ( fd_ >= 0 ) {
    content->clear();

    if ( fileSize ) {
      struct stat statbuf;
      if ( fstat( fd_, &statbuf ) == 0 ) {
        if ( S_ISREG( statbuf.st_mode ) ) {
          *fileSize = statbuf.st_size;
          content->reserve( static_cast< int >( std::min( implicit_cast< int64_t >( maxSize ), *fileSize ) ) );
        }
        else if ( S_ISDIR( statbuf.st_mode ) ) {
          err = EISDIR;
        }
        if ( modifyTime ) {
          *modifyTime = statbuf.st_mtime;
        }
        if ( createTime ) {
          *createTime = statbuf.st_ctime;
        }
      }
      else {
        err = errno;
      }
    }

    while ( content->size() < implicit_cast< size_t >( maxSize ) ) {
      size_t  toRead = std::min( implicit_cast< size_t >( maxSize ) - content->size(), sizeof( buf_ ) );
      ssize_t n      = ::read( fd_, buf_, toRead );
      if ( n > 0 ) {
        content->append( buf_, n );
      }
      else {
        if ( n < 0 ) {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

int FileReader::ReadToBuffer( int* size ) {
  int err = err_;
  if ( fd_ >= 0 ) {
    ssize_t n = ::pread( fd_, buf_, sizeof( buf_ ) - 1, 0 );
    if ( n >= 0 ) {
      if ( size ) {
        *size = static_cast< int >( n );
      }
      buf_[ n ] = '\0';
    }
    else {
      err = errno;
    }
  }
  return err;
}

template int file::ReadFile( std::string_view filename, int maxSize, std::string* content, int64_t*, int64_t*, int64_t* );

template int FileReader::ReadToString( int maxSize, std::string* content, int64_t*, int64_t*, int64_t* );
