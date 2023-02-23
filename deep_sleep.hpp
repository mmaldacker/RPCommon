//
// Created by Max on 10/12/2022.
//

#pragma once

#include <stdint.h>

void deep_sleep_init();
void deep_sleep(uint32_t delay_ms);
void deep_sleep_pin(uint32_t gpio_pin, bool edge, bool high);