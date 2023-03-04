//
// Created by Max on 12/12/2022.
//
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#include "pico/cyw43_arch.h"
#include "tcp_client.hpp"

ip_addr_t tcp_client::getAddr(const std::string& ip)
{
  std::vector<uint> address;
  std::istringstream ipstream;
  ipstream.str(ip);
  for (std::string part; std::getline(ipstream, part, '.');)
  {
    address.push_back(std::stoi(part));
  }

  ip_addr_t ip_addr;
  IP4_ADDR(&ip_addr, address[0], address[1], address[2], address[3]);

  return ip_addr;
}

tcp_client::tcp_client(const std::string& ip, int port,
                       std::uint32_t timeout_ms)
{
  pcb = tcp_new();

  tcp_arg(pcb, this);

  auto err_callback = +[](void* arg, err_t err) {
    printf("TCP Error: %s\n", lwip_strerr(err));
    auto* client = static_cast<tcp_client*>(arg);
    client->is_error = true;
  };
  tcp_err(pcb, err_callback);

  auto tcp_recv_callback = +[](void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) -> err_t
  {
    auto* client = static_cast<tcp_client*>(arg);

    printf("TCP Recv: %s Len: %d\n", lwip_strerr(err), p == nullptr ? -1 : p->tot_len);

    if (p == nullptr)
    {
      client->is_error = true;
      return ERR_OK;
    }

    std::vector<std::uint8_t> data(p->tot_len);
    unsigned int offset = 0;
    for (struct pbuf* q = p; q != nullptr; q = q->next)
    {
      std::memcpy(data.data() + offset, q->payload, q->len);
      offset += q->len;
    }

    client->received_buffer = std::move(data);

    tcp_recved(client->pcb, client->received_buffer.size());
    pbuf_free(p);

    return ERR_OK;
  };
  tcp_recv(pcb, tcp_recv_callback);

  auto tcp_send_callback = +[](void* arg, struct tcp_pcb* tpcb, u16_t len) -> err_t
  {
    printf("TCP Send: %d\n", len);
    static_cast<tcp_client*>(arg)->num_send_remaining -= len;
    return ERR_OK;
  };
  tcp_sent(pcb, tcp_send_callback);

  auto tcp_connected_callback = +[](void* arg, struct tcp_pcb* tpcb, err_t err) -> err_t
  {
    printf("TCP Connected: %s\n", lwip_strerr(err));

    if (err == ERR_OK)
    {
      static_cast<tcp_client*>(arg)->is_connected = true;
      return ERR_OK;
    }

    return ERR_ABRT;
  };

  ip_addr_t ip_addr = getAddr(ip);

  tcp_connect(pcb, &ip_addr, port, tcp_connected_callback);

  wait([&] { return is_connected; }, timeout_ms);
}

tcp_client::~tcp_client()
{
  tcp_close(pcb);

  printf("TCP Disconnected\n");
}

tcp_client::status tcp_client::send_packet(const std::string& s)
{
  if (!is_connected || is_error)
  {
    printf("Not connected or error\n");
    return status::error;
  }

  printf("TCP Sending: %d\n", s.size());
  num_send_remaining += s.size();

  auto error = tcp_write(pcb, s.c_str(), s.length(), TCP_WRITE_FLAG_COPY);
  if (error)
  {
    printf("Error TCP Send: %s\n", lwip_strerr(error));
    return status::error;
  }

  return status::ok;
}

tcp_client::status tcp_client::flush(std::uint32_t timeout_ms)
{
  if (!is_connected || is_error)
  {
    printf("Not connected or error\n");
    return status::error;
  }

  auto error = tcp_output(pcb);
  if (error)
  {
    printf("Error TCP Output: %s\n", lwip_strerr(error));
    return status::error;
  }

  return wait([&] { return num_send_remaining == 0; }, timeout_ms);
}

tcp_client::status tcp_client::wait_response(const std::function<bool(const std::vector<std::uint8_t> buffer)>& f,
                               std::uint32_t timeout_ms)
{
  if (!is_connected || is_error)
  {
    printf("Not connected or error\n");
    return status::error;
  }

  auto start_ms = to_ms_since_boot(get_absolute_time());
  while (to_ms_since_boot(get_absolute_time()) - start_ms < timeout_ms)
  {
    if (is_received || is_error || f(received_buffer))
    {
      break;
    }

    cyw43_arch_poll();
    sleep_ms(5);
  }

  if (is_error)
  {
    printf("TCP wait response error\n");
    return status::error;
  }

  if (!f(received_buffer) && !is_received)
  {
    printf("TCP Wait response timeout\n");
    return status::timeout;
  }

  is_received = false;
  return status::ok;
}

tcp_client::status tcp_client::wait(const std::function<bool()>& predicate,
                      std::uint32_t timeout_ms)
{
  auto start_ms = to_ms_since_boot(get_absolute_time());
  while (!is_error && !predicate() && to_ms_since_boot(get_absolute_time()) - start_ms < timeout_ms)
  {
    cyw43_arch_poll();
    sleep_ms(5);
  }

  if (is_error)
  {
    printf("TCP Wait Error\n");
    return status::error;
  }

  if (!predicate())
  {
    printf("TCP Wait timeout\n");
    return status::timeout;
  }

  return status::ok;
}
