#include "crow_alarm_panel.h"
#include "esphome/core/log.h"

namespace esphome {
namespace crow_alarm_panel {

static const char *TAG_ACP = "crow_alarm_panel.acp";

void CrowAlarmControlPanel::dump_config() { ESP_LOGCONFIG(TAG_ACP, "Crow Alarm Control Panel"); }

void CrowAlarmControlPanel::control(const alarm_control_panel::AlarmControlPanelCall &call) {
  if (this->parent_ == nullptr) {
    ESP_LOGE(TAG_ACP, "Parent not set, ignoring control call");
    return;
  }

  const auto target_state = call.get_state();
  if (!target_state.has_value()) {
    ESP_LOGW(TAG_ACP, "No target state in control call");
    return;
  }

  switch (*target_state) {
    case alarm_control_panel::ACP_STATE_ARMED_AWAY:
      if (this->requires_code_to_arm_ && !call.get_code().has_value() && this->code_.empty()) {
        this->status_momentary_warning("Code required to arm", 2000);
        return;
      }
      this->parent_->arm_away();
      this->publish_state(alarm_control_panel::ACP_STATE_ARMING);
      break;
    case alarm_control_panel::ACP_STATE_ARMED_HOME:
      if (this->requires_code_to_arm_ && !call.get_code().has_value() && this->code_.empty()) {
        this->status_momentary_warning("Code required to arm", 2000);
        return;
      }
      this->parent_->arm_stay();
      this->publish_state(alarm_control_panel::ACP_STATE_ARMING);
      break;
    case alarm_control_panel::ACP_STATE_DISARMED: {
      std::string code = this->code_;
      if (call.get_code().has_value()) {
        code = call.get_code().value();
      }
      if (code.empty()) {
        this->status_momentary_warning("Code required to disarm", 2000);
        return;
      }
      this->parent_->disarm(code);
      this->publish_state(alarm_control_panel::ACP_STATE_DISARMING);
      break;
    }
    case alarm_control_panel::ACP_STATE_TRIGGERED:
    case alarm_control_panel::ACP_STATE_PENDING:
      this->publish_state(*target_state);
      break;
    default:
      ESP_LOGW(TAG_ACP, "Unsupported alarm control panel request");
      break;
  }
}
}  // namespace crow_alarm_panel
}  // namespace esphome
