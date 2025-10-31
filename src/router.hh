#pragma once

#include <memory>
#include <optional>

#include "exception.hh"
#include "network_interface.hh"

// \brief A router that has multiple network interfaces and
// performs longest-prefix-match routing between them.

class TrieNode
{
public:
  std::optional<uint8_t> end_value_ {};
  std::array<std::shared_ptr<TrieNode>, 2> next_ {};

  TrieNode() = default;
};

class Trie
{
private:
  std::shared_ptr<TrieNode> root_;

public:
  Trie() : root_( std::make_shared<TrieNode>() ) {}

  void insert_ip( uint32_t ip, uint8_t prefix, uint8_t end_value )
  {
    auto pointer = root_;
    for ( uint8_t i = 0; i < prefix; i++ ) {
      uint8_t bit = ( ip >> ( 31 - i ) ) & 0x1;
      if ( !pointer->next_[bit] ) {
        pointer->next_[bit] = std::make_shared<TrieNode>();
      }
      pointer = pointer->next_[bit];
    }
    pointer->end_value_ = end_value;
  }

  std::optional<uint8_t> search_longest_prefix( uint32_t ip )
  {
    std::optional<uint8_t> result = root_->end_value_;
    auto pointer = root_;

    for ( int i = 31; i >= 0; i-- ) {
      uint8_t bit = ( ip >> i ) & 0x1;
      if ( !pointer->next_[bit] ) {
        break;
      }
      pointer = pointer->next_[bit];
      if ( pointer->end_value_.has_value() ) {
        result = pointer->end_value_; // 更新最长匹配
      }
    }
    return result;
  }
};

class Router
{
public:
  // Add an interface to the router
  // \param[in] interface an already-constructed network interface
  // \returns The index of the interface after it has been added to the router
  size_t add_interface( std::shared_ptr<NetworkInterface> interface )
  {
    _interfaces.push_back( notnull( "add_interface", std::move( interface ) ) );
    return _interfaces.size() - 1;
  }

  // Access an interface by index
  std::shared_ptr<NetworkInterface> interface( const size_t N ) { return _interfaces.at( N ); }

  // Add a route (a forwarding rule)
  void add_route( uint32_t route_prefix,
                  uint8_t prefix_length,
                  std::optional<Address> next_hop,
                  size_t interface_num );

  // Route packets between the interfaces
  void route();

private:
  // The router's collection of network interfaces
  std::vector<std::shared_ptr<NetworkInterface>> _interfaces {};
  Trie trie_ {};
  std::vector<std::pair<std::optional<Address>, size_t>> route_list_ {};
};
