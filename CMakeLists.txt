cmake_minimum_required(VERSION 3.14)
project(ttt_net VERSION 1.0.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ... остальной CMake без изменений ...
add_subdirectory(apps/client_gui)  # <-- новая папка

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(ENABLE_SANITIZERS "Enable AddressSanitizer & LeakSanitizer" OFF)
option(ENABLE_WERROR     "Treat warnings as errors" ON)

if(ENABLE_WERROR)
    add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion)
endif()

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,leak -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,leak)
endif()

# Модули как разделяемые библиотеки
add_subdirectory(src/core)
add_subdirectory(src/net)
add_subdirectory(src/config)

# Приложения
add_subdirectory(apps/server)
add_subdirectory(apps/client)

# Документация и установка
install(DIRECTORY config/ DESTINATION share/ttt-net)
install(FILES docs/README.md DESTINATION share/doc/ttt-net)

include(packaging/CPackConfig.cmake)