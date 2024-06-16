#include "venetian_blinds.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace venetian_blinds {

static const char *TAG = "venetian_blinds.cover";

using namespace esphome::cover;

void VenetianBlinds::dump_config() {
  LOG_COVER("", "Venetian Blinds", this);
  ESP_LOGCONFIG(TAG, "  Position: %.1f%", this->position);
  ESP_LOGCONFIG(TAG, "  Tilt: %.1f%", this->tilt);
  ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Tilt Duration: %.1fs", this->tilt_duration_ / 1e3f);

}
void VenetianBlinds::setup() {
//  auto restore = this->restore_state_();
//  if (restore.has_value()) {
//    restore->apply(this);
//  } else {
    this->position = 0.5f;
    this->tilt = 0.0f;
//  }
}
void VenetianBlinds::loop() {
  if (this->current_operation == COVER_OPERATION_IDLE)
    return;

  const uint32_t now = millis();

  // Recompute position every loop cycle
  this->recompute_position_();

  if (this->is_at_target_()) {
    if (this->has_built_in_endstop_ &&
        (this->target_position_ == COVER_OPEN || this->target_position_ == COVER_CLOSED)) {
      // Don't trigger stop, let the cover stop by itself.
      this->current_operation = COVER_OPERATION_IDLE;
    } else {
      this->start_direction_(COVER_OPERATION_IDLE);
    }
    this->publish_state();
  }

  // Send current position every second
  if (now - this->last_publish_time_ > 1000) {
    this->publish_state(false);
    this->last_publish_time_ = now;
  }
}
float VenetianBlinds::get_setup_priority() const { return setup_priority::DATA; }
CoverTraits VenetianBlinds::get_traits() {
  auto traits = CoverTraits();
  traits.set_supports_stop(true);
  traits.set_supports_position(true);
  traits.set_supports_tilt(this->tilt_duration_ > 0);
  traits.set_supports_toggle(true);
  traits.set_is_assumed_state(this->assumed_state_);
  return traits;
}
void VenetianBlinds::control(const CoverCall &call) {
  if (call.get_stop()) {
    this->start_direction_(COVER_OPERATION_IDLE);
    this->publish_state();
  }
  if (call.get_toggle().has_value()) {
    if (this->current_operation != COVER_OPERATION_IDLE) {
      this->start_direction_(COVER_OPERATION_IDLE);
      this->publish_state();
    } else {
      if (this->position == COVER_CLOSED || this->last_operation_ == COVER_OPERATION_CLOSING) {
        this->target_position_ = COVER_OPEN;
        this->target_tilt_ = 1.0f;
        this->start_direction_(COVER_OPERATION_OPENING);
      } else {
        this->target_position_ = COVER_CLOSED;
        this->target_tilt_ = 0.0f;
        this->start_direction_(COVER_OPERATION_CLOSING);
      }
    }
  }
  if (call.get_position().has_value()) {
    auto pos = *call.get_position();
    if (pos == this->position) {
      // already at target
      if (this->manual_control_ && (pos == COVER_OPEN || pos == COVER_CLOSED)) {
        // for covers with manual control switch, we can't rely on the computed position, so if
        // the command triggered again, we'll assume it's in the opposite direction anyway.
        auto op = pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
        this->position = pos == COVER_CLOSED ? COVER_OPEN : COVER_CLOSED;
        this->target_position_ = pos;
        this->target_tilt_ = pos == COVER_CLOSED ? 0.0f : 1.0f;
        this->start_direction_(op);
      }
      // for covers with built in end stop, we should send the command again
      if (this->has_built_in_endstop_ && (pos == COVER_OPEN || pos == COVER_CLOSED)) {
        auto op = pos == COVER_CLOSED ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
        this->target_position_ = pos;
        this->target_tilt_ = pos == COVER_CLOSED ? 0.0f : 1.0f;
        this->start_direction_(op);
      }
    } else {
      auto op = pos < this->position ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
      if (this->manual_control_ && (pos == COVER_OPEN || pos == COVER_CLOSED)) {
        this->position = pos == COVER_CLOSED ? COVER_OPEN : COVER_CLOSED;
      }
      this->target_position_ = pos;
      this->target_tilt_ = pos == COVER_CLOSED ? 0.0f : 1.0f;
      this->start_direction_(op);
    }
  }
  if (call.get_tilt().has_value()) {
    auto tilt = *call.get_tilt();
    if (tilt != this->tilt) {
      // not at target
      auto op = tilt < this->tilt ? COVER_OPERATION_CLOSING
                                  : COVER_OPERATION_OPENING;
      this->target_position_ = this->position;
      this->target_tilt_ = tilt;
      this->start_direction_(op);
    }
  }  
}
void VenetianBlinds::stop_prev_trigger_() {
  if (this->prev_command_trigger_ != nullptr) {
    this->prev_command_trigger_->stop_action();
    this->prev_command_trigger_ = nullptr;
  }
}
bool VenetianBlinds::is_at_target_() const {
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      return this->position >= this->target_position_ &&
             (this->tilt_duration_ == 0 || this->tilt >= this->target_tilt_);
    case COVER_OPERATION_CLOSING:
      return this->position <= this->target_position_ &&
             (this->tilt_duration_ == 0 || this->tilt <= this->target_tilt_);
    case COVER_OPERATION_IDLE:
    default:
      return true;
  }
}
void VenetianBlinds::start_direction_(CoverOperation dir) {
  if (dir == this->current_operation && dir != COVER_OPERATION_IDLE)
    return;

  this->recompute_position_();
  Trigger<> *trig;
  switch (dir) {
    case COVER_OPERATION_IDLE:
      trig = this->stop_trigger_;
      break;
    case COVER_OPERATION_OPENING:
      this->last_operation_ = dir;
      trig = this->open_trigger_;
      break;
    case COVER_OPERATION_CLOSING:
      this->last_operation_ = dir;
      trig = this->close_trigger_;
      break;
    default:
      return;
  }

  this->current_operation = dir;

  const uint32_t now = millis();
  this->start_dir_time_ = now;
  this->last_recompute_time_ = now;

  this->stop_prev_trigger_();
  trig->trigger();
  this->prev_command_trigger_ = trig;
}
void VenetianBlinds::recompute_position_() {
  if (this->current_operation == COVER_OPERATION_IDLE)
    return;

  float dir;
  float action_dur;
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      dir = 1.0f;
      action_dur = this->open_duration_;
      break;
    case COVER_OPERATION_CLOSING:
      dir = -1.0f;
      action_dur = this->close_duration_;
      break;
    default:
      return;
  }

  const uint32_t now = millis();

  // First tilt, when tilting is done, change position
  if (this->tilt_duration_ > 0 && (
    (dir > 0 && this->tilt >= this->target_tilt_) || 
    (dir < 0 && this->tilt <= this->target_tilt_)))
  {
    this->tilt += dir * (now - this->last_recompute_time_) / this->tilt_duration_;
    this->tilt = clamp(this->tilt, 0.0f, 1.0f);
  } else {
    this->position += dir * (now - this->last_recompute_time_) / action_dur;
    this->position = clamp(this->position, 0.0f, 1.0f);
  }
  this->last_recompute_time_ = now;
}

}  // namespace venetian_blinds
}  // namespace esphome