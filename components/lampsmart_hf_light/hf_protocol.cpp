#include "hf_protocol.h"

#include <algorithm>
#include <cstring>

namespace esphome {
namespace lampsmarthf {
namespace hf_protocol {

bool HFPacketBuilder::sbox_initialized_ = false;
uint8_t HFPacketBuilder::sbox_[16];

const uint8_t HFPacketBuilder::MASTER_KEY_PARTS[16] = {
    0x52, 0xEA, 0x73, 0xFF, 0x49, 0x60, 0xBF, 0x56,
    0x42, 0x05, 0x07, 0xE8, 0xD3, 0xA7, 0xB9, 0x9D,
};

HFPacketBuilder::HFPacketBuilder() {
  if (!sbox_initialized_) {
    this->init_sbox_();
    sbox_initialized_ = true;
  }
}

void HFPacketBuilder::decrypt_tea_8bytes_(const uint8_t *in, uint8_t *out) {
  const auto *key_bytes = reinterpret_cast<const uint8_t *>(TEA_KEY_STRING);

  uint32_t v0 = (in[0]) | (in[1] << 8) | (in[2] << 16) | (in[3] << 24);
  uint32_t v1 = (in[4]) | (in[5] << 8) | (in[6] << 16) | (in[7] << 24);

  uint32_t k0 = (key_bytes[0]) | (key_bytes[1] << 8) | (key_bytes[2] << 16) | (key_bytes[3] << 24);
  uint32_t k1 = (key_bytes[4]) | (key_bytes[5] << 8) | (key_bytes[6] << 16) | (key_bytes[7] << 24);
  uint32_t k2 = (key_bytes[8]) | (key_bytes[9] << 8) | (key_bytes[10] << 16) | (key_bytes[11] << 24);
  uint32_t k3 = (key_bytes[12]) | (key_bytes[13] << 8) | (key_bytes[14] << 16) | (key_bytes[15] << 24);

  uint32_t sum = 0xC6EF3720;
  const uint32_t delta = 0x9E3779B9;

  for (int i = 0; i < 32; i++) {
    v1 -= ((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3);
    v0 -= ((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1);
    sum -= delta;
  }

  out[0] = static_cast<uint8_t>(v0 & 0xFF);
  out[1] = static_cast<uint8_t>((v0 >> 8) & 0xFF);
  out[2] = static_cast<uint8_t>((v0 >> 16) & 0xFF);
  out[3] = static_cast<uint8_t>((v0 >> 24) & 0xFF);
  out[4] = static_cast<uint8_t>(v1 & 0xFF);
  out[5] = static_cast<uint8_t>((v1 >> 8) & 0xFF);
  out[6] = static_cast<uint8_t>((v1 >> 16) & 0xFF);
  out[7] = static_cast<uint8_t>((v1 >> 24) & 0xFF);
}

void HFPacketBuilder::init_sbox_() {
  uint8_t buffer[8];

  std::memcpy(buffer, MASTER_KEY_PARTS, 8);
  this->decrypt_tea_8bytes_(buffer, sbox_);

  std::memcpy(buffer, MASTER_KEY_PARTS + 8, 8);
  this->decrypt_tea_8bytes_(buffer, sbox_ + 8);
}

void HFPacketBuilder::obfuscate_payload_(uint8_t *data, int offset, int length, uint8_t loop_key, uint8_t modifier_key) {
  const uint8_t sbox_idx = ((modifier_key >> 4) & 0x0F) ^ (modifier_key & 0x0F);
  const uint8_t key_modifier = sbox_[sbox_idx];

  for (int i = 0; i < length; i++) {
    const int pos = offset + i;
    const uint8_t sbox_val = sbox_[(i + loop_key) & 0x0F];
    data[pos] = static_cast<uint8_t>((data[pos] ^ key_modifier) + sbox_val);
  }
}

uint16_t HFPacketBuilder::calculate_crc16_(const uint8_t *data, int length, uint16_t initial) {
  uint16_t crc = initial;
  for (int i = 0; i < length; i++) {
    crc ^= static_cast<uint16_t>(data[i] << 8);
    for (int bit = 0; bit < 8; bit++) {
      const bool bit15 = (crc & 0x8000U) != 0;
      crc = static_cast<uint16_t>(crc << 1);
      if (bit15) {
        crc ^= 0x1021U;
      }
    }
  }
  return crc;
}

std::vector<uint8_t> HFPacketBuilder::build_packet(uint8_t seq_id, const std::vector<uint8_t> &address, uint8_t cmd,
                                                   uint8_t seq_num, const PayloadParams &payload) {
  if (address.size() < 4) {
    return {};
  }

  uint8_t raw[16];
  const uint8_t rnd = static_cast<uint8_t>(seq_num + 1);

  raw[0] = 0xFF;
  raw[1] = rnd;
  raw[2] = seq_num;
  raw[3] = address[0];
  raw[4] = address[1];
  raw[5] = address[2];
  raw[6] = address[3];
  raw[7] = cmd;
  raw[8] = 0xFF;
  raw[9] = 0xFF;
  raw[10] = seq_num;
  raw[11] = payload.p0;
  raw[12] = payload.p1;
  raw[13] = payload.p2;
  raw[14] = 0;
  raw[15] = 0;

  if (cmd != 0xB4) {
    this->obfuscate_payload_(raw, 9, 5, seq_id, raw[10]);
  }

  const uint16_t crc = this->calculate_crc16_(raw, 13, 0);
  raw[14] = static_cast<uint8_t>(crc & 0xFF);
  raw[15] = static_cast<uint8_t>((crc >> 8) & 0xFF);

  this->obfuscate_payload_(raw, 0, 16, 0x56, raw[1]);

  std::vector<uint8_t> packet(26);
  packet[0] = 0x48;
  packet[1] = 0x46;
  packet[2] = 0x4B;
  packet[3] = 0x4A;

  std::memcpy(packet.data() + 4, raw, sizeof(raw));

  packet[20] = 0xFF;
  packet[21] = 0xFF;
  packet[22] = 0xFF;
  packet[23] = 0xFF;
  packet[24] = 0xFF;
  packet[25] = 0xFF;

  return packet;
}

PayloadParams pack_single_value(uint8_t val) { return {val, 0, 0}; }

PayloadParams pack_brightness(int val) {
  val = std::clamp(val, 40, 1000);
  return {
      0x00,
      static_cast<uint8_t>(val / 256),
      static_cast<uint8_t>(val % 256),
  };
}

PayloadParams pack_temperature(int val) {
  val = std::clamp(val, 0, 140);
  return {
      0x00,
      0,
      static_cast<uint8_t>(val),
  };
}

}  // namespace hf_protocol
}  // namespace lampsmarthf
}  // namespace esphome
