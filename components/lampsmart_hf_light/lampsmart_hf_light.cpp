#include "lampsmart_hf_light.h"

#include <algorithm>
#include <cmath>

#include "esphome/core/log.h"

namespace esphome {
namespace lampsmarthf {

static const char *const TAG = "lampsmart_hf";

class InstantLightTransformer : public light::LightTransformer {
 public:
  optional<light::LightColorValues> apply() override { return this->target_values_; }
  bool is_finished() override { return true; }
};

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

std::unique_ptr<light::LightTransformer> LampSmartHFLight::create_default_transition() {
  return make_unique<InstantLightTransformer>();
}

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
  this->remote_values_as_cwww_(state, &cold_white, &warm_white);
  this->remote_values_as_brightness_(state, &brightness);
  const uint16_t target_brightness = this->brightness_from_state_(brightness);
  const uint16_t target_temperature = this->temperature_from_channels_(cold_white, warm_white);

  const bool is_on = state->remote_values.is_on();
  ESP_LOGD(TAG, "write_state: remote=%s current=%s cold=%0.3f warm=%0.3f brightness=%0.3f", ONOFF(state->remote_values.is_on()),
           ONOFF(state->current_values.is_on()), cold_white, warm_white, brightness);

  this->controller_.clear_pending_commands();

  if (!is_on) {
    ESP_LOGI(TAG, "write_state -> off");
    if (!this->has_known_state_ || this->is_on_) {
      this->controller_.turn_off();
    }
    this->has_known_state_ = true;
    this->is_on_ = false;
    this->last_brightness_ = 0;
    this->last_temperature_ = 0;
    return;
  }

  const bool was_off = !this->has_known_state_ || !this->is_on_;
  if (was_off) {
    ESP_LOGI(TAG, "write_state -> turn_on");
    this->controller_.turn_on();
  }

  if (was_off || target_brightness != this->last_brightness_) {
    ESP_LOGI(TAG, "write_state -> brightness %u (prev=%u)", static_cast<unsigned>(target_brightness),
             static_cast<unsigned>(this->last_brightness_));
    this->controller_.set_brightness(target_brightness);
    this->last_brightness_ = target_brightness;
  }

  if (was_off || target_temperature != this->last_temperature_) {
    ESP_LOGI(TAG, "write_state -> temperature %u (prev=%u)", static_cast<unsigned>(target_temperature),
             static_cast<unsigned>(this->last_temperature_));
    this->controller_.set_temperature(target_temperature);
    this->last_temperature_ = target_temperature;
  }

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

void LampSmartHFLight::pair() {
  ESP_LOGI(TAG, "Starting pairing");
  this->controller_.pair();
}

void LampSmartHFLight::remote_values_as_brightness_(light::LightState *state, float *brightness) const {
  state->remote_values.as_brightness(brightness);
  *brightness = state->gamma_correct_lut(*brightness);
}

void LampSmartHFLight::remote_values_as_cwww_(light::LightState *state, float *cold_white, float *warm_white) const {
  const auto &v = state->remote_values;
  if (!this->constant_brightness_) {
    v.as_cwww(cold_white, warm_white, false);
    *cold_white = state->gamma_correct_lut(*cold_white);
    *warm_white = state->gamma_correct_lut(*warm_white);
    return;
  }

  if (!(v.get_color_mode() & light::ColorCapability::COLD_WARM_WHITE)) {
    *cold_white = *warm_white = 0.0f;
    return;
  }

  const float cw_level = state->gamma_correct_lut(v.get_cold_white());
  const float ww_level = state->gamma_correct_lut(v.get_warm_white());
  const float white_level = state->gamma_correct_lut(v.get_state() * v.get_brightness());
  const float sum = cw_level > 0.0f || ww_level > 0.0f ? cw_level + ww_level : 1.0f;

  *cold_white = white_level * std::max(cw_level, ww_level) * cw_level / sum;
  *warm_white = white_level * std::max(cw_level, ww_level) * ww_level / sum;
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
  uint16_t protocol_temperature = static_cast<uint16_t>(std::lround(cold_ratio * 140.0f));
  if (this->reversed_) {
    protocol_temperature = static_cast<uint16_t>(140 - protocol_temperature);
  }
  return protocol_temperature;
}

}  // namespace lampsmarthf
}  // namespace esphome
