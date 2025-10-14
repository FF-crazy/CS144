#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity )
{
  stream_.reserve( capacity );
}

void Writer::push( string data )
{
  if ( error_ || is_closed_ || available_capacity() == 0 || data.empty() ) {
    return;
  }
  const uint64_t pushed_size = min( data.size(), available_capacity() );
  pushed_ += pushed_size;
  stream_.append( data, 0, pushed_size );
}

void Writer::close()
{
  this->is_closed_ = true;
}

bool Writer::is_closed() const
{
  return this->is_closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - pushed_ + popped_;
}

uint64_t Writer::bytes_pushed() const
{
  return this->pushed_;
}

string_view Reader::peek() const
{
  return string_view( stream_ );
}

void Reader::pop( uint64_t len )
{
  if ( len == 0 || bytes_buffered() == 0 ) {
    return;
  }
  const uint64_t popped_size = min( bytes_buffered(), len );
  popped_ += popped_size;
  stream_.erase( 0, popped_size );
}

bool Reader::is_finished() const
{
  return is_closed_ && bytes_buffered() == 0;
}

uint64_t Reader::bytes_buffered() const
{
  return pushed_ - popped_;
}

uint64_t Reader::bytes_popped() const
{
  return this->popped_;
}