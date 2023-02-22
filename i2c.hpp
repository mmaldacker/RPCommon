//
// Created by Max on 13/12/2022.
//

#ifndef PICOSENSOR_I2C_HPP
#define PICOSENSOR_I2C_HPP

#include "hardware/i2c.h"

class i2c
{
public:
  i2c(uint i2c, uint sdaPin, uint sclPin);
  ~i2c();

  i2c_inst_t* get_i2c_inst() const;
private:
  i2c_inst_t* i2c_inst;
};

#endif  // PICOSENSOR_I2C_HPP
