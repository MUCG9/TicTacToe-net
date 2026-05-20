#pragma once
#include <string>
#include <memory>
#include <stdexcept>

namespace ttt::net {

class TcpSocket {
public:
    TcpSocket();
    ~TcpSocket();
    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;

    void bind(const std::string& address, int port);
    void listen(int backlog = 1);
    [[nodiscard]] std::unique_ptr<TcpSocket> accept();
    void connect(const std::string& address, int port);
    void send_data(const std::string& data);
    [[nodiscard]] std::string recv_data(size_t max_len = 4096);

private:
    int fd_ = -1;
    void set_non_blocking(bool enable);
    void handle_error(const std::string& msg);
};

} // namespace ttt::net