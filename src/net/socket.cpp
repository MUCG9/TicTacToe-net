#include "net/socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <system_error>

namespace ttt::net {

TcpSocket::TcpSocket() : fd_(socket(AF_INET, SOCK_STREAM, 0)) {
    if (fd_ < 0) handle_error("socket() failed");
}

TcpSocket::~TcpSocket() {
    if (fd_ >= 0) close(fd_);
}

void TcpSocket::bind(const std::string& address, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, address.c_str(), &addr.sin_addr) <= 0)
        throw std::invalid_argument("Invalid IP address");
    if (::bind(fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        handle_error("bind() failed");
}

void TcpSocket::listen(int backlog) {
    if (::listen(fd_, backlog) < 0) handle_error("listen() failed");
}

std::unique_ptr<TcpSocket> TcpSocket::accept() {
    sockaddr_in client_addr{};
    socklen_t len = sizeof(client_addr);
    int client_fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&client_addr), &len);
    if (client_fd < 0) handle_error("accept() failed");
    auto sock = std::make_unique<TcpSocket>();
    sock->fd_ = client_fd;
    return sock;
}

void TcpSocket::connect(const std::string& address, int port) {
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, address.c_str(), &server.sin_addr) <= 0)
        throw std::invalid_argument("Invalid server IP");
    if (::connect(fd_, reinterpret_cast<sockaddr*>(&server), sizeof(server)) < 0)
        handle_error("connect() failed");
}

void TcpSocket::send_data(const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        ssize_t n = ::send(fd_, data.c_str() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n <= 0) handle_error("send() failed");
        sent += n;
    }
}

std::string TcpSocket::recv_data(size_t) {
    std::string result;
    char ch;

    while (true) {
        ssize_t n = ::recv(fd_, &ch, 1, 0);

        if (n <= 0)
            handle_error("recv() failed or connection closed");

        if (ch == '\n')
            break;

        result += ch;
    }

    return result;
}

void TcpSocket::handle_error(const std::string& msg) {
    throw std::system_error(errno, std::generic_category(), msg);
}

} // namespace ttt::net