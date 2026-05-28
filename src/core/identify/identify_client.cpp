#include "core/identify/identify_client.hpp"

#include "core/print.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace cathook::core::identify
{

identify_client::~identify_client()
{
  stop();
}

void identify_client::set_peers_handler(peers_callback_t callback)
{
  m_peers_callback = std::move(callback);
}

void identify_client::set_chat_handler(chat_callback_t callback)
{
  m_chat_callback = std::move(callback);
}

void identify_client::connect(std::string host, int port)
{
  m_host = std::move(host);
  m_port = port;
  m_running = true;
  m_worker = std::thread{[this]() {
    worker_loop();
  }};
}

void identify_client::stop()
{
  m_running = false;
  close_socket();
  m_cv.notify_all();
  if (m_worker.joinable())
  {
    m_worker.join();
  }
}

void identify_client::update_identity(std::string_view server_id, std::string_view player_hash, std::string_view signature, int head_scale)
{
  std::lock_guard<std::mutex> lock{m_identity_mutex};
  if (server_id == m_current_server_id && player_hash == m_current_player_hash && signature == m_current_signature && head_scale == m_current_head_scale)
  {
    return;
  }

  m_current_server_id = server_id;
  m_current_player_hash = player_hash;
  m_current_signature = signature;
  m_current_head_scale = head_scale;

  if (server_id.empty() || player_hash.empty() || signature.empty())
  {
    return;
  }

  send_line("{\"type\":\"hello\",\"server_id\":\"" + escape_string(server_id) +
            "\",\"player_hash\":\"" + escape_string(player_hash) + 
            "\",\"signature\":\"" + escape_string(signature) + 
            "\",\"head_scale\":" + std::to_string(head_scale) + "}");
}

void identify_client::send_chat(std::string_view message)
{
  if (message.empty() || message.size() > 128)
  {
    return;
  }

  send_line("{\"type\":\"chat\",\"msg\":\"" + escape_string(message) + "\"}");
}

void identify_client::close_socket()
{
  int s = m_socket.exchange(-1);
  if (s >= 0)
  {
    ::close(s);
  }
}

bool identify_client::dial()
{
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if (::getaddrinfo(m_host.c_str(), std::to_string(m_port).c_str(), &hints, &res) != 0)
  {
    return false;
  }

  int s = -1;
  for (auto* p = res; p != nullptr; p = p->ai_next)
  {
    s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (s < 0)
    {
      continue;
    }

    if (::connect(s, p->ai_addr, p->ai_addrlen) == 0)
    {
      break;
    }

    ::close(s);
    s = -1;
  }

  ::freeaddrinfo(res);

  if (s < 0)
  {
    return false;
  }

  m_socket = s;
  return true;
}

void identify_client::worker_loop()
{
  std::string buf;
  std::mt19937 rng{static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count())};
  std::uniform_int_distribution<int> jitter_dial(0, 5000); // 0-5s
  std::uniform_int_distribution<int> jitter_loop(0, 3000); // 0-3s

  while (m_running)
  {
    if (m_socket < 0 && !dial())
    {
      std::unique_lock<std::mutex> lock{m_identity_mutex};
      m_cv.wait_for(lock, std::chrono::milliseconds(5000 + jitter_dial(rng)), [this]() {
        return !m_running;
      });
      continue;
    }

    {
      std::lock_guard<std::mutex> lock{m_identity_mutex};
      if (!m_current_server_id.empty() && !m_current_player_hash.empty() && !m_current_signature.empty())
      {
        send_line("{\"type\":\"hello\",\"server_id\":\"" + escape_string(m_current_server_id) +
                  "\",\"player_hash\":\"" + escape_string(m_current_player_hash) + 
                  "\",\"signature\":\"" + escape_string(m_current_signature) + 
                  "\",\"head_scale\":" + std::to_string(m_current_head_scale) + "}");
      }
    }

    char chunk[1024];
    while (m_running)
    {
      int s = m_socket;
      if (s < 0)
      {
        break;
      }

      ssize_t n = ::recv(s, chunk, sizeof(chunk), 0);
      if (n <= 0)
      {
        break;
      }

      buf.append(chunk, static_cast<size_t>(n));
      size_t pos = 0;
      while ((pos = buf.find('\n')) != std::string::npos)
      {
        handle_line(buf.substr(0, pos));
        buf.erase(0, pos + 1);
      }
    }

    close_socket();
    buf.clear();

    if (m_running)
    {
      std::unique_lock<std::mutex> lock{m_identity_mutex};
      m_cv.wait_for(lock, std::chrono::milliseconds(2000 + jitter_loop(rng)), [this]() {
        return !m_running;
      });
    }
  }
}

