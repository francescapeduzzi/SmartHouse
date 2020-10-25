#pragma once
#include <stdint.h>

typedef enum {
  PWMEnabled=1,
  PWMSuccess=0,
  PWMChannelOutOfBound=-1,
  PWMInvalidDutyCycle=-2,
  PWMInvalidPeriod=-3
} PWMError;

// initializes the pwm subsystem, and sets the PWM period to something reasonable
PWMError PWM_init(void);

// how many pwm on this chip?
uint8_t PWM_numChannels(void);

// what was the duty cycle I last set? negative value means error
uint8_t PWM_getDutyCycle(uint8_t pwm_channel);

// sets the duty cycle
PWMError PWM_setDutyCycle(uint8_t pwm_channel, uint8_t duty_cycle);

// enables a pwm on a channel
PWMError PWM_enable(uint8_t num_channel, uint8_t enable);

// enables a pwm on a channel
PWMError PWM_isEnabled(uint8_t num_channel);

