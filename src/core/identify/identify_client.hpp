// warning this shit was written with ai lol

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <random>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

class IdentifyClient
{
public:
    using PeersCb = std::function<void(const std::vector<std::string>&)>;
    using ChatCb  = std::function<void(const std::string&, const std::string&)>;

    IdentifyClient() = default;
    ~IdentifyClient() { stop(); }

    IdentifyClient(const IdentifyClient&) = delete;
    IdentifyClient& operator=(const IdentifyClient&) = delete;

    void setPeersHandler(PeersCb cb) { peers_cb_ = std::move(cb); }
    void setChatHandler(ChatCb cb)   { chat_cb_  = std::move(cb); }

    void connect(std::string host, int port)
    {
        host_ = std::move(host);
        port_ = port;
        running_ = true;
        worker_ = std::thread([this] { workerLoop(); });
    }

    void stop()
    {
        running_ = false;
        closeSocket();
        if (worker_.joinable())
            worker_.join();
    }

    void updateIdentity(const std::string& server_id, const std::string& player_hash)
    {
        std::lock_guard<std::mutex> lk(id_mu_);
        if (server_id == cur_sid_ && player_hash == cur_phash_)
            return;
        cur_sid_   = server_id;
        cur_phash_ = player_hash;
        if (server_id.empty() || player_hash.empty())
            return;
        sendLine("{\"type\":\"hello\",\"server_id\":\"" + esc(server_id)
                 + "\",\"player_hash\":\"" + esc(player_hash) + "\"}");
    }

    void sendChat(const std::string& msg)
    {
        if (msg.empty() || msg.size() > 128)
            return;
        sendLine("{\"type\":\"chat\",\"msg\":\"" + esc(msg) + "\"}");
    }

private:
    std::string host_;
    int port_ = 0;
    std::atomic<int> sock_{-1};
    std::atomic<bool> running_{false};
    std::thread worker_;
    std::mutex send_mu_;
    std::mutex id_mu_;
    std::string cur_sid_, cur_phash_;
    PeersCb peers_cb_;
    ChatCb chat_cb_;

    void closeSocket()
    {
        int s = sock_.exchange(-1);
        if (s >= 0)
            ::close(s);
    }

    bool dial()
    {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family   = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        if (::getaddrinfo(host_.c_str(), std::to_string(port_).c_str(), &hints, &res) != 0)
            return false;
        int s = -1;
        for (auto* p = res; p; p = p->ai_next)
        {
            s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (s < 0)
                continue;
            if (::connect(s, p->ai_addr, p->ai_addrlen) == 0)
                break;
            ::close(s);
            s = -1;
        }
        ::freeaddrinfo(res);
        if (s < 0)
            return false;
        sock_ = s;
        return true;
    }

    void workerLoop()
    {
        std::string buf;
        std::mt19937 rng{ (uint32_t) std::chrono::steady_clock::now().time_since_epoch().count() };
        std::uniform_int_distribution<int> jitter_dial(0, 5000);  // 0-5s extra
        std::uniform_int_distribution<int> jitter_loop(0, 3000);  // 0-3s extra

        while (running_)
        {
            if (sock_ < 0 && !dial())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(5000 + jitter_dial(rng)));
                continue;
            }
            {
                std::lock_guard<std::mutex> lk(id_mu_);
                if (!cur_sid_.empty() && !cur_phash_.empty())
                    sendLine("{\"type\":\"hello\",\"server_id\":\"" + esc(cur_sid_)
                             + "\",\"player_hash\":\"" + esc(cur_phash_) + "\"}");
            }
            char chunk[1024];
            while (running_)
            {
                int s = sock_;
                if (s < 0)
                    break;
                ssize_t n = ::recv(s, chunk, sizeof(chunk), 0);
                if (n <= 0)
                    break;
                buf.append(chunk, (size_t) n);
                size_t pos;
                while ((pos = buf.find('\n')) != std::string::npos)
                {
                    handleLine(buf.substr(0, pos));
                    buf.erase(0, pos + 1);
                }
            }
            closeSocket();
            buf.clear();
            if (running_)
                std::this_thread::sleep_for(std::chrono::milliseconds(2000 + jitter_loop(rng)));
        }
    }

    void sendLine(std::string line)
    {
        std::lock_guard<std::mutex> lk(send_mu_);
        int s = sock_;
        if (s < 0)
            return;
        line += '\n';
        size_t off = 0;
        while (off < line.size())
        {
            ssize_t n = ::send(s, line.data() + off, line.size() - off, MSG_NOSIGNAL);
            if (n <= 0)
            {
                closeSocket();
                return;
            }
            off += (size_t) n;
        }
    }

    static std::string esc(const std::string& s)
    {
        std::string out;
        out.reserve(s.size() + 4);
        for (char c : s)
        {
            if (c == '"' || c == '\\') { out += '\\'; out += c; }
            else if ((unsigned char) c >= 0x20) { out += c; }
        }
        return out;
    }

    static std::string findValue(const std::string& s, const std::string& key)
    {
        std::string needle = "\"" + key + "\"";
        size_t p = s.find(needle);
        if (p == std::string::npos) return "";
        p = s.find(':', p + needle.size());
        if (p == std::string::npos) return "";
        ++p;
        while (p < s.size() && (s[p] == ' ' || s[p] == '\t')) ++p;
        if (p >= s.size()) return "";

        if (s[p] == '"')
        {
            ++p;
            std::string out;
            while (p < s.size() && s[p] != '"')
            {
                if (s[p] == '\\' && p + 1 < s.size()) { out += s[p + 1]; p += 2; }
                else { out += s[p++]; }
            }
            return out;
        }
        if (s[p] == '[')
        {
            size_t start = p;
            int depth = 0;
            for (; p < s.size(); ++p)
            {
                if (s[p] == '[') ++depth;
                else if (s[p] == ']') { if (--depth == 0) return s.substr(start, p - start + 1); }
            }
        }
        return "";
    }

    static std::vector<std::string> parseStringArray(const std::string& arr)
    {
        std::vector<std::string> out;
        for (size_t i = 0; i < arr.size();)
        {
            if (arr[i] == '"')
            {
                ++i;
                std::string s;
                while (i < arr.size() && arr[i] != '"')
                {
                    if (arr[i] == '\\' && i + 1 < arr.size()) { s += arr[i + 1]; i += 2; }
                    else { s += arr[i++]; }
                }
                out.push_back(std::move(s));
                if (i < arr.size()) ++i;
            }
            else
            {
                ++i;
            }
        }
        return out;
    }

    void handleLine(const std::string& line)
    {
        std::string type = findValue(line, "type");
        if (type == "peers")
        {
            auto hashes = parseStringArray(findValue(line, "hashes"));
            if (peers_cb_) peers_cb_(hashes);
        }
        else if (type == "chat")
        {
            std::string from = findValue(line, "from");
            std::string msg  = findValue(line, "msg");
            if (chat_cb_) chat_cb_(from, msg);
        }
    }
};
