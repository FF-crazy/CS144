#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  uint64_t res = 0;
  for (auto& item : outstanding_) {
    res += item.sequence_length();
  }
  return res;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return repeat_time_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  if (input_.has_error()) {
    transmit(make_empty_message());
    return;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {isn_, false, "", false, input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST) {
    input_.set_error();
    return;
  }
  RTO_ms_ = initial_RTO_ms_; //reset RTO
  window_size_ = msg.window_size;
  repeat_time_ = 0;
  while (!outstanding_.empty()) {
    if (outstanding_.front().seqno == msg.ackno) {
      break;
    }
    outstanding_.pop_front();
  }

}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if (input_.has_error()) {
    return;
  }
  current_time_ms_ += ms_since_last_tick;
  if (current_time_ms_ >= RTO_ms_ && !outstanding_.empty()) {
    transmit(outstanding_.front());
    if (window_size_ > 0) {
      repeat_time_++;
    }
    RTO_ms_ = current_time_ms_ + (initial_RTO_ms_ << repeat_time_);
  }
}
