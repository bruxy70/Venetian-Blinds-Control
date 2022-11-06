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
    ESP_LOGCONFIG(TAG, "  Tilt Open/Close Duration: %.1fs", this->tilt_duration / 1e3f);
    ESP_LOGCONFIG(TAG, "  Open Net Duration: %.1fs", this->open_net_duration_ / 1e3f);
    ESP_LOGCONFIG(TAG, "  Close Net Duration: %.1fs", this->close_net_duration_ / 1e3f);
    ESP_LOGCONFIG(TAG, "  Position: %.1f%", this->position);
    ESP_LOGCONFIG(TAG, "  Exact Position: %.1fs", this->exact_position_ / 1e3f);
    ESP_LOGCONFIG(TAG, "  Tilt: %.1f%", this->tilt);
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
    this->open_net_duration_ = this->open_duration - this->tilt_duration;
    this->close_net_duration_ = this->close_duration - this->tilt_duration;

    this->exact_position_ = this->close_net_duration_ * this->position; // position factor should be same for both open and close even if both durations are different
    this->exact_tilt_ = this->tilt_duration * this->tilt;
}

CoverTraits VenetianBlinds::get_traits() {
    auto traits = CoverTraits();
    traits.set_supports_position(true);
    traits.set_supports_tilt(true);
    traits.set_is_assumed_state(this->assumed_state);
    return traits;
}

void VenetianBlinds::control(const CoverCall &call) {
    if (call.get_stop()) {
        this->start_direction_(COVER_OPERATION_IDLE);
        this->publish_state();
    }
    if (call.get_position().has_value()) {
        auto pos = *call.get_position();
        if (pos != this->position) {
            auto op = pos < this->position ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
            this->target_position_ = pos * this->close_net_duration_; // TODO: FIXME to use based on operation
            this->start_direction_(op);
        }
    }
    if(call.get_tilt().has_value()) {
        auto tilt = *call.get_tilt();
        if (tilt != this->tilt) {
            auto op = tilt < this->tilt ? COVER_OPERATION_CLOSING : COVER_OPERATION_OPENING;
            this->target_tilt_ = tilt * this->tilt_duration;
            this->start_direction_(op);
        }
    }
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

    // Send current position every second
    if (now - this->last_publish_time_ > 1000) {
        this->publish_state(false);
        this->last_publish_time_ = now;
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
      return this->position >= this->target_position_;
    case COVER_OPERATION_CLOSING:
      return this->position <= this->target_position_;
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

  float dir;
  uint32_t action_dur;
  uint32_t tilt_boundary;
  switch (this->current_operation) {
    case COVER_OPERATION_OPENING:
      dir = 1.0f;
      action_dur = this->open_net_duration_;
      tilt_boundary = this->tilt_duration;
      break;
    case COVER_OPERATION_CLOSING:
      dir = -1.0f;
      action_dur = this->close_net_duration_;
      tilt_boundary = 0;
      break;
    default:
      return;
  }

  const uint32_t now = millis();
  this->exact_tilt_ += dir * (now - this->last_recompute_time_);
  this->exact_tilt_ = clamp(this->exact_tilt_, uint32_t(0), this->tilt_duration);
  const uint32_t tilt_overflow = dir * (this->tilt - tilt_boundary);
  if (tilt_overflow > 0) {
    this->exact_position_ += dir * tilt_overflow;
    this->exact_position_ = clamp(this->exact_position_, uint32_t(0), action_dur);
  }

  this->position = this->exact_position_ / action_dur;
  this->tilt = this->exact_tilt_ / this->tilt_duration;
  this->last_recompute_time_ = now;
}

}
}
