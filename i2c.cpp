//
// Created by Max on 13/12/2022.
//

#include "i2c.hpp"
#include "hardware/gpio.h"

i2c::i2c(uint i2c, uint sdaPin, uint sclPin) : i2c_inst(i2c == 0 ? i2c0 : i2c1)
{
  i2c_init(i2c_inst, 100 * 1000);
  gpio_set_function(sdaPin, GPIO_FUNC_I2C);
  gpio_set_function(sclPin, GPIO_FUNC_I2C);
  gpio_pull_up(sdaPin);
  gpio_pull_up(sclPin);
}

i2c::~i2c()
{
    i2c_deinit(i2c_inst);
}

i2c_inst_t* i2c::get_i2c_inst() const
{
  return i2c_inst;
}
