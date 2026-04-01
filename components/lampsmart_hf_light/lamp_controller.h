#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <vector>

#include "ble_advertiser.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "hf_device.h"

namespace esphome {
namespace lampsmarthf {

class LampController {
 public:
  LampController();

  void set_address(const std::array<uint8_t, 4> &address);
  void set_group_id(uint8_t group_id);
  void set_tx_duration(uint32_t tx_duration_ms) { tx_duration_ms_ = tx_duration_ms; }

  esp_err_t begin();
  esp_err_t pair();
  esp_err_t turn_on();
  esp_err_t turn_off();
  esp_err_t set_brightness(uint16_t value);
  esp_err_t set_temperature(uint16_t value);
  void clear_pending_commands();

  bool is_on() const { return is_on_; }
  bool ble_ready() const;
  bool is_advertising() const;

 protected:
  struct PendingAdvertisement {
    const char *label;
    std::vector<uint8_t> payload;
    uint32_t duration_ms;
  };

  static void adv_stop_timer_callback_(void *arg);
  void ensure_adv_stop_timer_();
  esp_err_t advertise_for_(const char *label, const std::vector<uint8_t> &payload, uint32_t duration_ms);
  esp_err_t start_next_advertisement_();
  void handle_adv_stop_();

  hf_protocol::HFDevice device_;
  bool is_on_{false};
  uint32_t tx_duration_ms_{600};
  esp_timer_handle_t adv_stop_timer_{nullptr};
  bool advertisement_active_{false};
  std::deque<PendingAdvertisement> pending_advertisements_;
};

}  // namespace lampsmarthf
}  // namespace esphome
