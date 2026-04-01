#include "lampsmart_hf_light.h"

#include <algorithm>
#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace lampsmarthf {

static const char *const TAG = "lampsmart_hf";

void LampSmartHFLight::setup() {
  this->controller_.set_address(this->address_);
  this->controller_.set_group_id(this->group_id_);
  this->controller_.set_tx_duration(this->tx_duration_);

  const esp_err_t ret = this->controller_.begin();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize BLE advertiser: %d", ret);
    this->mark_failed();
    return;
  }

  if (this->light_state_ != nullptr) {
    this->register_service(&LampSmartHFLight::on_pair, "pair_" + this->light_state_->get_object_id());
  }
}

void LampSmartHFLight::dump_config() {
  ESP_LOGCONFIG(TAG, "LampSmart HF Light");
  ESP_LOGCONFIG(TAG, "  Address: %02X:%02X:%02X:%02X", this->address_[0], this->address_[1], this->address_[2],
                this->address_[3]);
  ESP_LOGCONFIG(TAG, "  Group: 0x%02X", this->group_id_);
  ESP_LOGCONFIG(TAG, "  Advertising Duration: %u ms", static_cast<unsigned>(this->tx_duration_));
  ESP_LOGCONFIG(TAG, "  Constant Brightness: %s", YESNO(this->constant_brightness_));
  ESP_LOGCONFIG(TAG, "  Reversed Temperature: %s", YESNO(this->reversed_));
  ESP_LOGCONFIG(TAG, "  Minimum Brightness: %u", static_cast<unsigned>(this->min_brightness_));
  ESP_LOGCONFIG(TAG, "  BLE Ready: %s", YESNO(this->controller_.ble_ready()));
}

float LampSmartHFLight::get_setup_priority() const { return setup_priority::HARDWARE; }

light::LightTraits LampSmartHFLight::get_traits() {
  auto traits = light::LightTraits();
  traits.set_supported_color_modes({light::ColorMode::COLD_WARM_WHITE});
  traits.set_min_mireds(this->cold_white_temperature_);
  traits.set_max_mireds(this->warm_white_temperature_);
  return traits;
}

void LampSmartHFLight::write_state(light::LightState *state) {
  float cold_white;
  float warm_white;
  float brightness;
  state->current_values_as_cwww(&cold_white, &warm_white, this->constant_brightness_);
  state->current_values_as_brightness(&brightness);

  const bool is_on = state->current_values.is_on();
  ESP_LOGD(TAG, "write_state: remote=%s current=%s cold=%0.3f warm=%0.3f", ONOFF(state->remote_values.is_on()),
           ONOFF(is_on), cold_white, warm_white);

  if (!is_on) {
    if (!this->has_known_state_ || this->is_on_) {
      this->controller_.turn_off();
    }
    this->has_known_state_ = true;
    this->is_on_ = false;
    return;
  }

  if (!this->has_known_state_ || !this->is_on_) {
    this->controller_.turn_on();
  }

  this->controller_.set_brightness(this->brightness_from_state_(brightness));
  this->controller_.set_temperature(this->temperature_from_channels_(cold_white, warm_white));

  this->has_known_state_ = true;
  this->is_on_ = true;
}

void LampSmartHFLight::set_address(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
  this->address_ = {b0, b1, b2, b3};
  this->controller_.set_address(this->address_);
}

void LampSmartHFLight::set_group_id(uint8_t group_id) {
  this->group_id_ = group_id;
  this->controller_.set_group_id(group_id);
}

void LampSmartHFLight::set_tx_duration(uint32_t tx_duration) {
  this->tx_duration_ = tx_duration;
  this->controller_.set_tx_duration(tx_duration);
}

void LampSmartHFLight::on_pair() {
  ESP_LOGI(TAG, "Pair service called");
  this->controller_.pair();
}

uint16_t LampSmartHFLight::brightness_from_state_(float brightness) const {
  brightness = std::clamp(brightness, 0.0f, 1.0f);
  uint16_t scaled = static_cast<uint16_t>(std::lround(brightness * 1000.0f));
  if (scaled > 0 && scaled < this->min_brightness_) {
    scaled = this->min_brightness_;
  }
  return scaled;
}

uint16_t LampSmartHFLight::temperature_from_channels_(float cold_white, float warm_white) const {
  const float total = cold_white + warm_white;
  float cold_ratio = total > 0.0f ? cold_white / total : 0.5f;
  cold_ratio = std::clamp(cold_ratio, 0.0f, 1.0f);
  if (this->reversed_) {
    cold_ratio = 1.0f - cold_ratio;
  }
  return static_cast<uint16_t>(std::lround(1.0f + cold_ratio * 127.0f));
}

}  // namespace lampsmarthf
}  // namespace esphome
