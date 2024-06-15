#include "venetian_blinds.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace venetian_blinds {

static const char *TAG = "venetian_blinds.cover";

using namespace esphome::cover;

void VenetianBlinds::dump_config() {
  LOG_COVER("", "Venetian Blinds", this);
  ESP_LOGCONFIG(TAG, "  Open Duration: %.1fs", this->open_duration / 1e3f);
  ESP_LOGCONFIG(TAG, "  Close Duration: %.1fs", this->close_duration / 1e3f);
  ESP_LOGCONFIG(TAG, "  Tilt Open/Close Duration: %.1fs",
                this->tilt_duration / 1e3f);
  ESP_LOGCONFIG(TAG, "  Actuator Activation Duration: %.1fs",
                this->actuator_activation_duration / 1e3f);
  ESP_LOGCONFIG(TAG, "  Open Net Duration: %.1fs",
                this->open_net_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Close Net Duration: %.1fs",
                this->close_net_duration_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Position: %.1f%", this->position);
  ESP_LOGCONFIG(TAG, "  Tilt: %.1f%", this->tilt);
  ESP_LOGCONFIG(TAG, "  Exact Position: %.1fs", this->exact_position_ / 1e3f);
  ESP_LOGCONFIG(TAG, "  Exact Tilt: %.1fs", this->exact_tilt_ / 1e3f);
}

void VenetianBlinds::setup() {
  auto restore = this->restore_state_();
  if (restore.has_value()) {
    restore->apply(this);
  } else {
    this->position = 0.0;
    this->tilt = 0.0;
  }
  const uint32_t tilt_and_activation_duration = this->tilt_duration +
      this->actuator_activation_duration;
  this->open_net_duration_ = this->open_duration - tilt_and_activation_duration;
  this->close_net_duration_ = this->close_duration - tilt_and_activation_duration;

  this->exact_position_ =
      this->close_net_duration_ *
      this->position; // position factor should be same for both open and close
                      // even if both durations are different
  this->exact_tilt_ = this->tilt_duration * this->tilt;
}

void VenetianBlinds::loop() {
  if (this->current_operation == COVER_OPERATION_IDLE)
    return;

  const uint32_t now = millis();

  // Recompute position every loop cycle
  this->recompute_position_();

  if (this->is_at_target_()) {
    this->start_direction_(COVER_OPERATION_IDLE);
    this->publish_state();
  }

/*
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
*/

  // Send current position every second
  if (now - this->last_publish_time_ > 1000) {
    this->publish_state(false);
    this->last_publish_time_ = now;
  }
}

float VenetianBlinds::get_setup_priority() const { return setup_priority::DATA; }

CoverTraits VenetianBlinds::get_traits() {
  auto traits = CoverTraits();
  traits.set_supports_position(true);
  traits.set_supports_tilt(true);
  traits.set_supports_stop(true);
  traits.set_is_assumed_state(this->assumed_state);
  return traits;
}

void VenetianBlinds::control(const CoverCall &call) {
  if (call.get_stop()) {
    this->start_direction_(COVER_OPERATION_IDLE);
    this->publish_state();
  }
  if (call.get_position().has_value()) {
    auto requested_position = *call.get_position();
    if ((requested_position != this->position) ||
        (requested_position == 0 && this->tilt > 0)) {
      cover::CoverOperation operation;
      uint32_t operation_duration;
      if ((requested_position < this->position) ||
          (requested_position == 0)) {
        operation = COVER_OPERATION_CLOSING;
        operation_duration = this->close_net_duration_;
        this->target_tilt_ = 0.0f;
      } else {
        operation = COVER_OPERATION_OPENING;
        operation_duration = this->open_net_duration_;
        this->target_tilt_ = 1.0f;
      }

      this->target_position_ = requested_position * operation_duration;
      this->start_direction_(operation);
    }
  }
  if (call.get_tilt().has_value()) {
    auto requested_tilt = *call.get_tilt();
    if (requested_tilt != this->tilt) {
      auto operation = requested_tilt < this->tilt ? COVER_OPERATION_CLOSING
                                                   : COVER_OPERATION_OPENING;
      this->target_position_ = this->exact_position_;
      this->target_tilt_ = requested_tilt * this->tilt_duration;
      this->start_direction_(operation);
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
    return this->exact_position_ >= this->target_position_ &&
           this->exact_tilt_ >= this->target_tilt_;
  case COVER_OPERATION_CLOSING:
    return this->exact_position_ <= this->target_position_ &&
           this->exact_tilt_ <= this->target_tilt_;
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
    trig = this->stop_trigger;
    break;
  case COVER_OPERATION_OPENING:
    this->last_operation_ = dir;
    trig = this->open_trigger;
    break;
  case COVER_OPERATION_CLOSING:
    this->last_operation_ = dir;
    trig = this->close_trigger;
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

  const uint32_t now = millis();

  // when actuator is still activating, the position is not changed
  const uint32_t actuator_start_time = this->start_dir_time_ + this->actuator_activation_duration;
  const bool actuator_is_activating = now <= actuator_start_time;
  if (actuator_is_activating)
    return;

  int direction;
  int action_duration;
  int tilt_boundary;
  switch (this->current_operation) {
  case COVER_OPERATION_OPENING:
    direction = 1;
    action_duration = this->open_net_duration_;
    tilt_boundary = this->tilt_duration;
    break;
  case COVER_OPERATION_CLOSING:
    direction = -1;
    action_duration = this->close_net_duration_;
    tilt_boundary = 0;
    break;
  default:
    return;
  }

  /*
   * when actuator-activation finished between "start_direction_" and this
   * invocation of "recompute_position_", movement just started and the cover
   * moved for (now - actuator_start_time)
   */
  const uint32_t movement_start_time = this->last_recompute_time_ < actuator_start_time ?
      actuator_start_time : this->last_recompute_time_;
  const uint32_t cover_moving_time = now - movement_start_time;
  this->exact_tilt_ += direction * cover_moving_time;
  const int tilt_overflow = direction * (this->exact_tilt_ - tilt_boundary);
  this->exact_tilt_ = clamp(this->exact_tilt_, 0, (int)this->tilt_duration);
  if (tilt_overflow > 0) {
    this->exact_position_ += direction * tilt_overflow;
    this->exact_position_ = clamp(this->exact_position_, 0, action_duration);
  }

  this->position = this->exact_position_ / (float)action_duration;
  this->tilt = this->exact_tilt_ / (float)this->tilt_duration;

  this->last_recompute_time_ = now;
}

} // namespace venetian_blinds
} // namespace esphome
