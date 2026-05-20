#include <iostream>
#include <signal.h>
#include <thread>
#include <vector>
#include <mutex>
#include "core/board.h"
#include "net/socket.h"
#include "net/protocol.h"
#include "config/parser.h"

struct Client {
    std::unique_ptr<ttt::net::TcpSocket> socket;
    ttt::core::Player player;
    bool ready = false;
};

int main(int argc, char* argv[]) {
    ttt::config::Parser cfg;
    try { cfg.load_file("config/default.conf"); }
    catch (const std::exception& e) { std::cerr << "Config warning: " << e.what() << "\n"; }
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t eq = arg.find('=');
        if (eq != std::string::npos) 
            cfg.set_override(arg.substr(0, eq), arg.substr(eq + 1));
    }

    int port = std::stoi(cfg.get("server.port", "9090"));
    std::string bind_addr = cfg.get("server.bind_address", "0.0.0.0");

    signal(SIGPIPE, SIG_IGN);
    
    try {
        ttt::net::TcpSocket srv;
        srv.bind(bind_addr, port);
        srv.listen(2);
        std::cout << "Server listening on " << bind_addr << ":" << port << "\n";
        std::cout << "Waiting for 2 players...\n";

        // Ждём двух клиентов
        std::vector<Client> clients;
        for (int i = 0; i < 2; ++i) {
            auto client = srv.accept();
            ttt::core::Player p = (i == 0) ? ttt::core::Player::X : ttt::core::Player::O;
            clients.push_back({std::move(client), p, false});
            std::cout << "Player " << (p == ttt::core::Player::X ? "X" : "O") 
                      << " connected (" << (i+1) << "/2)\n";
        }
        
        std::cout << "Game started! Player X goes first.\n";

        ttt::core::Board board;
        ttt::core::Player current = ttt::core::Player::X;
        
        // Отправляем каждому игроку его символ и начальное состояние
        for (auto& cl : clients) {
            std::string symbol = (cl.player == ttt::core::Player::X) ? "X" : "O";
            cl.socket->send_data(ttt::net::Protocol::serialize(
                {"YOU_ARE", {symbol}}));
            cl.socket->send_data(ttt::net::Protocol::serialize(
                ttt::net::Protocol::make_state(board.to_string())));
        }

        // Игровой цикл
        while (true) {
            // Находим текущего игрока
            int current_idx = (current == ttt::core::Player::X) ? 0 : 1;
            auto& current_client = clients[current_idx];
            
            // Ждём ход от текущего игрока
            try {
                auto raw = current_client.socket->recv_data();
                auto msg = ttt::net::Protocol::deserialize(raw);
                
                if (msg.type == "MOVE") {
                    int r = std::stoi(msg.args.at(0));
                    int c = std::stoi(msg.args.at(1));
                    
                    if (!board.make_move(r, c, current)) {
                        current_client.socket->send_data(
                            ttt::net::Protocol::serialize(
                                ttt::net::Protocol::make_error("Invalid move")));
                        continue;
                    }
                    
                    // Отправляем новое состояние ВСЕМ клиентам
                    auto state_msg = ttt::net::Protocol::make_state(board.to_string());
                    for (auto& cl : clients) {
                        cl.socket->send_data(
                            ttt::net::Protocol::serialize(state_msg));
                    }
                    
                    // Проверяем конец игры
                    auto state = board.check_state();
                    if (state != ttt::core::State::InProgress) {
                        std::string result;
                        if (state == ttt::core::State::Draw) result = "DRAW";
                        else if (state == ttt::core::State::Win_X) result = "X_WINS";
                        else result = "O_WINS";
                        
                        for (auto& cl : clients) {
                            cl.socket->send_data(
                                ttt::net::Protocol::serialize(
                                    ttt::net::Protocol::make_result(result)));
                        }
                        std::cout << "Game over: " << result << "\n";
                        break;
                    }
                    
                    // Передаём ход
                    current = (current == ttt::core::Player::X) ? 
                              ttt::core::Player::O : ttt::core::Player::X;
                    std::cout << "Turn: " << (current == ttt::core::Player::X ? "X" : "O") << "\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                break;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}