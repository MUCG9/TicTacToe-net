add_library(config_parser SHARED parser.cpp)
target_include_directories(config_parser PUBLIC ${PROJECT_SOURCE_DIR}/include)