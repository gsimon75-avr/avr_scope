#ifndef DEVICE_H
#define DEVICE_H 1

#include <stdint.h>
#include "common.h"

extern uint16_t voltage_factor;

void init_device(const char *devname);
void set_op_mode(op_mode_t m);
void set_sample_rate(uint8_t n);
void set_voltage_ref(uint8_t n);
void set_zero_level(uint8_t n);
void set_trig_level(uint8_t n);
void set_trig_type(trig_type_t type);
void set_pwm_prescaler(uint8_t n);
void set_pwm_total(uint8_t n);
void set_pwm_high(uint8_t n);
void shutdown_device();

#endif // DEVICE_H

