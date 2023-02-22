//
// Created by Max on 15/01/2023.
//

#include "flash_storage.hpp"
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>

constexpr uint32_t START_OFFSET = 0x200000 - 10 * FLASH_SECTOR_SIZE;
const uint8_t* START_ADDRESS = (const uint8_t*)(XIP_BASE);

std::vector<uint8_t> flash_storage::read(uint32_t offset, uint32_t size)
{
  std::vector<uint8_t> data(size);
  std::memcpy(data.data(), START_ADDRESS + START_OFFSET + offset, size);
  return data;
}
void flash_storage::write(uint32_t offset, const std::vector<uint8_t>& data)
{
  std::vector<uint8_t> write_data = data;

  auto aligned_size = (std::uint32_t)data.size();
  auto d = std::div((int)data.size(), FLASH_PAGE_SIZE);
  if (d.rem != 0)
  {
    aligned_size = (d.quot + 1) * FLASH_PAGE_SIZE;
  }

  write_data.resize(aligned_size);

  uint32_t ints = save_and_disable_interrupts();
  flash_range_program(START_OFFSET + offset, write_data.data(), write_data.size());
  restore_interrupts(ints);
}
