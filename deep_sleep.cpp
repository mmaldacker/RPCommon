//
// Created by Max on 10/12/2022.
//

#include "deep_sleep.hpp"
#include <hardware/gpio.h>
#include <hardware/regs/io_bank0.h>
#include <hardware/xosc.h>

#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/pll.h"
#include "hardware/structs/rosc.h"
#include "hardware/structs/scb.h"
#include "hardware/structs/timer.h"
#include "hardware/sync.h"

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

void deep_sleep_pin(uint32_t gpio_pin, bool edge, bool high)
{
  const uint32_t src_hz = 6.5 * MHZ;

  uint32_t ints = save_and_disable_interrupts();

  // Disable USB and ADC clocks.
  clock_stop(clk_usb);
  clock_stop(clk_adc);

  // CLK_REF = XOSC or ROSC
  clock_configure(clk_ref, CLOCKS_CLK_REF_CTRL_SRC_VALUE_ROSC_CLKSRC_PH, 0, src_hz,
                  src_hz);

  // CLK_SYS = CLK_REF
  clock_configure(clk_sys, CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLK_REF, 0, src_hz,
                  src_hz);

  // CLK_RTC = 46875
  clock_configure(clk_rtc, 0, CLOCKS_CLK_RTC_CTRL_AUXSRC_VALUE_ROSC_CLKSRC_PH,
                  src_hz, 46875);

  // CLK_PERI = CLK_SYS
  clock_configure(clk_peri, 0, CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  src_hz, src_hz);

  // Disable PLLs.
  pll_deinit(pll_sys);
  pll_deinit(pll_usb);

  // Disable XOSC.
  xosc_disable();

  bool low = !high;
  bool level = !edge;

  // Configure the appropriate IRQ at IO bank 0
  assert(gpio_pin < NUM_BANK0_GPIOS);

  uint32_t event = 0;

  if (level && low) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_LOW_BITS;
  if (level && high) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_LEVEL_HIGH_BITS;
  if (edge && high) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_HIGH_BITS;
  if (edge && low) event = IO_BANK0_DORMANT_WAKE_INTE0_GPIO0_EDGE_LOW_BITS;

  gpio_set_dormant_irq_enabled(gpio_pin, event, true);

  // Execution stops here until woken up
  // WARNING: This stops the rosc until woken up by an irq
  hw_clear_bits(&rosc_hw->status, ROSC_STATUS_BADWRITE_BITS);
  rosc_hw->dormant = ROSC_DORMANT_VALUE_DORMANT;
  // Wait for it to become stable once woken up
  while(!(rosc_hw->status & ROSC_STATUS_STABLE_BITS));

  // Clear the irq so we can go back to dormant mode again if we want
  gpio_acknowledge_irq(gpio_pin, event);

  // Bring back all clocks.
  clocks_init();
  restore_interrupts(ints);
}