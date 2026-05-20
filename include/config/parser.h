#pragma once
#include <string>
#include <unordered_map>

namespace ttt::config {

class Parser {
public:
    Parser();
    void load_file(const std::string& path);
    void set_override(const std::string& key, const std::string& val);
    [[nodiscard]] std::string get(const std::string& key, const std::string& default_val) const;

private:
    std::unordered_map<std::string, std::string> data_;
};

} // namespace ttt::config