#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if ( output_.writer().is_closed() || output_.writer().available_capacity() == 0 ) {
    return;
  }
  push_cache_( first_index, data, is_last_substring );
  push_helper_();
}

void Reassembler::push_helper_()
{
  if ( !ready_to_send_.contains( buffer_point_ ) ) {
    return;
  }
  auto& str = ready_to_send_[buffer_point_];
  auto size = str.first.size();
  output_.writer().push( str.first );
  if ( ready_to_send_[buffer_point_].second ) {
    output_.writer().close();
  }
  ready_to_send_.erase( buffer_point_ );
  buffer_point_ += size;
  push_helper_();
}

void Reassembler::merge_segment_()
{
  if ( ready_to_send_.empty() ) {
    return;
  }
  auto item = ready_to_send_.begin();
  while ( true ) {
    auto next = std::next( item );
    if ( next == ready_to_send_.end() ) {
      break;
    }
    auto item_end = item->first + item->second.first.size();
    if ( item_end > next->first ) {
      auto next_end = next->first + next->second.first.size();
      if ( next_end > item_end ) {
        item->second.first += next->second.first.substr( item_end - next->first );
      }
      item->second.second = next->second.second;
      ready_to_send_.erase( next );
    } else {
      ++item;
    }
  }
}
void Reassembler::push_cache_( uint64_t& first_index, string& data, bool& is_last_substring )
{
  // 丢弃已经处理过的重叠部分
  if ( first_index < buffer_point_ ) {
    if ( first_index + data.size() <= buffer_point_ ) {
      return; // 完全重叠，直接返回
    }
    data = data.substr( buffer_point_ - first_index );
    first_index = buffer_point_;
  }

  // 截断超出容量的部分
  uint64_t max_index = buffer_point_ + output_.writer().available_capacity();
  if ( first_index + data.size() > max_index ) {
    data = data.substr( 0, max_index - first_index );
    is_last_substring = false;
  }
  // 处理碰撞
  if ( ready_to_send_.contains( first_index ) && ready_to_send_[first_index].first.size() > data.size() ) {
    return;
  } else {
    ready_to_send_[first_index] = make_pair( data, is_last_substring );
  }
  merge_segment_();
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t res = 0;
  for ( auto& item : ready_to_send_ ) {
    res += item.second.first.size();
  }
  return res;
}
