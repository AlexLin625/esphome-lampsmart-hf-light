#pragma once

#include <cstdint>
#include <vector>

namespace esphome {
namespace lampsmarthf {
namespace hf_protocol {

struct PayloadParams {
  uint8_t p0;
  uint8_t p1;
  uint8_t p2;
};

enum NumericalOpType {
  NONE = 0x00,
  NUM_INCREASE = 0x01,
  NUM_DECREASE = 0x02,
};

class HFPacketBuilder {
 public:
  HFPacketBuilder();

  std::vector<uint8_t> build_packet(uint8_t seq_id, const std::vector<uint8_t> &address, uint8_t cmd, uint8_t seq_num,
                                    const PayloadParams &payload);

 protected:
  static bool sbox_initialized_;
  static uint8_t sbox_[16];
  static const uint8_t MASTER_KEY_PARTS[16];
  static constexpr const char *TEA_KEY_STRING = "!hIflIngCypcal@#";

  void init_sbox_();
  void decrypt_tea_8bytes_(const uint8_t *in, uint8_t *out);
  void obfuscate_payload_(uint8_t *data, int offset, int length, uint8_t loop_key, uint8_t modifier_key);
  uint16_t calculate_crc16_(const uint8_t *data, int length, uint16_t initial);
};

PayloadParams pack_single_value(uint8_t val);
PayloadParams pack_brightness(int val, NumericalOpType op = NUM_INCREASE);
PayloadParams pack_temperature(int val, NumericalOpType op = NUM_INCREASE);

}  // namespace hf_protocol
}  // namespace lampsmarthf
}  // namespace esphome
