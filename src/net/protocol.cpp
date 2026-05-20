#include "net/protocol.h"
#include <sstream>
#include <algorithm>

namespace ttt::net {

std::string Protocol::serialize(const Message& msg) {
    std::string res = msg.type;
    for (const auto& a : msg.args) res += " " + a;
    return res + "\n";
}

Message Protocol::deserialize(const std::string& raw) {
    std::istringstream iss(raw);
    std::string type;
    iss >> type;
    Message msg{type, {}};
    std::string token;
    while (iss >> token) msg.args.push_back(token);
    return msg;
}

Message Protocol::make_move(int r, int c) { return {"MOVE", {std::to_string(r), std::to_string(c)}}; }
Message Protocol::make_state(const std::string& board) { return {"STATE", {board}}; }
Message Protocol::make_result(const std::string& res) { return {"RESULT", {res}}; }
Message Protocol::make_error(const std::string& msg) { return {"ERROR", {msg}}; }

} // namespace ttt::net