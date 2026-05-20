# Network Tic-Tac-Toe (TTT-Net)

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Qt6](https://img.shields.io/badge/Qt-6.x-blue.svg)](https://www.qt.io/)
[![CMake](https://img.shields.io/badge/CMake-3.14+-blue.svg)](https://cmake.org/)

Кроссплатформенная сетевая реализация игры "Крестики-нолики" на **C++17**.
Проект демонстрирует архитектуру клиент-сервер, использование разделяемых библиотек, асинхронную работу с сетью и современную систему сборки.

## Возможности

- **Графический интерфейс** на Qt6 Widgets (кроссплатформенный)
- **Сетевая игра** через TCP (поддержка игры по локальной сети и интернету)
- **Конфигурация без перекомпиляции** (CLI-аргументы, ENV, INI-файл)
- **Готов к упаковке** (автоматическая генерация `.deb` и `.rpm` пакетов)
- **Тестирование памяти** (поддержка AddressSanitizer, LeakSanitizer, Valgrind)
- **Модульная архитектура** (разделяемые библиотеки: `core`, `net`, `config`)
- **Переносимость** (сборка на Linux через CMake, подготовка под Windows)

---

## Требования

### Для сборки

| Компонент | Версия | Примечание |
|-----------|--------|------------|
| **C++ Compiler** | GCC 11+ / Clang 13+ | С поддержкой C++17 |
| **CMake** | ≥ 3.14 | Система сборки |
| **Qt6** | ≥ 6.2 | Модули: `Widgets`, `Network` |
| **Git** | Любая | Для клонирования репозитория |

## Установка зависимостей

**Fedora / RHEL:**
```
sudo dnf install cmake gcc-c++ make qt6-qtbase-devel git
```

**Ubuntu / Debian:**
```
sudo apt update
sudo apt install build-essential cmake libqt6widgets6 libqt6network6 qt6-base-dev git
```

**Arch Linux:**
```
sudo pacman -S base-devel cmake qt6-base git
```

## Быстрый старт

### 1. Клонируйте репозиторий

```
https://github.com/MUCG9/TicTacToe-net.git
cd ttt-net
```

### 2. Сборка проекта

```
# Создаём папку для сборки (out-of-source build)
mkdir build && cd build

# Конфигурация CMake (Release-режим)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Компиляция (используем все ядра процессора)
make -j$(nproc)
```

### 3. Запуск игры

**Вариант А: Игра против сервера (один ПК)**
```
# Терминал 1: Запуск сервера
./apps/server/ttt_server

# Терминал 2: Запуск графического клиента
./apps/client_gui/ttt_client_gui
```

**Вариант Б: Игра по сети (два ПК)**
```
# На ПК с сервером:
./apps/server/ttt_server server.port=9090

# На втором ПК (клиент):
./apps/client_gui/ttt_client_gui client.server_host=192.168.1.100
```
Замените `192.168.1.100` на реальный IP-адрес компьютера с сервером.

## Конфигурация

Проект поддерживает три уровня конфигурации (по убыванию приоритета):

1. Аргументы командной строки (наивысший приоритет)
2. Переменные окружения
3. Файл `config/default.conf` (базовые настройки)

**Параметры сервера (`ttt_server`)**
|Параметр|По умолчанию|Описание|
|-|-|-|
|server.port|9090|TCP-порт для прослушивания подключений|
|server.bind_address|0.0.0.0|IP-адрес интерфейса для привязки|
|general.log_level|info|Уровень логирования: `debug`, `info`, `warn`, `error`|


## Сборка пакетов (DEB / RPM)

Проект настроен для автоматической упаковки через CPack.

```
cd build

# Генерация пакетов (требует установленной сборки)
make package

# Или явный вызов:

cpack -G DEB   # Для Debian/Ubuntu
cpack -G RPM   # Для Fedora/RHEL
```

Результат появится в папке `build/`:

- `ttt-net-1.0.0-Linux.deb`
- `ttt-net-1.0.0-Linux.rpm`

**Установка:**
```
# Debian/Ubuntu
sudo dpkg -i ttt-net-1.0.0-Linux.deb

# Fedora/RHEL
sudo rpm -ivh ttt-net-1.0.0-Linux.rpm
```

После установки бинарники доступны в `$PATH`:
```
ttt_server
ttt_client_gui
```

## Тестирование и отладка

**Проверка утечек памяти (Valgrind)**
```
# Сборка с отладочной информацией
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./apps/server/ttt_server
```

**AddressSanitizer / LeakSanitizer**
```
# Сборка с санитайзерами
cmake .. -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
make

./apps/server/ttt_server  # Санитайзеры сработают автоматически при ошибках
```

**Статический анализ**
```
# Проверка форматирования кода
clang-format -i src/**/*. {cpp,h} apps/**/*.{cpp,h}

# Проверка предупреждений компилятора (включены как ошибки)
cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_WERROR=ON
make -j$(nproc)  # Сборка прервётся при любом предупреждении
```

## Структура проекта
```
ttt-net/
├── CMakeLists.txt          # Корневой файл сборки
├── .gitignore              # Исключения для Git
├── .clang-format           # Правила форматирования кода
├── config/
│   └── default.conf        # Файл конфигурации по умолчанию
├── include/                # Публичные заголовки
│   ├── core/
│   │   └── board.h         # Логика игрового поля
│   ├── net/
│   │   ├── socket.h        # Обёртка над BSD sockets
│   │   └── protocol.h      # Сетевой протокол (сериализация)
│   └── config/
│       └── parser.h        # Парсер INI-файлов
├── src/                    # Исходный код библиотек
│   ├── core/               # Реализация Board
│   ├── net/                # Реализация Socket и Protocol
│   └── config/             # Реализация ConfigParser
├── apps/                   # Точка входа приложений
│   ├── server/             # Серверная часть (main.cpp)
│   ├── client/             # Консольный клиент
│   └── client_gui/         # Графический клиент на Qt
├── docs/                   # Документация
│   └── README.md           # Этот файл
└── packaging/              # Настройки упаковки
    └── CPackConfig.cmake   # Конфигурация для DEB/RPM
```

## Сетевой протокол
```
Обмен данными происходит через текстовые сообщения в формате:
<COMMAND> <ARG1> <ARG2> ... \n
```

**Команды сервер → клиент**
|Команда|Аргументы|Описание|
|-|-|-|
|YOU_ARE|X или O|Назначение символа игроку|
|STATE|<9 символов>|Текущее состояние доски (строка 3x3)|
|RESULT|X_WIN, O_WIN, DRAW|Результат завершённой игры|
|ERROR|<текст>|Сообщение об ошибке|

**Команды клиент → сервер**
|Команда|Аргументы|Описание|
|-|-|-|
|MOVE|`<row> <col>`|Ход игрока (координаты 0-2)|

**Пример сессии**
```
S->C: YOU_ARE X
S->C: STATE .........
C->S: MOVE 1 1
S->C: STATE ....X....
S->C: STATE ..X..O...
...
S->C: RESULT X_WIN
```

## Разработка
**Ветвление (Git Flow)**
```
# Создание новой фичи
git checkout -b feature/new-feature

# После завершения работы
git add .
git commit -m "feat: описание изменений"
git checkout main
git merge feature/new-feature
```

**Добавление новой зависимости**

Если вы добавляете новую библиотеку:

1. Обновите `CMakeLists.txt` (find_package, target_link_libraries)
2. Добавьте зависимость в `docs/README.md` (раздел "Требования")
3. Обновите `packaging/CPackConfig.cmake` (если нужна системная зависимость)

## Авторы
Проект выполнен в рамках учебного курса по программированию на C++ в группе БО1-406.

- *Насыров Альмир:* Ядро игры, конфигурация, CLI-интерфейс, документация, упаковка
- *Дурандин Вячеслав:* Сетевой модуль, серверная часть, GUI на Qt, тестирование





