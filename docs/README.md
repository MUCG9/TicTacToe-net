# Network Tic-Tac-Toe (TTT-Net)

Кроссплатформенная сетевая реализация крестиков-ноликов на C++17.
Проект разделён на разделяемые библиотеки, поддерживает конфигурацию без перекомпиляции и готов к упаковке в DEB/RPM.

## Сборка
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)