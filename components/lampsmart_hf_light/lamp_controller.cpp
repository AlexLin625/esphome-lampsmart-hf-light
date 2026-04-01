#include "lamp_controller.h"

#include <algorithm>

#include "esp_log.h"

namespace esphome {
namespace lampsmarthf {

static const char *const TAG = "lampsmart_hf_ctl";

LampController::LampController() = default;

void LampController::set_address(const std::array<uint8_t, 4> &address) {
  this->device_.set_address(std::vector<uint8_t>(address.begin(), address.end()));
}

void LampController::set_group_id(uint8_t group_id) { this->device_.set_group_id(group_id); }

esp_err_t LampController::begin() { return BleAdvertiser::get_instance().init(); }

esp_err_t LampController::pair() {
  const std::vector<uint8_t> payload = this->device_.pair();
  ESP_LOGI(TAG, "Pair payload size=%u", static_cast<unsigned>(payload.size()));
  return this->advertise_for_("pair", payload, this->tx_duration_ms_);
}

esp_err_t LampController::turn_on() {
  const std::vector<uint8_t> payload = this->device_.turn_on();
  ESP_LOGI(TAG, "Turn on payload size=%u", static_cast<unsigned>(payload.size()));
  const esp_err_t ret = this->advertise_for_("turn_on", payload, this->tx_duration_ms_);
  if (ret == ESP_OK) {
    this->is_on_ = true;
  }
  return ret;
}

esp_err_t LampController::turn_off() {
  const std::vector<uint8_t> payload = this->device_.turn_off();
  ESP_LOGI(TAG, "Turn off payload size=%u", static_cast<unsigned>(payload.size()));
  const esp_err_t ret = this->advertise_for_("turn_off", payload, this->tx_duration_ms_);
  if (ret == ESP_OK) {
    this->is_on_ = false;
  }
  return ret;
}

esp_err_t LampController::set_brightness(uint16_t value) {
  const uint16_t clamped = std::min<uint16_t>(1000, value);
  const std::vector<uint8_t> payload = this->device_.set_brightness(clamped);
  ESP_LOGI(TAG, "Brightness payload size=%u value=%u", static_cast<unsigned>(payload.size()), clamped);
  return this->advertise_for_("brightness", payload, this->tx_duration_ms_);
}

esp_err_t LampController::set_temperature(uint16_t value) {
  const uint16_t clamped = std::min<uint16_t>(128, value);
  const std::vector<uint8_t> payload = this->device_.set_temperature(clamped);
  ESP_LOGI(TAG, "Temperature payload size=%u value=%u", static_cast<unsigned>(payload.size()), clamped);
  return this->advertise_for_("temperature", payload, this->tx_duration_ms_);
}

bool LampController::ble_ready() const { return BleAdvertiser::get_instance().is_ready(); }

bool LampController::is_advertising() const { return BleAdvertiser::get_instance().is_advertising(); }

void LampController::adv_stop_timer_callback_(void *arg) {
  auto *self = static_cast<LampController *>(arg);
  self->handle_adv_stop_();
}

void LampController::ensure_adv_stop_timer_() {
  if (this->adv_stop_timer_ != nullptr) {
    return;
  }

  const esp_timer_create_args_t timer_args = {
      .callback = &LampController::adv_stop_timer_callback_,
      .arg = this,
      .dispatch_method = ESP_TIMER_TASK,
      .name = "hf_adv_stop",
      .skip_unhandled_events = true,
  };

  const esp_err_t ret = esp_timer_create(&timer_args, &this->adv_stop_timer_);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create advertise stop timer: %d", ret);
    this->adv_stop_timer_ = nullptr;
  }
}

esp_err_t LampController::advertise_for_(const char *label, const std::vector<uint8_t> &payload, uint32_t duration_ms) {
  this->pending_advertisements_.push_back({label, payload, duration_ms});
  ESP_LOGI(TAG, "Queue advertisement label=%s duration=%u pending=%u", label, static_cast<unsigned>(duration_ms),
           static_cast<unsigned>(this->pending_advertisements_.size()));
  if (this->advertisement_active_) {
    return ESP_OK;
  }
  return this->start_next_advertisement_();
}

esp_err_t LampController::start_next_advertisement_() {
  this->ensure_adv_stop_timer_();
  if (this->adv_stop_timer_ == nullptr) {
    return ESP_FAIL;
  }

  if (this->pending_advertisements_.empty()) {
    this->advertisement_active_ = false;
    return ESP_OK;
  }

  const auto &pending = this->pending_advertisements_.front();
  ESP_LOGI(TAG, "Start advertisement label=%s duration=%u payload_size=%u", pending.label,
           static_cast<unsigned>(pending.duration_ms), static_cast<unsigned>(pending.payload.size()));
  BleAdvertiser::get_instance().start_advertising(pending.payload);
  this->advertisement_active_ = true;

  esp_timer_stop(this->adv_stop_timer_);
  const esp_err_t ret =
      esp_timer_start_once(this->adv_stop_timer_, static_cast<uint64_t>(pending.duration_ms) * 1000ULL);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start advertise stop timer: %d", ret);
    this->advertisement_active_ = false;
    return ret;
  }

  return ESP_OK;
}

void LampController::handle_adv_stop_() {
  BleAdvertiser::get_instance().stop_advertising();
  if (!this->pending_advertisements_.empty()) {
    ESP_LOGI(TAG, "Finish advertisement label=%s remaining=%u", this->pending_advertisements_.front().label,
             static_cast<unsigned>(this->pending_advertisements_.size() - 1));
    this->pending_advertisements_.pop_front();
  }
  this->advertisement_active_ = false;
  if (!this->pending_advertisements_.empty()) {
    this->start_next_advertisement_();
  }
}

void LampController::clear_pending_commands() {
  if (!this->pending_advertisements_.empty() || this->advertisement_active_) {
    ESP_LOGI(TAG, "Clear pending advertisements active=%s pending=%u", this->advertisement_active_ ? "true" : "false",
             static_cast<unsigned>(this->pending_advertisements_.size()));
  }
  this->pending_advertisements_.clear();
  this->advertisement_active_ = false;
  if (this->adv_stop_timer_ != nullptr) {
    esp_timer_stop(this->adv_stop_timer_);
  }
  BleAdvertiser::get_instance().stop_advertising();
}

}  // namespace lampsmarthf
}  // namespace esphome
