#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return zero_point + n;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  auto absolute = static_cast<uint64_t>(this->raw_value_ - zero_point.raw_value_);
  absolute += checkpoint & 0xFFFFFFFF00000000;
  auto flag = (1UL << 32);
  if (absolute < flag && checkpoint < absolute) {
    return absolute;
  }
  if (checkpoint < absolute) {
    return checkpoint + flag - absolute < absolute - checkpoint ? absolute - flag : absolute;
  } else {
    return checkpoint - absolute < absolute + flag - checkpoint ? absolute : absolute + flag;
  }
}
