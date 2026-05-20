#include <iostream>
#include <signal.h>
#include <thread>
#include <deque>
#include <mutex>
#include <memory>
#include "core/board.h"
#include "net/socket.h"
#include "net/protocol.h"
#include "config/parser.h"

using namespace ttt;

// Очередь ожидающих игроков (потокобезопасная)
std::mutex queue_mutex;
std::deque<std::unique_ptr<net::TcpSocket>> waiting_queue;

// Функция, выполняющаяся в отдельном потоке для каждой пары игроков
void run_game_session(std::unique_ptr<net::TcpSocket> p1, std::unique_ptr<net::TcpSocket> p2) {
    try {
        // Назначаем роли
        p1->send_data(net::Protocol::serialize({"YOU_ARE", {"X"}}));
        p2->send_data(net::Protocol::serialize({"YOU_ARE", {"O"}}));

        core::Board board;
        core::Player current = core::Player::X;
        net::TcpSocket* clients[2] = {p1.get(), p2.get()};

        std::cout << "[Session " << std::this_thread::get_id() << "] Game started: X vs O\n";

        while (true) {
            int idx = (current == core::Player::X) ? 0 : 1;
            auto& sock = *clients[idx];

            // Блокирующее чтение хода
            std::string raw = sock.recv_data();
            auto msg = net::Protocol::deserialize(raw);

            if (msg.type == "MOVE" && msg.args.size() >= 2) {
                int r = std::stoi(msg.args[0]);
                int c = std::stoi(msg.args[1]);

                if (!board.make_move(r, c, current)) {
                    sock.send_data(net::Protocol::serialize(net::Protocol::make_error("Invalid move")));
                    continue;
                }

                // Рассылаем состояние обоим игрокам
                auto state_msg = net::Protocol::serialize(net::Protocol::make_state(board.to_string()));
                clients[0]->send_data(state_msg);
                clients[1]->send_data(state_msg);

                // Проверка конца игры
                auto state = board.check_state();
                if (state != core::State::InProgress) {
                    std::string res = (state == core::State::Draw) ? "DRAW" :
                                      (state == core::State::Win_X) ? "X_WIN" : "O_WIN";
                    auto res_msg = net::Protocol::serialize(net::Protocol::make_result(res));
                    clients[0]->send_data(res_msg);
                    clients[1]->send_data(res_msg);
                    std::cout << "[Session " << std::this_thread::get_id() << "] Finished: " << res << "\n";
                    break;
                }

                current = (current == core::Player::X) ? core::Player::O : core::Player::X;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "[Session " << std::this_thread::get_id() << "] Closed: " << e.what() << "\n";
    }
    // unique_ptr автоматически закроет сокеты при выходе из функции (RAII)
}

int main(int argc, char* argv[]) {
    config::Parser cfg;
    try { cfg.load_file("config/default.conf"); }
    catch (const std::exception& e) { std::cerr << "Config warning: " << e.what() << "\n"; }
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t eq = arg.find('=');
        if (eq != std::string::npos) cfg.set_override(arg.substr(0, eq), arg.substr(eq + 1));
    }

    int port = std::stoi(cfg.get("server.port", "9090"));
    std::string bind_addr = cfg.get("server.bind_address", "0.0.0.0");

    signal(SIGPIPE, SIG_IGN);

    try {
        net::TcpSocket srv;
        srv.bind(bind_addr, port);
        srv.listen(5);
        std::cout << "Server listening on " << bind_addr << ":" << port << "\n";
        std::cout << "Matchmaking active. Connect 2 clients to start a game.\n";

        // Бесконечный цикл принятия подключений
        while (true) {
            auto client = srv.accept();
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                waiting_queue.push_back(std::move(client));
                std::cout << "[Queue] Players waiting: " << waiting_queue.size() << "\n";

                // Если есть пара → запускаем игру в новом потоке
                if (waiting_queue.size() >= 2) {
                    auto p1 = std::move(waiting_queue.front()); waiting_queue.pop_front();
                    auto p2 = std::move(waiting_queue.front()); waiting_queue.pop_front();

                    std::thread game_thread(run_game_session, std::move(p1), std::move(p2));
                    game_thread.detach(); // Поток живёт независимо от сервера
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Server fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}