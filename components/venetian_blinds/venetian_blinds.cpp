#include "venetian_blinds.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace venetian_blinds {

static const char *TAG = "venetian_blinds.cover";

using namespace esphome::cover;

void VenetianBlinds::dump_config() {
    LOG_COVER("", "Venetian Blinds", this);
    ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration / 1e3f);
    ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration / 1e3f);
}

void VenetianBlinds::setup() {   
    auto restore = this->restore_state_();
    if (restore.has_value()) {
        restore->apply(this);
    } else {
        this->position = 0.0;
        this->tilt = 0.0;
    }
        exact_pos = this->position;
        exact_tilt = this->tilt;
}

CoverTraits VenetianBlinds::get_traits() {
    auto traits = CoverTraits();
    traits.set_supports_position(true);
    traits.set_supports_tilt(true);
    traits.set_is_assumed_state(this->assumed_state);
    return traits;
}

void VenetianBlinds::control(const CoverCall &call) {
    if (call.get_tilt().has_value()) {
        int new_tilt = *call.get_tilt()*100;
        relative_tilt = exact_tilt - new_tilt;
        relative_pos = 0;
        last_position_update = millis();
    }
    if (call.get_position().has_value()) {
        int new_pos = *call.get_position()*100;
        relative_pos = exact_pos - new_pos;
        relative_tilt = 0;
        last_position_update = millis();
    }
    if (call.get_stop()) {
        relative_pos = 0;
        relative_tilt = 0;
        this->stop_trigger->trigger();
        this->current_action = COVER_OPERATION_IDLE;
        this->position = exact_pos/100.0;
        this->tilt = exact_tilt/100.0;
        this->publish_state();
    }
}
void VenetianBlinds::loop() {
    uint32_t current_time = millis();
    if(relative_pos > 0 || relative_tilt < 0) {
        if(this->current_action != COVER_OPERATION_CLOSING) {
            this->close_trigger->trigger();
            this->current_action = COVER_OPERATION_CLOSING;
        }
        if(current_time - last_tilt_update >= (this->tilt_duration / 100)) {
            last_tilt_update = current_time;
            relative_tilt=clamp(relative_tilt+1,-100,0);
            exact_tilt=clamp(exact_tilt+1,0,100);
        }
        if(current_time - last_position_update >= (this->open_duration / 100)) {
            last_position_update = current_time;
            relative_pos=clamp(relative_pos-1,0,100);
            exact_pos--;
            if(exact_pos % 5 == 0) {
                this->position = exact_pos/100.0;
                this->tilt = exact_tilt/100.0;
                this->publish_state();
            }
        }
        if(relative_pos == 0 && relative_tilt == 0) {
            this->stop_trigger->trigger();
            this->current_action = COVER_OPERATION_IDLE;
            this->position = exact_pos/100.0;
            this->tilt = exact_tilt/100.0;
            this->publish_state();
        }
    } 
    else if(relative_pos < 0 || relative_tilt > 0) {
        if(this->current_action != COVER_OPERATION_OPENING) {
            this->open_trigger->trigger();
            this->current_action = COVER_OPERATION_OPENING;
        }
        if(current_time - last_tilt_update >= (this->tilt_duration / 100)) {
            last_tilt_update = current_time;
            relative_tilt=clamp(relative_tilt-1,0,100);
            exact_tilt=clamp(exact_tilt-1,0,100);
        }
        if(current_time - last_position_update >= (this->close_duration / 100)) {
            last_position_update = current_time;
            relative_pos=clamp(relative_pos+1,-100,0);
            exact_pos++;
            if(exact_pos % 5 == 0) {
                this->position = exact_pos/100.0;
                this->tilt = exact_tilt/100.0;
                this->publish_state();
            }
        }
        if(relative_pos == 0 && relative_tilt == 0) {
            this->stop_trigger->trigger();
            this->current_action = COVER_OPERATION_IDLE;
            this->position = exact_pos/100.0;
            this->tilt = exact_tilt/100.0;
            this->publish_state();
        }
    }
};

}
}