add_executable(ttt_client_gui
    main.cpp
    game_window.cpp
    game_window.h
    connection_dialog.cpp
    connection_dialog.h
)
target_link_libraries(ttt_client_gui PRIVATE
    Qt6::Widgets
    Qt6::Network
    game_core
    net_proto
    config_parser
)
install(TARGETS ttt_client_gui DESTINATION bin)