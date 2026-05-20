add_executable(ttt_client main.cpp)
target_link_libraries(ttt_client PRIVATE game_core net_proto config_parser)
install(TARGETS ttt_client DESTINATION bin)