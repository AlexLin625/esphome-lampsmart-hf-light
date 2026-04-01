#include "hf_device.h"

namespace esphome {
namespace lampsmarthf {
namespace hf_protocol {

HFDevice::HFDevice() : address_({0x00, 0x00, 0x00, 0x00}) {}

uint8_t HFDevice::next_seq_() { return current_seq_++; }

std::vector<uint8_t> HFDevice::turn_on() {
  return builder_.build_packet(group_id_, address_, 0xB3, next_seq_(), pack_single_value(0));
}

std::vector<uint8_t> HFDevice::turn_off() {
  return builder_.build_packet(group_id_, address_, 0xB2, next_seq_(), pack_single_value(0));
}

std::vector<uint8_t> HFDevice::set_brightness(uint16_t value, NumericalOpType op) {
  return builder_.build_packet(group_id_, address_, 0xB5, next_seq_(), pack_brightness(value, op));
}

std::vector<uint8_t> HFDevice::set_temperature(uint16_t value, NumericalOpType op) {
  return builder_.build_packet(group_id_, address_, 0xB7, next_seq_(), pack_temperature(value, op));
}

std::vector<uint8_t> HFDevice::pair() {
  const PayloadParams pair_payload = {0xAA, 0x66, 0x55};
  return builder_.build_packet(0xAA, address_, 0xB4, next_seq_(), pair_payload);
}

}  // namespace hf_protocol
}  // namespace lampsmarthf
}  // namespace esphome
