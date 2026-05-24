add_executable(board_test test_board.cpp)
target_link_libraries(board_test PRIVATE game_core GTest::gtest_main)
target_include_directories(board_test PRIVATE ${PROJECT_SOURCE_DIR}/include)

# Генераторное выражение для надёжной регистрации в CTest
add_test(NAME BoardUnitTests COMMAND $<TARGET_FILE:board_test>)