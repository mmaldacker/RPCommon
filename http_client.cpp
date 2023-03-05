//
// Created by Max on 04/03/2023.
//

#include "http_client.hpp"
#include "picohttpparser.h"
#include <charconv>

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

  int http_status;
  int minor_version;
  phr_header headers[100];
  size_t num_headers = sizeof(headers) / sizeof(headers[0]);
  size_t msg_length;
  const char *msg;
  int parsed_offset = 0;
  std::vector<uint8_t> response;

  auto status = client.wait_response(
      response,
      [&](const std::vector<uint8_t> &buffer) {

        num_headers = sizeof(headers) / sizeof(headers[0]);
        parsed_offset = phr_parse_response((const char *)buffer.data(), buffer.size(),
                                   &minor_version, &http_status, &msg, &msg_length,
                                   headers, &num_headers, 0);
        return parsed_offset >= 0;
      },
      timeout_ms);

  if (status == tcp_client::status::ok) {
    size_t content_length = 0;
    for (int i = 0; i < num_headers; i++)
    {
      if (std::string_view(headers[i].name, headers[i].name_len) == "Content-Length")
      {
        auto value = std::string_view(headers[i].value, headers[i].value_len);
        std::from_chars(value.begin(), value.end(), content_length);
        break;
      }
    }

    const char* body_content = (const char*)response.data() + parsed_offset;
     response_cb(http_status, content_length, body_content);
  } else {
    response_cb(-1, 0, nullptr);
  }
}