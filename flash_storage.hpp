//
// Created by Max on 15/01/2023.
//

#ifndef IRMIDICONTROLLER_FLASH_STORAGE_HPP
#define IRMIDICONTROLLER_FLASH_STORAGE_HPP

#include <vector>
#include <stdint.h>

class flash_storage
{
public:
  static std::vector<uint8_t> read(uint32_t offset, uint32_t size);
  static void write(uint32_t offset, const std::vector<uint8_t>& data);
};

#endif  // IRMIDICONTROLLER_FLASH_STORAGE_HPP
