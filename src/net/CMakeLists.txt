add_library(net_proto SHARED socket.cpp protocol.cpp)
target_include_directories(net_proto PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(net_proto PUBLIC game_core)
set_target_properties(net_proto PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION 1)