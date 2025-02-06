//
// Created by Max on 12/12/2022.
//

#ifndef PICOSENSOR_WIRELESS_HPP
#define PICOSENSOR_WIRELESS_HPP

#include <string>
#include <cstdint>

class wireless
{
public:
  wireless();

  void connect(const std::string& ssid, const std::string& password, std::uint32_t timeout_ms, int num_retries = 3);
  [[nodiscard]] bool is_connected() const;

  void led_on();
  void led_off();

  ~wireless();
};

#endif  // PICOSENSOR_WIRELESS_HPP
