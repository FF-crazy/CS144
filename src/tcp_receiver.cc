#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (message.RST || reader().is_finished()) {
    reassembler_.reader().set_error();
    return;
  }
  uint64_t index = 0;
  if (message.SYN) {
    isn_ = message.seqno + 1;
  } else {
    if (!isn_.has_value() || message.seqno + 1 == isn_) {
      return;
    }
    auto checkpoint = writer().bytes_pushed();
    index = message.seqno.unwrap(isn_.value(), checkpoint);
  }
  reassembler_.insert(index, message.payload, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  optional<Wrap32> ackno = {};
  if (isn_.has_value()) {
    ackno = Wrap32::wrap(writer().bytes_pushed(), isn_.value()) + writer().is_closed();
  }
  uint16_t window_size = writer().available_capacity() < UINT16_MAX ? writer().available_capacity() : UINT16_MAX;
  return {ackno, window_size, reader().has_error()};
}
