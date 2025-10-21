#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }
  if ( writer().is_closed() ) {
    return;
  }
  uint64_t index = 0;
  if ( message.SYN ) {
    isn_ = message.seqno;
  }
  if ( !isn_.has_value() ) {
    return;
  }
  auto checkpoint = writer().bytes_pushed();
  index = message.seqno.unwrap( isn_.value(), checkpoint );
  if ( !message.SYN && index == 0 ) {
    return;
  }
  reassembler_.insert( index - 1 + message.SYN, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  optional<Wrap32> ackno = {};
  if ( isn_.has_value() ) {
    ackno = Wrap32::wrap( writer().bytes_pushed() + writer().is_closed() + 1, isn_.value() );
  }
  uint16_t window_size = writer().available_capacity() < UINT16_MAX ? writer().available_capacity() : UINT16_MAX;
  return { ackno, window_size, reader().has_error() };
}