void identify_client::send_line(std::string line)
{
  std::lock_guard<std::mutex> lock{m_send_mutex};
  int s = m_socket;
  if (s < 0)
  {
    return;
  }

  line += '\n';
  size_t offset = 0;
  while (offset < line.size())
  {
    ssize_t n = ::send(s, line.data() + offset, line.size() - offset, MSG_NOSIGNAL);
    if (n <= 0)
    {
      close_socket();
      return;
    }
    offset += static_cast<size_t>(n);
  }
}

std::string identify_client::escape_string(std::string_view value)
{
  std::string out;
  out.reserve(value.size() + 4);
  for (char c : value)
  {
    if (c == '"' || c == '\\')
    {
      out += '\\';
      out += c;
    }
    else if (static_cast<unsigned char>(c) >= 0x20)
    {
      out += c;
    }
  }
  return out;
}

std::string identify_client::find_json_value(std::string_view json, std::string_view key)
{
  std::string needle = "\"" + std::string{key} + "\"";
  size_t pos = json.find(needle);
  if (pos == std::string_view::npos)
  {
    return {};
  }

  pos = json.find(':', pos + needle.size());
  if (pos == std::string_view::npos)
  {
    return {};
  }

  ++pos;
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
  {
    ++pos;
  }

  if (pos >= json.size())
  {
    return {};
  }

  if (json[pos] == '"')
  {
    ++pos;
    std::string out;
    while (pos < json.size() && json[pos] != '"')
    {
      if (json[pos] == '\\' && pos + 1 < json.size())
      {
        out += json[pos + 1];
        pos += 2;
      }
      else
      {
        out += json[pos++];
      }
    }
    return out;
  }

  if (json[pos] == '[')
  {
    size_t start = pos;
    int depth = 0;
    for (; pos < json.size(); ++pos)
    {
      if (json[pos] == '[')
      {
        ++depth;
      }
      else if (json[pos] == ']')
      {
        if (--depth == 0)
        {
          return std::string{json.substr(start, pos - start + 1)};
        }
      }
    }
  }

  return {};
}

std::vector<std::string> identify_client::parse_string_array(std::string_view array_str)
{
  std::vector<std::string> out;
  for (size_t i = 0; i < array_str.size();)
  {
    if (array_str[i] == '"')
    {
      ++i;
      std::string s;
      while (i < array_str.size() && array_str[i] != '"')
      {
        if (array_str[i] == '\\' && i + 1 < array_str.size())
        {
          s += array_str[i + 1];
          i += 2;
        }
        else
        {
          s += array_str[i++];
        }
      }
      out.push_back(std::move(s));
      if (i < array_str.size())
      {
        ++i;
      }
    }
    else
    {
      ++i;
    }
  }
  return out;
}

void identify_client::handle_line(std::string_view line)
{
  std::string type = find_json_value(line, "type");
  if (type == "peers")
  {
    auto hashes = parse_string_array(find_json_value(line, "hashes"));
    if (m_peers_callback)
    {
      m_peers_callback(hashes);
    }
  }
  else if (type == "chat")
  {
    std::string from = find_json_value(line, "from");
    std::string msg = find_json_value(line, "msg");
    if (m_chat_callback)
    {
      m_chat_callback(from, msg);
    }
  }
}

} // namespace cathook::core::identify
