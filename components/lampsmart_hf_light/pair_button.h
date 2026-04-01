#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "lampsmart_hf_light.h"

namespace esphome {
namespace lampsmarthf {

class LampSmartHFPairButton : public button::Button, public Parented<LampSmartHFLight> {
 public:
  LampSmartHFPairButton() = default;

 protected:
  void press_action() override;
};

}  // namespace lampsmarthf
}  // namespace esphome
