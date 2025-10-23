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
  uint32_t effective_window = max(window_size_, 1U);
  uint64_t flight_num = sequence_numbers_in_flight();
  bool sent_new_data = false;
  while (flight_num < effective_window) {
    if (is_FIN_) {
      break;
    }
    uint64_t available = effective_window - flight_num;
    auto msg = make_message_(available);
    if (!msg.sequence_length()) {
      break;
    }
    transmit(msg);
    outstanding_.push_back(msg);
    sent_new_data = true;
    flight_num = sequence_numbers_in_flight();
  }
  if (sent_new_data && outstanding_.size() == 1) {
    current_time_ms_ = 0;
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {seqno_, false, "", false, input_.has_error()};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if (msg.RST) {
    input_.set_error();
    return;
  }
  window_size_ = msg.window_size;
  if (!msg.ackno.has_value()) {
    return;
  }

  if (seqno_.unwrap(isn_, 0) < msg.ackno->unwrap(isn_, 0)) {
    return;
  }
  bool acked_new_data = false;
  while (!outstanding_.empty()) {
    auto& front = outstanding_.front();
    Wrap32 end_seqno = front.seqno + front.sequence_length();
    if ( end_seqno.unwrap( isn_, 0 ) <= msg.ackno->unwrap( isn_, 0 ) ) {
      outstanding_.pop_front();
      acked_new_data = true;
    } else {
      break;
    }
  }
  if (acked_new_data) {
    RTO_ms_ = initial_RTO_ms_;
    repeat_time_ = 0;
    current_time_ms_ = 0;
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
      RTO_ms_ *= 2;
    }
    current_time_ms_ = 0;
  }

}
TCPSenderMessage TCPSender::make_message_( uint64_t size )
{
  TCPSenderMessage msg;
  msg.seqno = seqno_;
  msg.RST = input_.has_error();
  if (!is_SYN_) {
    msg.SYN = true;
    is_SYN_ = true;
    size = (size > 0) ? size - 1 : 0;
  }

  if (size > 0 && input_.reader().bytes_buffered() > 0) {
    auto space = min(min(size, TCPConfig::MAX_PAYLOAD_SIZE), input_.reader().bytes_buffered());
    read(input_.reader(), space, msg.payload);
    size -= space;
  }
  if (input_.reader().is_finished() && size > 0) {
    is_FIN_ = true;
    msg.FIN = true;
  }
  seqno_ = seqno_ + msg.sequence_length();
  return msg;
}
