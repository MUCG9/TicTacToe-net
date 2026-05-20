#include "core/board.h"
#include <algorithm>

namespace ttt::core {

Board::Board() {
    for (auto& row : grid_)
        std::fill(row.begin(), row.end(), Cell::Empty);
}

bool Board::make_move(int row, int col, Player player) {
    if (row < 0 || row >= 3 || col < 0 || col >= 3)
        throw std::out_of_range("Move coordinates out of bounds [0-2]");
    if (!is_cell_empty(row, col))
        return false;
    grid_[row][col] = (player == Player::X) ? Cell::X : Cell::O;
    return true;
}

State Board::check_state() const {
    if (check_lines(Player::X)) return State::Win_X;
    if (check_lines(Player::O)) return State::Win_O;
    bool full = std::all_of(grid_.begin(), grid_.end(),
        [](const auto& row) { return std::all_of(row.begin(), row.end(),
            [](Cell c) { return c != Cell::Empty; }); });
    return full ? State::Draw : State::InProgress;
}

std::string Board::to_string() const {
    std::string res;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            char c = '.';
            if (grid_[i][j] == Cell::X) c = 'X';
            else if (grid_[i][j] == Cell::O) c = 'O';
            res += c;
        }
    }
    return res; // Теперь возвращает "X..O..X.." (9 символов)
}

bool Board::is_cell_empty(int row, int col) const {
    return grid_[row][col] == Cell::Empty;
}

bool Board::check_lines(Player p) const {
    Cell target = (p == Player::X) ? Cell::X : Cell::O;
    for (int i = 0; i < 3; ++i) {
        if (grid_[i][0] == target && grid_[i][1] == target && grid_[i][2] == target) return true;
        if (grid_[0][i] == target && grid_[1][i] == target && grid_[2][i] == target) return true;
    }
    if (grid_[0][0] == target && grid_[1][1] == target && grid_[2][2] == target) return true;
    if (grid_[0][2] == target && grid_[1][1] == target && grid_[2][0] == target) return true;
    return false;
}

} // namespace ttt::core