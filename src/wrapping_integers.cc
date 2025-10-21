#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  auto absolute
    = static_cast<uint64_t>( this->raw_value_ - zero_point.raw_value_ ) + ( checkpoint & 0xFFFFFFFF00000000 );
  if ( absolute > checkpoint && absolute - checkpoint > ( 1UL << 31 ) ) {
    if ( absolute >= ( 1UL << 32 ) ) {
      return absolute - ( 1UL << 32 );
    }
  }
  if ( checkpoint > absolute && checkpoint - absolute >= ( 1UL << 31 ) ) {
    return absolute + ( 1UL << 32 );
  }
  return absolute;
}
