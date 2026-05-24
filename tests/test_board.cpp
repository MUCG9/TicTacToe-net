#include <gtest/gtest.h>
#include "core/board.h"

using namespace ttt::core;

// 1. Проверка начального состояния
TEST(BoardTest, InitialState) {
    Board b;
    EXPECT_EQ(b.check_state(), State::InProgress);
    EXPECT_TRUE(b.is_cell_empty(0, 0));
    EXPECT_TRUE(b.is_cell_empty(2, 2));
}

// 2. Валидный ход
TEST(BoardTest, ValidMove) {
    Board b;
    EXPECT_TRUE(b.make_move(1, 1, Player::X));
    EXPECT_FALSE(b.is_cell_empty(1, 1));
    EXPECT_EQ(b.check_state(), State::InProgress);
}

// 3. Попытка занять занятую клетку
TEST(BoardTest, MoveOnOccupiedCell) {
    Board b;
    b.make_move(0, 0, Player::X);
    EXPECT_FALSE(b.make_move(0, 0, Player::O));
}

// 4. Выход за границы поля
TEST(BoardTest, OutOfBoundsMove) {
    Board b;
    EXPECT_THROW(b.make_move(-1, 0, Player::X), std::out_of_range);
    EXPECT_THROW(b.make_move(3, 3, Player::X), std::out_of_range);
    EXPECT_THROW(b.make_move(0, 5, Player::O), std::out_of_range);
}

// 5. Победа по горизонтали
TEST(BoardTest, WinHorizontal) {
    Board b;
    b.make_move(0, 0, Player::X);
    b.make_move(1, 0, Player::O);
    b.make_move(0, 1, Player::X);
    b.make_move(1, 1, Player::O);
    b.make_move(0, 2, Player::X);
    EXPECT_EQ(b.check_state(), State::Win_X);
}

// 6. Победа по вертикали
TEST(BoardTest, WinVertical) {
    Board b;
    b.make_move(0, 0, Player::O);
    b.make_move(0, 1, Player::X);
    b.make_move(1, 0, Player::O);
    b.make_move(1, 1, Player::X);
    b.make_move(2, 0, Player::O);
    EXPECT_EQ(b.check_state(), State::Win_O);
}

// 7. Победа по диагонали
TEST(BoardTest, WinDiagonal) {
    Board b;
    b.make_move(0, 0, Player::X);
    b.make_move(0, 1, Player::O);
    b.make_move(1, 1, Player::X);
    b.make_move(1, 0, Player::O);
    b.make_move(2, 2, Player::X);
    EXPECT_EQ(b.check_state(), State::Win_X);
}

// 8. Ничья (исправленная последовательность ходов)
TEST(BoardTest, Draw) {
    Board b;
    // X O X
    // X X O
    // O X O
    b.make_move(0,0,Player::X); b.make_move(0,1,Player::O); b.make_move(0,2,Player::X);
    b.make_move(1,0,Player::X); b.make_move(1,1,Player::X); b.make_move(1,2,Player::O);
    b.make_move(2,0,Player::O); b.make_move(2,1,Player::X); b.make_move(2,2,Player::O);
    
    EXPECT_EQ(b.check_state(), State::Draw);
}

// 9. Сериализация доски (to_string)
TEST(BoardTest, ToStringFormat) {
    Board b;
    b.make_move(0, 0, Player::X);
    b.make_move(2, 2, Player::O);
    std::string state = b.to_string();
    EXPECT_EQ(state.length(), 9);
    EXPECT_EQ(state[0], 'X');
    EXPECT_EQ(state[8], 'O');
    EXPECT_EQ(state[4], '.');
}