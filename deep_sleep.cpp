//
// Created by Max on 10/12/2022.
//

#include "deep_sleep.hpp"

#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/rosc.h"
#include "hardware/sync.h"
#include "hardware/pll.h"

#define ALARM_NUM 0
#define ALARM_IRQ TIMER_IRQ_0

void alarm_irq()
{
  // Clear the alarm irq
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
}

void deep_sleep_init()
{
  // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
  hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);
  // Set irq handler for alarm irq
  irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
  // Enable the alarm irq
  irq_set_enabled(ALARM_IRQ, true);
}

void deep_sleep(uint32_t delay_ms)
{
  const uint32_t xosc_hz = XOSC_MHZ * MHZ;

  uint32_t ints = save_and_disable_interrupts();

  // Disable USB and ADC clocks.
  clock_stop(clk_usb);
  clock_stop(clk_adc);

  // CLK_REF = XOSC
  clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC, 0, xosc_hz, xosc_hz);

  // CLK_SYS = CLK_REF
  clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, xosc_hz, xosc_hz);

  // CLK_RTC = XOSC / 256
  clock_configure(clk_rtc, 0, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_XOSC_CLKSRC, xosc_hz, xosc_hz / 256);

  // CLK_PERI = CLK_SYS
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS, xosc_hz, xosc_hz);

  // Disable PLLs.
  pll_deinit(pll_sys);
  pll_deinit(pll_usb);

  // Disable ROSC.
  rosc_hw->ctrl = ROSC_CTRL_ENABLE_VALUE_DISABLE << ROSC_CTRL_ENABLE_LSB;

  uint32_t sleep_en0 = clocks_hw->sleep_en0;
  uint32_t sleep_en1 = clocks_hw->sleep_en1;
  clocks_hw->sleep_en0 = CLOCKS_SLEEP_EN0_CLK_RTC_RTC_BITS;

  // Use timer alarm to wake.
  clocks_hw->sleep_en1 = CLOCKS_SLEEP_EN1_CLK_SYS_TIMER_BITS;
  timer_hw->alarm[ALARM_NUM] = timer_hw->timerawl + delay_ms * 1000;

  scb_hw->scr |= M0PLUS_SCR_SLEEPDEEP_BITS;
  __wfi();
  scb_hw->scr &= ~M0PLUS_SCR_SLEEPDEEP_BITS;
  clocks_hw->sleep_en0 = sleep_en0;
  clocks_hw->sleep_en1 = sleep_en1;

  // Enable ROSC.
  rosc_hw->ctrl = ROSC_CTRL_ENABLE_VALUE_ENABLE << ROSC_CTRL_ENABLE_LSB;

  // Bring back all clocks.
  clocks_init();
  restore_interrupts(ints);
}