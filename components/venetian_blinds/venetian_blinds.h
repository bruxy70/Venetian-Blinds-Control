#pragma once
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "esphome/components/cover/cover.h"

namespace esphome {
namespace venetian_blinds {

class VenetianBlinds : public Component, public cover::Cover {
  public:
    void setup() override;
    void loop() override;
    void dump_config() override;
    cover::CoverTraits get_traits() override;
    void control(const cover::CoverCall &call) override;
    Trigger<> *get_open_trigger() const { return this->open_trigger; }
    Trigger<> *get_close_trigger() const { return this->close_trigger; }
    Trigger<> *get_stop_trigger() const { return this->stop_trigger; }    
    void set_open_duration(uint32_t open) { this->open_duration = open; }
    void set_close_duration(uint32_t close) { this->close_duration = close; }
    void set_tilt_duration(uint32_t tilt) { this->tilt_duration = tilt; }
    void set_assumed_state(bool value) { this->assumed_state = value; }
  private:
    uint32_t last_position_update{0};
    uint32_t last_tilt_update{0};
    int exact_pos;
    int relative_pos{0};
    int exact_tilt;
    int relative_tilt{0};
    cover::CoverOperation current_action{cover::COVER_OPERATION_IDLE};
  protected:
    Trigger<> *open_trigger{new Trigger<>()};
    Trigger<> *close_trigger{new Trigger<>()};
    Trigger<> *stop_trigger{new Trigger<>()};
    uint32_t open_duration;
    uint32_t close_duration;
    uint32_t tilt_duration;
    bool assumed_state{false};
};

}
}