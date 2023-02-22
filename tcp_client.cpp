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

  auto err_callback = +[](void* arg, err_t err) { printf("TCP Error: %s\n", lwip_strerr(err)); };
  tcp_err(pcb, err_callback);

  auto tcp_recv_callback = +[](void* arg, struct tcp_pcb* tpcb, struct pbuf* p, err_t err) -> err_t
  {
    auto* client = static_cast<tcp_client*>(arg);

    printf("TCP Recv: %s Len: %d\n", lwip_strerr(err), p == nullptr ? -1 : p->tot_len);

    if (p == nullptr)
    {
      client->is_received = true;
      return ERR_OK;
    }

    std::vector<std::uint8_t> data(p->tot_len);
    unsigned int offset = 0;
    for (struct pbuf* q = p; q != nullptr; q = q->next)
    {
      std::memcpy(data.data() + offset, q->payload, q->len);
      offset += q->len;
    }

    printf("TCP Recv:\n%s\n", data.data());

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

bool tcp_client::send_packet(const std::string& s,
                             std::uint32_t timeout_ms)
{
  if (!is_connected)
  {
    return false;
  }

  printf("TCP Sending: %d\n", s.size());
  num_send_remaining = s.size();

  auto error = tcp_write(pcb, s.c_str(), s.length(), TCP_WRITE_FLAG_COPY);
  if (error)
  {
    printf("Error TCP Send: %s\n", lwip_strerr(error));
    return false;
  }

  error = tcp_output(pcb);
  if (error)
  {
    printf("Error TCP Output: %s\n", lwip_strerr(error));
    return false;
  }

  wait([&] { return num_send_remaining == 0; }, timeout_ms);
  return true;
}

bool tcp_client::send_http_request(const std::string& header, const std::string& body,
                                   std::uint32_t timeout_ms)
{
  if (!is_connected)
  {
    return false;
  }

  std::uint32_t size = header.length() + body.length();

  printf("TCP Sending: %lu\n", size);
  num_send_remaining = size;

  auto error =
      tcp_write(pcb, header.c_str(), header.length(), TCP_WRITE_FLAG_COPY | TCP_WRITE_FLAG_MORE);
  if (error)
  {
    printf("Error TCP Send: %s\n", lwip_strerr(error));
    return false;
  }

  error = tcp_write(pcb, body.c_str(), body.length(), TCP_WRITE_FLAG_COPY);
  if (error)
  {
    printf("Error TCP Send: %s\n", lwip_strerr(error));
    return false;
  }

  error = tcp_output(pcb);
  if (error)
  {
    printf("Error TCP Output: %s\n", lwip_strerr(error));
    return false;
  }

  wait([&] { return num_send_remaining == 0; }, timeout_ms);
  return true;
}

void tcp_client::wait_response(const std::function<bool(const std::vector<std::uint8_t> buffer)>& f,
                               std::uint32_t timeout_ms)
{
  if (!is_connected)
  {
    return;
  }

  auto start_ms = to_ms_since_boot(get_absolute_time());
  while (to_ms_since_boot(get_absolute_time()) - start_ms < timeout_ms)
  {
    if (is_received)
    {
      break;
    }

    if (f(received_buffer))
    {
      break;
    }

    cyw43_arch_poll();
    sleep_ms(5);
  }

  if (!f(received_buffer) && !is_received)
  {
    printf("TCP Wait response timeout\n");
  }

  is_received = false;
}

void tcp_client::wait(const std::function<bool()>& predicate,
                      std::uint32_t timeout_ms)
{
  auto start_ms = to_ms_since_boot(get_absolute_time());
  while (!predicate() && to_ms_since_boot(get_absolute_time()) - start_ms < timeout_ms)
  {
    cyw43_arch_poll();
    sleep_ms(5);
  }

  if (!predicate())
  {
    printf("TCP Wait timeout\n");
  }
}
