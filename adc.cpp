//
// Created by Max on 13/12/2022.
//

#include "adc.hpp"

adc::adc()
{
  adc_init();
  adc_set_temp_sensor_enabled(true);
}

uint adc::read(uint pin)
{
  adc_select_input(pin);
  return adc_read();
}
float adc::read_voltage(uint pin)
{
  const float conversionFactor = 3.3f / (1 << 12);
  return (float)read(pin) * conversionFactor;
}
