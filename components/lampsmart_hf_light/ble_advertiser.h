#pragma once

#include <cstdint>
#include <vector>

#include "esp_err.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"

namespace esphome {
namespace lampsmarthf {

class BleAdvertiser {
 public:
  static BleAdvertiser &get_instance();

  esp_err_t init();
  void start_advertising(const std::vector<uint8_t> &msd_data);
  esp_err_t stop_advertising();
  bool is_ready() const;
  bool is_advertising() const;

 protected:
  BleAdvertiser() = default;

  static void host_task_(void *param);
  static void on_reset_(int reason);
  static void on_sync_();
  static void advertise_();

  static std::vector<uint8_t> msd_data_;
  static uint8_t own_addr_type_;
  static bool advertising_;
  static bool synced_;
  static bool initialized_;
};

}  // namespace lampsmarthf
}  // namespace esphome
