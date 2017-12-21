// EPOS ARM Cortex PWM Mediator Declarations

#ifndef __cortex_pwm_h
#define __cortex_pwm_h

#include <timer.h>
#include __MODEL_H

__BEGIN_SYS

class PWM: private PWM_Common, private Machine_Model
{
public:
    PWM(User_timer * timer, GPIO * gpio, const Percent & duty_cycle)
    : _timer(timer), _gpio(gpio) {
        enable_pwm(_timer->_channel, _gpio->_port, _gpio->_pin);
        timer->pwm(duty_cycle);
    }
    ~PWM() { disable_pwm(_timer->_channel, _gpio->_port, _gpio->_pin); }

    void enable() { _timer->enable(); }
    void disable() { _timer->disable(); }
    void power(const Power_Mode & mode) { timer->power(mode); }

private:
    GPIO * _gpio;
    User_timer * _timer;
};

__END_SYS

#endif
