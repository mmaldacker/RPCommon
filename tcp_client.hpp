//
// Created by Max on 12/12/2022.
//

#ifndef PICOSENSOR_TCP_CLIENT_HPP
#define PICOSENSOR_TCP_CLIENT_HPP

#include <functional>
#include <string>

#include "lwip/pbuf.h"
#include "lwip/sockets.h"
#include "lwip/tcp.h"

class tcp_client
{
public:
  enum class status
  {
    ok,
    error,
    timeout
  };

  explicit tcp_client(const std::string& ip, int port, std::uint32_t timeout_ms);
  ~tcp_client();

  status send_packet(const std::string& s);
  status flush(std::uint32_t timeout_ms);

  status wait_response(const std::function<bool(const std::vector<std::uint8_t> buffer)>& f,
                     std::uint32_t timeout_ms);

private:
  tcp_pcb* pcb;
  volatile bool is_connected = false;
  volatile std::uint32_t num_send_remaining = 0;
  volatile bool is_received = false;
  volatile bool is_error = false;
  std::vector<std::uint8_t> received_buffer;

  static ip_addr_t getAddr(const std::string& ip);

  status wait(const std::function<bool()>& predicate, std::uint32_t timeout_ms);
};

#endif  // PICOSENSOR_TCP_CLIENT_HPP
