#pragma once

#include <cstdint>
#include <vector>

#include "hf_protocol.h"

namespace esphome {
namespace lampsmarthf {
namespace hf_protocol {

class HFDevice {
 public:
  HFDevice();

  void set_address(const std::vector<uint8_t> &address) { address_ = address; }
  void set_group_id(uint8_t id) { group_id_ = id; }
  uint8_t get_group_id() const { return group_id_; }
  void set_sequence_number(uint8_t seq) { current_seq_ = seq; }
  uint8_t get_sequence_number() const { return current_seq_; }

  std::vector<uint8_t> turn_on();
  std::vector<uint8_t> turn_off();
  std::vector<uint8_t> set_brightness(uint16_t value, NumericalOpType op = NONE);
  std::vector<uint8_t> set_temperature(uint16_t value, NumericalOpType op = NONE);
  std::vector<uint8_t> pair();

 protected:
  std::vector<uint8_t> address_;
  uint8_t group_id_{0x10};
  uint8_t current_seq_{1};
  HFPacketBuilder builder_;

  uint8_t next_seq_();
};

}  // namespace hf_protocol
}  // namespace lampsmarthf
}  // namespace esphome
