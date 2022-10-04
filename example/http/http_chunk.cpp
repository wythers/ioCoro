#include "http_chunk.hpp"

using namespace http;

std::pair<char const*, char const*> const
http_response::parse_status_line()
{
  string_view checker{ m_response_body };

  int pos{};
  pos = checker.find(" ");
  strncpy(m_http_version, checker.data(), pos);
  checker.remove_prefix(pos + 1);

  pos = checker.find(" ");
  strncpy(m_status_code, checker.data(), pos);
  checker.remove_prefix(pos + 1);

  pos = checker.find("\r\n");
  strncpy(m_status_message, checker.data(), pos);

  if (m_http_version != "HTTP/1.1"sv || m_status_code != "200"sv ||
      m_status_message != "OK"sv)
    return { m_status_code, m_status_message };

  return {};
}

std::pair<char const*, char const*> const
http_response::parse_header_line()
{
  string_view checker{ m_response_body };
  checker.remove_prefix(checker.find("\r\n") + 2);

  for (size_t pos = checker.find("\r\n"); pos != 0;) {
    auto spliced = checker.find(": ");
    m_headers[{ checker.data(), spliced }] = { checker.data() + spliced + 2,
                                               pos - spliced - 2 };

    checker.remove_prefix(pos + 2);
    pos = checker.find("\r\n");
  }

  if (!m_headers.count("Content-Length"))
    return { "500", "Server Error" };

  m_entity_body.resize(atoi(m_headers["Content-Length"].data()));
  return {};
}

bool
http_response::had_header_line()
{
  if (string_view{ m_response_body }.find("\r\n\r\n") != string_view::npos)
    return true;

  return false;
}

int
http_request::parse_status_line()
{
  string_view checker{ m_request_body };

  int pos{};
  pos = checker.find(" ");
  strncpy(m_method, checker.data(), pos);
  checker.remove_prefix(pos + 1);

  pos = checker.find(" ");
  strncpy(m_src_path, checker.data(), pos);
  checker.remove_prefix(pos + 1);

  pos = checker.find("\r\n");
  strncpy(m_http_version, checker.data(), pos);

  if (m_http_version != "HTTP/1.1"sv)
    return 505;

  if (m_method != "GET"sv)
    return 501;

  return 0;
}

int
http_request::respond_to_request()
{
  struct stat st{};
  int ret = stat(m_src_path, &st);
  if (ret == -1)
    return 404;

  FILE* file = fopen(m_src_path, "r");
  m_entity_body.resize(st.st_size);

  fread(m_entity_body.data(), 1, m_entity_body.size(), file);
  
  fclose(file);
  return 0;
}