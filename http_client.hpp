//
// Created by Max on 04/03/2023.
//

#ifndef WEATHERDISPLAY_HTTP_CLIENT_HPP
#define WEATHERDISPLAY_HTTP_CLIENT_HPP

#include <string>
#include "tcp_client.hpp"

std::string make_http_header(const std::string& host,
                             const std::string& method,
                             const std::string& path,
                             std::uint32_t length = 0);

class http_client {
public:
  explicit http_client(const std::string& ip, int port, std::uint32_t timeout_ms);

  void send_http_request(const std::string& header,
                         const std::string& body,
                         std::uint32_t timeout_ms,
                         std::function<void(int, size_t, const char*)> response_cb);

private:
  tcp_client client;
};

#endif // WEATHERDISPLAY_HTTP_CLIENT_HPP
