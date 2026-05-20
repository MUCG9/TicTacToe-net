#include <iostream>
#include <string>
#include "core/board.h"
#include "net/socket.h"
#include "net/protocol.h"
#include "config/parser.h"

int main(int argc, char* argv[]) {
    ttt::config::Parser cfg;
    try { cfg.load_file("config/default.conf"); }
    catch (const std::exception& e) { std::cerr << "Config warning: " << e.what() << "\n"; }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t eq = arg.find('=');
        if (eq != std::string::npos) cfg.set_override(arg.substr(0, eq), arg.substr(eq + 1));
    }

    std::string host = cfg.get("client.server_host", "127.0.0.1");
    int port = std::stoi(cfg.get("client.server_port", "9090"));

    try {
        ttt::net::TcpSocket cli;
        cli.connect(host, port);
        std::cout << "Connected to " << host << ":" << port << "\n";

        while (true) {
            auto raw = cli.recv_data();
            auto msg = ttt::net::Protocol::deserialize(raw);
            if (msg.type == "STATE") {
                std::cout << "\n--- Board ---\n" << msg.args.at(0) << "-------------\n";
                std::cout << "Your turn (row col): ";
                int r, c;
                if (!(std::cin >> r >> c)) {
                    std::cerr << "Invalid input\n";
                    std::cin.clear(); std::cin.ignore(1024, '\n');
                    continue;
                }
                cli.send_data(ttt::net::Protocol::serialize(ttt::net::Protocol::make_move(r, c)));
            } else if (msg.type == "RESULT") {
                std::cout << "Game ended: " << msg.args.at(0) << "\n";
                break;
            } else if (msg.type == "ERROR") {
                std::cout << "Error from server: " << msg.args.at(0) << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}