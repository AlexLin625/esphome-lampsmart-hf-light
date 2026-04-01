#pragma once

#include <array>
#include <memory>

#include "esphome/components/light/light_output.h"
#include "esphome/core/component.h"
#include "esphome/core/entity_base.h"
#include "lamp_controller.h"

namespace esphome {
namespace lampsmarthf {

class LampSmartHFLight : public light::LightOutput, public Component, public EntityBase {
 public:
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;
  std::unique_ptr<light::LightTransformer> create_default_transition() override;

  void setup_state(light::LightState *state) override { this->light_state_ = state; }
  light::LightTraits get_traits() override;
  void write_state(light::LightState *state) override;

  void set_address(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);
  void set_group_id(uint8_t group_id);
  void set_tx_duration(uint32_t tx_duration);
  void set_cold_white_temperature(float cold_white_temperature) { cold_white_temperature_ = cold_white_temperature; }
  void set_warm_white_temperature(float warm_white_temperature) { warm_white_temperature_ = warm_white_temperature; }
  void set_constant_brightness(bool constant_brightness) { constant_brightness_ = constant_brightness; }
  void set_reversed(bool reversed) { reversed_ = reversed; }
  void set_min_brightness(uint16_t min_brightness) { min_brightness_ = min_brightness; }
  void pair();

 protected:
  void remote_values_as_brightness_(light::LightState *state, float *brightness) const;
  void remote_values_as_cwww_(light::LightState *state, float *cold_white, float *warm_white) const;
  uint16_t brightness_from_state_(float brightness) const;
  uint16_t temperature_from_channels_(float cold_white, float warm_white) const;

  LampController controller_;
  std::array<uint8_t, 4> address_{{0x00, 0x00, 0x00, 0x00}};
  uint8_t group_id_{0x10};
  uint32_t tx_duration_{600};
  float cold_white_temperature_{167.0f};
  float warm_white_temperature_{333.0f};
  bool constant_brightness_{false};
  bool reversed_{false};
  uint16_t min_brightness_{40};
  bool has_known_state_{false};
  bool is_on_{false};
  uint16_t last_brightness_{0};
  uint16_t last_temperature_{0};
  light::LightState *light_state_{nullptr};
};

}  // namespace lampsmarthf
}  // namespace esphome
