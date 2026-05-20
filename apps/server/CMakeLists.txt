add_executable(ttt_server main.cpp)
target_link_libraries(ttt_server PRIVATE game_core net_proto config_parser)
install(TARGETS ttt_server DESTINATION bin)