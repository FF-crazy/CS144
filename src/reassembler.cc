#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring ) {
    if (output_.writer().is_closed() || output_.writer().available_capacity() == 0) {
        return;
    }
    // 丢弃已经处理过的重叠部分
    if (first_index < buffer_point_) {
        if (first_index + data.size() <= buffer_point_) {
            return;  // 完全重叠，直接返回
        }
        data = data.substr(buffer_point_ - first_index);
        first_index = buffer_point_;
    }

    // 截断超出容量的部分
    uint64_t max_index = buffer_point_ + output_.writer().available_capacity();
    if (first_index + data.size() > max_index) {
        data = data.substr(0, max_index - first_index);
        is_last_substring = false;
    }
    // 截断已经存在在缓存里的
    if (!ready_to_send_.empty() && first_index + data.size() > ready_to_send_.begin()->first) {
        data = data.substr(0, ready_to_send_.begin()->first - first_index);
    }

    if (first_index == buffer_point_) {
        output_.writer().push(data);
        buffer_point_ += data.size();
        push_helper();
    } else {
            ready_to_send_[first_index] = make_pair(data, is_last_substring);
    }
    if (is_last_substring && first_index + data.size() == buffer_point_) {
        output_.writer().close();
    }
}

void Reassembler::push_helper() {
    if (!ready_to_send_.contains(buffer_point_)) {
        return;
    }
    auto& str = ready_to_send_[buffer_point_];
    auto size = str.first.size();
    output_.writer().push(str.first);
    if (ready_to_send_[buffer_point_].second) {
        output_.writer().close();
    }
    ready_to_send_.erase(buffer_point_);
    buffer_point_ += size;
    push_helper();
}

uint64_t Reassembler::bytes_pending() const
{
  uint64_t res = 0;
  for (auto& item : ready_to_send_) {
      res += item.second.first.size();
  }
  return res;
}
