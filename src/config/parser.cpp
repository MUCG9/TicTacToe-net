#include "config/parser.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace ttt::config {

Parser::Parser() = default;

void Parser::load_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open config: " + path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto pos = line.find('=');
        if (pos == std::string::npos) continue;
        auto key = line.substr(0, pos);
        auto val = line.substr(pos + 1);
        // Trim
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        val.erase(0, val.find_first_not_of(" \t"));
        val.erase(val.find_last_not_of(" \t") + 1);
        data_[key] = val;
    }
}

void Parser::set_override(const std::string& key, const std::string& val) {
    data_[key] = val;
}

std::string Parser::get(const std::string& key, const std::string& default_val) const {
    auto it = data_.find(key);
    return (it != data_.end()) ? it->second : default_val;
}

} // namespace ttt::config