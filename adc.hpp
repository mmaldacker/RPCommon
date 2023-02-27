//
// Created by Max on 13/12/2022.
//

#ifndef PICOSENSOR_ADC_HPP
#define PICOSENSOR_ADC_HPP

#include "hardware/adc.h"

class adc
{
public:
  adc();

  uint read(uint pin);
  float read_voltage(uint pin);
  float read_power_voltage();
};

int power_source(bool *battery_powered);

#endif  // PICOSENSOR_ADC_HPP
