#pragma once
#include <string>
#include <vector>

namespace ttt::net {

struct Message {
    std::string type;
    std::vector<std::string> args;
};

class Protocol {
public:
    [[nodiscard]] static std::string serialize(const Message& msg);
    [[nodiscard]] static Message deserialize(const std::string& raw);
    [[nodiscard]] static Message make_move(int r, int c);
    [[nodiscard]] static Message make_state(const std::string& board);
    [[nodiscard]] static Message make_result(const std::string& res);
    [[nodiscard]] static Message make_error(const std::string& msg);
};

} // namespace ttt::net