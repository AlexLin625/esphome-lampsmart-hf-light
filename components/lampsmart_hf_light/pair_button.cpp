#include "pair_button.h"

namespace esphome {
namespace lampsmarthf {

void LampSmartHFPairButton::press_action() { this->parent_->pair(); }

}  // namespace lampsmarthf
}  // namespace esphome
