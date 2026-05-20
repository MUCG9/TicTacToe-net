#pragma once
#include <array>
#include <string>
#include <optional>
#include <stdexcept>

namespace ttt::core {

enum class Cell { Empty, X, O };
enum class Player { X, O };
enum class State { InProgress, Win_X, Win_O, Draw };

class Board {
public:
    Board();
    bool make_move(int row, int col, Player player);
    [[nodiscard]] State check_state() const;
    [[nodiscard]] std::string to_string() const;
    [[nodiscard]] bool is_cell_empty(int row, int col) const;

private:
    std::array<std::array<Cell, 3>, 3> grid_{};
    [[nodiscard]] bool check_lines(Player p) const;
};

} // namespace ttt::core