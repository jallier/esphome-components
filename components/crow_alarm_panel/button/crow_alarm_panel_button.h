#pragma once

#include "esphome/components/button/button.h"
#include "esphome/core/component.h"
#include "esphome/core/log.h"
#include "../crow_alarm_panel.h"

namespace esphome {
namespace crow_alarm_panel {

class CrowAlarmPanelButton : public button::Button, public Component {
 public:
  void set_parent(CrowAlarmPanel *parent) { this->parent_ = parent; }
  void set_button_type(const std::string &type) { this->button_type_ = type; }
  void set_code(const std::string &code) { this->code_ = code; }

 protected:
  void press_action() override {
    if (this->button_type_ == "arm_away") {
      if (this->parent_->is_arm_in_progress()) {
        ESP_LOGW("crow_alarm_panel.button", "Arm operation already in progress, ignoring button press");
        return;
      }
      this->parent_->arm_away();
    } else if (this->button_type_ == "arm_stay") {
      if (this->parent_->is_arm_in_progress()) {
        ESP_LOGW("crow_alarm_panel.button", "Arm operation already in progress, ignoring button press");
        return;
      }
      this->parent_->arm_stay();
    } else if (this->button_type_ == "disarm") {
      if (!this->parent_->is_armed()) {
        ESP_LOGW("crow_alarm_panel.button", "Cannot disarm - alarm is not armed");
        return;
      }
      if (this->parent_->is_disarm_in_progress()) {
        ESP_LOGW("crow_alarm_panel.button", "Disarm already in progress, ignoring button press");
        return;
      }
      this->parent_->disarm(this->code_);
    }
  }

  CrowAlarmPanel *parent_;
  std::string button_type_;
  std::string code_;
};

}  // namespace crow_alarm_panel
}  // namespace esphome
