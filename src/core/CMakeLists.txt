add_library(game_core SHARED board.cpp)
target_include_directories(game_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(game_core PUBLIC cxx_std_17)
set_target_properties(game_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)