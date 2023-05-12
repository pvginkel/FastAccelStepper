#ifndef RAMP_CONST_ACCELERATION_H
#define RAMP_CONST_ACCELERATION_H

#include "common.h"

struct ramp_ro_s {
  struct ramp_config_s config;
  bool force_stop : 1;
  bool force_immediate_stop : 1;
  bool incomplete_immediate_stop : 1;
  inline void init() {
    config.init();
    force_stop = false;
    force_immediate_stop = false;
  }
  inline int32_t targetPosition() { return config.parameters.target_pos; }
  inline void advanceTargetPositionWithinInterruptDisabledScope(int32_t delta) {
    config.parameters.target_pos += delta;
  }
  inline void immediateStop() { force_immediate_stop = true; }
  inline bool isImmediateStopInitiated() { return force_immediate_stop; }
  inline void clearImmediateStop() { force_immediate_stop = false; }
  inline void initiateStop() { force_stop = true; }
  inline bool isStopInitiated() { return force_stop; }
  inline void setKeepRunning() { config.parameters.keep_running = true; }
  inline bool isRunningContinuously() { return config.parameters.keep_running; }
};

struct ramp_rw_s {
  volatile uint8_t ramp_state;
  // if change_cnt does not match config.change_cnt, then eventually
  // performed_ramp_up_steps to be recalculated
  uint8_t change_cnt;
  // the speed is linked on both ramp slopes to this variable as per (if no
  // cubic ramp)
  //       s = v²/2a   =>   v = sqrt(2*a*s)
  uint32_t performed_ramp_up_steps;
  // Are the ticks stored of the last previous step, if pulse time requires
  // more than one command
  uint32_t pause_ticks_left;
  // Current ticks for ongoing step
  uint32_t curr_ticks;
  inline void stopRamp() {
    pause_ticks_left = 0;
    performed_ramp_up_steps = 0;
    ramp_state = RAMP_STATE_IDLE;
    curr_ticks = TICKS_FOR_STOPPED_MOTOR;
  }
  inline void init() {
    stopRamp();
    change_cnt = 0xff;
  }
  inline uint8_t rampState() {
    // reading one byte is atomic
    return ramp_state;
  }
  inline void startRampIfNotRunning() {
    // called with interrupts disabled
    if (ramp_state == RAMP_STATE_IDLE) {
      fasDisableInterrupts();
      ramp_state = RAMP_STATE_ACCELERATE;
      curr_ticks = TICKS_FOR_STOPPED_MOTOR;
      performed_ramp_up_steps = 0;
      fasEnableInterrupts();
    }
  }
};

class NextCommand {
 public:
  struct stepper_command_s command;
  struct ramp_rw_s rw;  // new _rw, if command has been queued
};

void init_ramp_module();
void _getNextCommand(const struct ramp_ro_s *ramp, const struct ramp_rw_s *rw,
                     const struct queue_end_s *queue_end, NextCommand *command);
#endif
