#pragma once

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/alarm_control_panel/alarm_control_panel.h"
#include "esphome/components/alarm_control_panel/alarm_control_panel_state.h"

#include <bitset>
#include <vector>

namespace esphome {
namespace crow_alarm_panel {

static const uint8_t UNKNOWN = 0x10;
static const uint8_t UNKNOWN2 = 0x20;

static const uint8_t ARMED_STATE = 0x11;
static const uint8_t ZONE_STATE = 0x12;
static const uint8_t KEYPAD_COMMAND = 0x14;  // Beeps etc
static const uint8_t KEYPAD_STATE = 0x15;

static const uint8_t SETTING_VALUE = 0x16;   // for locking LEDs
static const uint8_t SETTING_VALUE2 = 0x17;  // for flashing LEDs < 255
static const uint8_t SETTING_VALUE3 = 0x18;  // for flashing LEDs > 255

static const uint8_t MEMORY_EVENT = 0x20;

static const uint8_t OUTPUT_STATE = 0x50;
static const uint8_t CURRENT_TIME = 0x54;
static const uint8_t CURRENT_TEMP = 0x23; // Unconfirmed, this is just my suspicion based on observed data
static const uint8_t BOUNDARY = 0x7E;
// static const uint8_t KEYPRESS = 0xD1; // This is from upstream, but doesn't get sent by arrowhead panels that I can see
static const uint8_t KEYPRESS = 0xA1;
static const uint8_t MEMORY_CLEAR = 0xD2;
static const uint8_t PACKET_COMPLETE_MARKER = 0xFE; // This gets added after the boundary for keypad packets - signaling end of packet?

// Keys 0 - 9 are 0-9
static const uint8_t KEY_MEMORY = 11;
static const uint8_t KEY_BYPASS = 15;
static const uint8_t KEY_PROGRAM = 16;
static const uint8_t KEY_ENTER = 17;
static const uint8_t KEY_STAY = 0x0E; // Untested
static const uint8_t KEY_ARM = 0x0D;

static const uint8_t BUFFER_LENGTH = 20;

static const char *KEYS[18] = {"0", "1",     "2",      "3",       "4",   "5",    "6",      "7",       "8",
                               "9", "PANIC", "MEMORY", "CONTROL", "ARM", "STAY", "BYPASS", "PROGRAM", "ENTER"};

static const uint8_t RESPONSE_TIME = 0x19;

static const char *DAYS[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

struct CrowAlarmPanelMessage {
  uint8_t type;
  uint8_t data_buffer[BUFFER_LENGTH];
  uint8_t data_len;
};

struct CrowAlarmPanelKeypad {
  std::string name;
  uint8_t address;
};

// static const CrowAlarmPanelKeypad UNKNOWN_KEYPAD{
//   .address = 0xFF,
//   .name = "Unknown"
// };

class CrowAlarmPanelStore {
 public:
  void setup(InternalGPIOPin *clock_pin, InternalGPIOPin *data_pin);
  static void interrupt(CrowAlarmPanelStore *arg);

  uint8_t buffer[BUFFER_LENGTH];
  uint8_t buffer2[BUFFER_LENGTH];
  uint8_t data_length{0};

  bool data{false};
  uint32_t last_clock_time_{0};             // Track last clock edge
  uint32_t last_transmission_time_{0};      // Track last TX completion
  uint32_t prev_falling_edge_time_us_{0};   // Track last falling edge for glitch filtering

  bool inside_{false};

 protected:
  ISRInternalGPIOPin clock_pin_;
  ISRInternalGPIOPin data_pin_;
  uint8_t num_bits_{0};
  uint8_t boundary_buffer_{0};

 public:
  static const uint32_t BUS_IDLE_TIMEOUT_US = 5000;          // 5ms idle = bus free
  static const uint32_t MIN_TX_INTERVAL_MS = 50;             // Min 50ms between TX
  /**
   * Minimum interval between falling edges to avoid glitches. The clock runs at ~1.2kHz,
   * so we expect ~833μs between edges. Setting this to 700μs filters out most glitches while
   * still allowing normal operation.
   */
  static const uint32_t MIN_FALLING_EDGE_INTERVAL_US = 700;  // Min 700μs between falling edges
  static const uint32_t TX_START_TIMEOUT_US = 100000;        // 100ms to start TX
  static const uint32_t TX_BIT_TIMEOUT_US = 10000;           // 10ms between bits
};

struct CrowAlarmPanelZone {
  binary_sensor::BinarySensor *motion_binary_sensor;
  binary_sensor::BinarySensor *bypass_binary_sensor;
  uint8_t zone;
};

struct CrowAlarmPanelOutput {
  switch_::Switch *the_switch;
  uint8_t number;
};

class CrowAlarmPanel : public Component {
 public:
  void setup() override;
  void dump_config() override;
  void loop() override;

  void set_output(uint8_t output, bool state);

  void set_clock_pin(InternalGPIOPin *clock) { this->clock_pin_ = clock; }
  void set_data_pin(InternalGPIOPin *data) { this->data_pin_ = data; }
  void set_keypad_address(uint8_t address) { this->keypad_address_ = address; }
  void add_keypad(const std::string &name, uint8_t address) {
    this->keypads_.push_back(std::move(CrowAlarmPanelKeypad{
        .name = name,
        .address = address,
    }));
  }

  void register_zone(binary_sensor::BinarySensor *binary_sensor, uint8_t zone) {
    for (auto &z : this->zones_) {
      if (z.zone == zone) {
        z.motion_binary_sensor = binary_sensor;
        return;
      }
    }
    this->zones_.push_back(CrowAlarmPanelZone{
        .motion_binary_sensor = binary_sensor,
        .bypass_binary_sensor = nullptr,
        .zone = zone,
    });
  }
  void register_zone_bypass(binary_sensor::BinarySensor *binary_sensor, uint8_t zone) {
    for (auto &z : this->zones_) {
      if (z.zone == zone) {
        z.bypass_binary_sensor = binary_sensor;
        return;
      }
    }
    this->zones_.push_back(CrowAlarmPanelZone{
        .motion_binary_sensor = nullptr,
        .bypass_binary_sensor = binary_sensor,
        .zone = zone,
    });
  }

  void register_armed_state(text_sensor::TextSensor *armed_state_sensor) { this->armed_state_ = armed_state_sensor; }
  void register_output_switch(switch_::Switch *output_switch, uint8_t output_number) {
    this->outputs_.push_back(std::move(CrowAlarmPanelOutput{
        .the_switch = output_switch,
        .number = output_number,
    }));
  }
  void register_alarm_control_panel(alarm_control_panel::AlarmControlPanel *acp) { this->alarm_control_panel_ = acp; }

  void arm_away();
  void arm_stay();
  void disarm(const std::string &code);
  bool is_disarm_in_progress() const { return this->disarm_in_progress_; }
  bool is_arm_in_progress() const { return this->arm_in_progress_; }
  bool is_armed() const;

  Trigger<uint8_t, std::vector<uint8_t>> *get_on_message_trigger() const { return this->on_message_trigger_; }

  void send_packet(uint8_t type, const std::vector<uint8_t> &data);

  void keypress(uint8_t key);

 protected:
  CrowAlarmPanelKeypad find_keypad_(uint8_t address);
  bool is_bus_idle_();

  // Transmission methods (blocking)
  void send_packet_blocking_(const std::vector<uint8_t> &packet);
  bool wait_for_clock_edge_(bool wait_for_high, uint32_t timeout_us);
  std::vector<uint8_t> keypress_queue_;
  uint32_t last_keypress_sent_ms_{0};
  bool disarm_in_progress_{false};
  uint32_t disarm_started_ms_{0};
  bool arm_in_progress_{false};
  uint32_t arm_started_ms_{0};

  CrowAlarmPanelStore store_;
  InternalGPIOPin *clock_pin_;
  InternalGPIOPin *data_pin_;
  uint8_t keypad_address_;
  text_sensor::TextSensor *armed_state_;
  alarm_control_panel::AlarmControlPanel *alarm_control_panel_{nullptr};
  Trigger<uint8_t, std::vector<uint8_t>> *on_message_trigger_{new Trigger<uint8_t, std::vector<uint8_t>>()};

  std::vector<CrowAlarmPanelZone> zones_;
  std::vector<CrowAlarmPanelKeypad> keypads_;
  std::vector<CrowAlarmPanelOutput> outputs_;
};

class CrowAlarmControlPanel : public alarm_control_panel::AlarmControlPanel, public Component {
 public:
  void dump_config() override;
  uint32_t get_supported_features() const override {
    return alarm_control_panel::ACP_FEAT_ARM_AWAY | alarm_control_panel::ACP_FEAT_ARM_HOME;
  }
  bool get_requires_code() const override { return this->requires_code_; }
  bool get_requires_code_to_arm() const override { return this->requires_code_to_arm_; }

  void set_parent(CrowAlarmPanel *parent) { this->parent_ = parent; }
  void set_code(const std::string &code) { this->code_ = code; }
  void set_requires_code(bool requires_code) { this->requires_code_ = requires_code; }
  void set_requires_code_to_arm(bool requires_code_to_arm) { this->requires_code_to_arm_ = requires_code_to_arm; }

 protected:
  void control(const alarm_control_panel::AlarmControlPanelCall &call) override;

  CrowAlarmPanel *parent_{nullptr};
  std::string code_;
  bool requires_code_{true};
  bool requires_code_to_arm_{false};
};

}  // namespace crow_alarm_panel
}  // namespace esphome
