//
// Created by Max on 13/12/2022.
//

#include "adc.hpp"

#if defined CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#ifndef PICO_POWER_SAMPLE_COUNT
#define PICO_POWER_SAMPLE_COUNT 3
#endif

// Pin used for ADC 0
#define PICO_FIRST_ADC_PIN 26

int power_source(bool *battery_powered) {
#if defined CYW43_WL_GPIO_VBUS_PIN
    *battery_powered = !cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
    return PICO_OK;
#elif defined PICO_VBUS_PIN
    gpio_set_function(PICO_VBUS_PIN, GPIO_FUNC_SIO);
    *battery_powered = !gpio_get(PICO_VBUS_PIN);
    return PICO_OK;
#else
    return PICO_ERROR_NO_DATA;
#endif
}


adc::adc() {
    adc_init();
    adc_set_temp_sensor_enabled(true);
}

uint adc::read(uint pin) {
    adc_select_input(pin);
    return adc_read();
}

float adc::read_voltage(uint pin) {
    const float conversionFactor = 3.3f / (1 << 12);
    return (float) read(pin) * conversionFactor;
}

float adc::read_power_voltage() {
#ifndef PICO_VSYS_PIN
    return -1.0f;
#endif
#if CYW43_USES_VSYS_PIN
    cyw43_thread_enter();
#endif
    // setup adc
    adc_gpio_init(PICO_VSYS_PIN);
    adc_select_input(PICO_VSYS_PIN - PICO_FIRST_ADC_PIN);
    // read vsys
    uint32_t vsys = 0;
    for (int i = 0; i < PICO_POWER_SAMPLE_COUNT; i++) {
        vsys += adc_read();
    }
    vsys /= PICO_POWER_SAMPLE_COUNT;
#if CYW43_USES_VSYS_PIN
    cyw43_thread_exit();
#endif
    // Generate voltage
    const float conversion_factor = 3.3f / (1 << 12);
    return vsys * 3.0f * conversion_factor;
}