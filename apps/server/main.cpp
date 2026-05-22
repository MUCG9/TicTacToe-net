#include <iostream>
#include <signal.h>
#include <thread>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "core/board.h"
#include "net/socket.h"
#include "net/protocol.h"
#include "config/parser.h"

using namespace ttt;

struct GameSession {
    std::string room_id;
    std::unique_ptr<net::TcpSocket> p1_sock;
    std::unique_ptr<net::TcpSocket> p2_sock;
    std::string p1_name, p2_name;
    core::Board board;
    core::Player current = core::Player::X;
    std::mutex mtx;
};

std::mutex rooms_mtx;
std::unordered_map<std::string, std::shared_ptr<GameSession>> waiting_rooms;
std::unordered_map<std::string, std::shared_ptr<GameSession>> active_rooms;

void run_session(std::shared_ptr<GameSession> session) {
    try {
        session->p1_sock->send_data(net::Protocol::serialize({"YOU_ARE", {"X"}}));
        session->p1_sock->send_data(net::Protocol::serialize({"OPPONENT_NAME", {session->p2_name}}));
        session->p2_sock->send_data(net::Protocol::serialize({"YOU_ARE", {"O"}}));
        session->p2_sock->send_data(net::Protocol::serialize({"OPPONENT_NAME", {session->p1_name}}));
        
        session->p1_sock->send_data(net::Protocol::serialize({"ROOM_READY", {}}));
        session->p2_sock->send_data(net::Protocol::serialize({"ROOM_READY", {}}));

        auto init_state = net::Protocol::serialize(
            net::Protocol::make_state(session->board.to_string())
        );

        session->p1_sock->send_data(init_state);
        session->p2_sock->send_data(init_state);

        net::TcpSocket* clients[2] = {session->p1_sock.get(), session->p2_sock.get()};
        std::cout << "[Room " << session->room_id << "] Game started: " 
                  << session->p1_name << "(X) vs " << session->p2_name << "(O)\n";

        while (true) {
            int idx = (session->current == core::Player::X) ? 0 : 1;
            std::string raw = clients[idx]->recv_data();
            auto msg = net::Protocol::deserialize(raw);
            std::cout << "SERVER GOT: [" << raw << "]\n";

            if (msg.type == "MOVE" && msg.args.size() >= 2) {
                int r = std::stoi(msg.args[0]);
                int c = std::stoi(msg.args[1]);

                if (!session->board.make_move(r, c, session->current)) {
                    clients[idx]->send_data(net::Protocol::serialize(net::Protocol::make_error("Invalid move")));
                    continue;
                }

                auto state_msg = net::Protocol::serialize(net::Protocol::make_state(session->board.to_string()));
                clients[0]->send_data(state_msg);
                clients[1]->send_data(state_msg);

                auto state = session->board.check_state();
                if (state != core::State::InProgress) {
                    std::string res = (state == core::State::Draw) ? "DRAW" :
                                      (state == core::State::Win_X) ? "X_WIN" : "O_WIN";
                    auto res_msg = net::Protocol::serialize(net::Protocol::make_result(res));
                    clients[0]->send_data(res_msg);
                    clients[1]->send_data(res_msg);
                    std::cout << "[Room " << session->room_id << "] Finished: " << res << "\n";
                    break;
                }
                session->current = (session->current == core::Player::X) ? core::Player::O : core::Player::X;
            } 
            else if (msg.type == "RESTART") {
                session->board = core::Board();
                session->current = core::Player::X;
                auto state_msg = net::Protocol::serialize(net::Protocol::make_state(session->board.to_string()));
                clients[0]->send_data(state_msg);
                clients[1]->send_data(state_msg);
                std::cout << "[Room " << session->room_id << "] Restarted\n";
            }
        }
    } catch (const std::exception& e) {
        std::cout << "[Room " << session->room_id << "] Session closed: " << e.what() << "\n";
    }

    std::lock_guard<std::mutex> lock(rooms_mtx);
    active_rooms.erase(session->room_id);
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
        srv.listen(20);
        std::cout << "Lobby Server listening on " << bind_addr << ":" << port << "\n";

        while (true) {
            auto client = srv.accept();
            
            // ЗАЩИТА: Оборачиваем обработку клиента в try-catch
            try {
                // Читаем сообщение JOIN
                std::string raw = client->recv_data();
                if (raw.empty()) continue; // Если соединение разорвано сразу
                
                auto msg = net::Protocol::deserialize(raw);
                
                std::cout << "[Debug] Received: " << msg.type << " args: " << msg.args.size() << "\n";

                // ЗАЩИТА: Проверяем, есть ли аргументы, перед обращением к ним
                if (msg.type != "JOIN" || msg.args.size() < 2) {
                    std::string err = "Invalid JOIN format. Use: JOIN <room> <name> (got " + std::to_string(msg.args.size()) + " args)";
                    client->send_data(net::Protocol::serialize(net::Protocol::make_error(err)));
                    std::cout << "[Lobby] Invalid message received.\n";
                    continue;
                }

                std::string room_id = msg.args[0];
                std::string p_name;
                for (size_t i = 1; i < msg.args.size(); ++i) 
                    p_name += msg.args[i] + (i < msg.args.size() - 1 ? " " : "");

                std::lock_guard<std::mutex> lock(rooms_mtx);
                if (active_rooms.count(room_id)) {
                    client->send_data(net::Protocol::serialize(net::Protocol::make_error("Room is active or full")));
                    continue;
                }

                if (waiting_rooms.count(room_id)) {
                    auto session = waiting_rooms[room_id];
                    waiting_rooms.erase(room_id);
                    session->p2_sock = std::move(client);
                    session->p2_name = p_name;
                    active_rooms[room_id] = session;
                    
                    // Запускаем игру в отдельном потоке
                    std::thread t(run_session, session);
                    t.detach();
                } else {
                    auto session = std::make_shared<GameSession>();
                    session->room_id = room_id;
                    session->p1_sock = std::move(client);
                    session->p1_name = p_name;
                    waiting_rooms[room_id] = session;
                    session->p1_sock->send_data(net::Protocol::serialize({"WAITING", {}}));
                    std::cout << "[Lobby] Player " << p_name << " waiting in room " << room_id << "\n";
                }
            } 
            catch (const std::exception& e) {
                // Если клиент отключился или произошла ошибка сети, сервер не падает
                std::cout << "[Lobby] Error handling client: " << e.what() << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Server fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}