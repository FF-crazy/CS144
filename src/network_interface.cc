#include <iostream>

#include "arp_message.hh"
#include "exception.hh"
#include "network_interface.hh"

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( string_view name,
                                    shared_ptr<OutputPort> port,
                                    const EthernetAddress& ethernet_address,
                                    const Address& ip_address )
  : name_( name )
  , port_( notnull( "OutputPort", move( port ) ) )
  , ethernet_address_( ethernet_address )
  , ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address ) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but
//! may also be another host if directly connected to the same network as the destination) Note: the Address type
//! can be converted to a uint32_t (raw 32-bit IP address) by using the Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  EthernetFrame frame {{{}, ethernet_address_, EthernetHeader::TYPE_IPv4}, serialize(dgram)};
  auto next_mac = ip_to_eth_.find(next_hop.ipv4_numeric());
  if (next_mac != ip_to_eth_.end()) {
    // it turns out MAC in cache
    frame.header.dst = next_mac->second;
    transmit(frame);
  } else {
    pending_datagram_[next_hop.ipv4_numeric()].push_back(dgram);
    auto arp_sent = arp_request_time_.find(next_hop.ipv4_numeric());
    if (arp_sent == arp_request_time_.end() || current_time_ - arp_sent->second >= ARP_REQUEST_TIMEOUT) {
      auto arp = make_ARP_(ARPMessage::OPCODE_REQUEST, next_hop.ipv4_numeric(), {});
      EthernetFrame arp_frame {
        {ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(arp)
      };
      transmit(arp_frame);
      arp_request_time_[next_hop.ipv4_numeric()] = current_time_;
    }
  }
}

//! \param[in] frame the incoming Ethernet frame
void NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage arp;
    if (!parse(arp, frame.payload)) {
      return;
    }
    ip_to_eth_[arp.sender_ip_address] = arp.sender_ethernet_address;
    cache_ttl_.emplace(arp.sender_ip_address, current_time_);
    if (arp.opcode == ARPMessage::OPCODE_REQUEST && arp.target_ip_address == ip_address_.ipv4_numeric()) {
      EthernetFrame reply_frame {
        {arp.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(make_ARP_(ARPMessage::OPCODE_REPLY, arp.sender_ip_address, arp.sender_ethernet_address))
      };
      transmit(reply_frame);
    } else if (arp.opcode == ARPMessage::OPCODE_REPLY) {
      auto pending = pending_datagram_.find(arp.sender_ip_address);
      if (pending != pending_datagram_.end()) {
        for (const auto& dgram : pending->second) {
          EthernetFrame data_frame {
            {arp.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_IPv4},
            serialize(dgram)
          };
          transmit(data_frame);
        }
        pending_datagram_.erase(pending);
      }
    }

  } else if (frame.header.dst == ethernet_address_ && frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram;
    if (parse(dgram, frame.payload)) {
      datagrams_received_.push( dgram );
    }
  }

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  current_time_ += ms_since_last_tick;
  while (!cache_ttl_.empty() && current_time_ > cache_ttl_.front().second + CACHE_TTL_) {
    ip_to_eth_.erase(cache_ttl_.front().first);
    cache_ttl_.pop();
  }

}

ARPMessage NetworkInterface::make_ARP_(uint16_t opcode, uint32_t ip, EthernetAddress eth)
{
  ARPMessage arp;
  arp.opcode = opcode;
  arp.sender_ip_address = ip_address_.ipv4_numeric();
  arp.sender_ethernet_address = ethernet_address_;
  arp.target_ip_address = ip;

  if (opcode == ARPMessage::OPCODE_REQUEST) {
    arp.target_ethernet_address = {};
  } else {
    arp.target_ethernet_address = eth;
  }
  return arp;
}
