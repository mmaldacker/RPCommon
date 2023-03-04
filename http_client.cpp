//
// Created by Max on 04/03/2023.
//

#include "http_client.hpp"
#include "picohttpparser.h"

std::string make_http_header(const std::string &host, const std::string &method,
                             const std::string &path, std::uint32_t length) {
  const std::string request = method + " " + path + " HTTP/1.1\r\n";
  const std::string header_host = "Host: " + host + "\r\n";
  const std::string header_length =
      "Content-Length: " + std::to_string(length) + "\r\n";
  return request + header_host + header_length + "\r\n";
}

http_client::http_client(const std::string &ip, int port,
                         std::uint32_t timeout_ms)
    : client(ip, port, timeout_ms) {}

void http_client::send_http_request(
    const std::string &header, const std::string &body,
    std::uint32_t timeout_ms,
    std::function<void(int, size_t, const char *)> response_cb) {
  if (client.send_packet(header) != tcp_client::status::ok) {
    return;
  }
  if (!body.empty() && client.send_packet(body) != tcp_client::status::ok) {
    return;
  }
  if (client.flush(timeout_ms) != tcp_client::status::ok) {
    return;
  }

  int status;
  size_t msg_length;
  const char *msg;

  auto r = client.wait_response(
      [&](const std::vector<uint8_t> &buffer) {
        int minor_version;
        phr_header headers[100];
        size_t num_headers = sizeof(headers) / sizeof(headers[0]);

        int r = phr_parse_response((const char *)buffer.data(), buffer.size(),
                                   &minor_version, &status, &msg, &msg_length,
                                   headers, &num_headers, 0);
        return r >= 0;
      },
      timeout_ms);

  if (r == tcp_client::status::ok) {
    response_cb(status, msg_length, msg);
  } else {
    response_cb(-1, 0, nullptr);
  }
}