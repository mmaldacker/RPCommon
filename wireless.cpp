//
// Created by Max on 12/12/2022.
//

#include "wireless.hpp"
#include "pico/cyw43_arch.h"

wireless::wireless()
{
  if (cyw43_arch_init())
  {
    printf("WiFi init failed\n");
  }
  else
  {
    cyw43_arch_enable_sta_mode();
    printf("WiFi initialized\n");
  }
}

void wireless::connect(const std::string& ssid,
                       const std::string& password,
                       std::uint32_t timeout_ms,
                       int num_retries)
{
  if (is_connected())
  {
    printf("Already connected\n");
    return;
  }

  for (int i = 0;
       i < num_retries && cyw43_arch_wifi_connect_timeout_ms(
                              ssid.c_str(), password.c_str(), CYW43_AUTH_WPA2_AES_PSK, timeout_ms);
       i++)
  {
    printf("Failed to connect.\n");
  }
}

wireless::~wireless()
{
  cyw43_arch_deinit();
  printf("WiFi deinitialized\n");
}
void wireless::led_on()
{
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
}
void wireless::led_off()
{
  cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}
bool wireless::is_connected() const
{
  return cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP;
}
