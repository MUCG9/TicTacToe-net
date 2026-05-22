#include "game_window.h"
#include "net/protocol.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QApplication>

GameWindow::GameWindow(QTcpSocket* socket, const QString& opponentName,
                       ttt::core::Player symbol, QWidget* parent)
    : QMainWindow(parent), socket_(socket), 
      isMyTurn_(symbol == ttt::core::Player::X), // X всегда ходит первым
      mySymbol_(symbol),
      moveCount_(0) { // <-- Инициализация
    setupUI();
    infoLabel_->setText("Противник: " + opponentName);
    statusLabel_->setText(isMyTurn_ ? "Ваш ход (X)" : "Ход противника (O)");
    setWindowTitle("mipt.ttt");

    connect(socket_, &QTcpSocket::disconnected, this, &GameWindow::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &GameWindow::onReadyRead);
}

GameWindow::~GameWindow() {
    if (socket_) socket_->deleteLater();
}

void GameWindow::setupUI() {
    this->setStyleSheet(
        "QMainWindow { background-color: #0000CD; color: white; }"
        "QLabel { color: white; font-weight: bold; font-size: 14px; }"
        "QPushButton { background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial'; }"
        "QPushButton:hover { background-color: #E6E6FA; border: 2px solid white; }"
        "QPushButton:disabled { background-color: #F0F8FF; color: #A9A9A9; border: 2px solid #DCDCDC; }"
    );

    auto central = new QWidget(this);
    setCentralWidget(central);
    auto mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    infoLabel_ = new QLabel("Загрузка...", this);
    infoLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel_);

    statusLabel_ = new QLabel("Подключение...", this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel_);

    auto grid = new QGridLayout();
    grid->setSpacing(15);
    for (int r = 0; r < 3; ++r) {
        std::vector<QPushButton*> row;
        for (int c = 0; c < 3; ++c) {
            auto btn = new QPushButton("", this);
            btn->setFixedSize(100, 100);
            connect(btn, &QPushButton::clicked, this, [this, r, c]() { onCellClicked(r, c); });
            grid->addWidget(btn, r, c);
            row.push_back(btn);
        }
        cells_.push_back(std::move(row));
    }
    mainLayout->addLayout(grid);

    restartBtn_ = new QPushButton("🔄 Новая игра", this);
    restartBtn_->setFixedHeight(40);
    restartBtn_->setStyleSheet("QPushButton { background-color: #FFFFFF; color: #0000CD; font: bold 16px; border-radius: 5px; }");
    connect(restartBtn_, &QPushButton::clicked, this, &GameWindow::onRestartClicked);
    mainLayout->addWidget(restartBtn_);

    setFixedSize(360, 480);
}

void GameWindow::onDisconnected() {
    statusLabel_->setText("Сервер отключился");
    restartBtn_->setEnabled(false);
    for (auto& row : cells_) for (auto btn : row) btn->setEnabled(false);
}

void GameWindow::onReadyRead() {
    QString data = QString::fromUtf8(socket_->readAll());
    qDebug() << "CLIENT RECEIVED:" << data;
    QStringList msgs = data.split('\n', Qt::SkipEmptyParts);
    for (const QString& msg : msgs) {
        parseServerMessage(msg.trimmed());
    }
}

void GameWindow::parseServerMessage(const QString& raw) {
    auto msg = ttt::net::Protocol::deserialize(raw.toStdString());
    
    if (msg.type == "STATE") {
        applyBoardState(QString::fromStdString(msg.args.empty() ? "" : msg.args[0]));
        bool myTurnNow = (mySymbol_ == ttt::core::Player::X) ? isMyTurn_ : !isMyTurn_;
        statusLabel_->setText(myTurnNow ? "Ваш ход" : "Ход противника");
    } else if (msg.type == "ERROR") {
        statusLabel_->setText("Ошибка: " + QString::fromStdString(msg.args[0]));
    } else if (msg.type == "RESULT") {
        QString res = QString::fromStdString(msg.args[0]);
        QString message = (res == "DRAW") ? "Ничья!" : 
                          (res == "X_WIN") ? "Победили КРЕСТИКИ!" : "Победили НОЛИКИ!";
        
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Результат");
        msgBox.setText(message);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet("QMessageBox{background:#F0F8FF;font-size:16px;} QPushButton{background:#0000CD;color:white;border-radius:5px;padding:5px 15px;}");
        msgBox.exec();
        
        statusLabel_->setText("Игра завершена. Нажмите 'Новая игра'");
    }
}

void GameWindow::applyBoardState(const QString& state) {
    // Подсчитываем фигуры для определения хода
    int xCount = 0;
    int oCount = 0;
    
    for (int i = 0; i < state.length(); ++i) {
        if (state[i] == 'X') xCount++;
        if (state[i] == 'O') oCount++;
    }
    
    bool isXTurn = (xCount == oCount);
    bool myTurnNow = (mySymbol_ == ttt::core::Player::X) ? isXTurn : !isXTurn;

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r * 3 + c;
            QPushButton* btn = cells_[r][c];
            
            if (idx < state.length()) {
                QChar ch = state[idx];
                
                // 1. Устанавливаем текст
                btn->setText(QString(ch));
                
                // 2. Сбрасываем стиль, чтобы избежать конфликтов кэширования
                btn->setStyleSheet(""); 
                
                // 3. Применяем новый стиль в зависимости от символа
                QString style;
                if (ch == 'X') {
                    style = "background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';";
                } else if (ch == 'O') {
                    style = "background-color: white; color: #DC143C; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';";
                } else {
                    // Пустая клетка
                    style = "background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';";
                }
                btn->setStyleSheet(style);

                // 4. Управление доступностью (блокировка)
                bool canClick = (ch == '.' && myTurnNow);
                btn->setEnabled(canClick);
            }
        }
    }

    // Обновляем статусную строку
    if (myTurnNow) {
        QString symbolStr = (mySymbol_ == ttt::core::Player::X) ? "X" : "O";
        statusLabel_->setText("Ваш ход (" + symbolStr + ")");
        statusLabel_->setStyleSheet("color: #00FF00; font-weight: bold; font-size: 16px;");
    } else {
        statusLabel_->setText("Ход противника...");
        statusLabel_->setStyleSheet("color: #FFFF00; font-weight: bold; font-size: 16px;");
    }
    
    // Форсируем перерисовку окна
    this->update();
    QApplication::processEvents();
}

void GameWindow::onCellClicked(int row, int col) {
    // Проверка: кнопка должна быть enabled. Если нет - выходим.
    if (!cells_[row][col]->isEnabled()) return;
    
    // Дополнительная проверка на всякий случай
    if (!socket_) return;

    // Отправляем ход
    auto msg = ttt::net::Protocol::make_move(row, col);
    socket_->write(QString::fromStdString(
    ttt::net::Protocol::serialize(msg)
    ).toUtf8());

    socket_->flush();
    socket_->waitForBytesWritten();
    
    // Сразу блокируем кнопку, чтобы нельзя было нажать дважды пока ждем ответ
    cells_[row][col]->setEnabled(false);
    statusLabel_->setText("Отправка хода...");
}

void GameWindow::onRestartClicked() {
    if (!socket_) return;
    socket_->write("RESTART\n");
    isMyTurn_ = (mySymbol_ == ttt::core::Player::X);
    statusLabel_->setText("Ожидание перезапуска...");
    resetBoardUI();
}

void GameWindow::resetBoardUI() {
    for (auto& row : cells_)
        for (auto btn : row) {
            btn->setText(".");
            btn->setStyleSheet("background:white;color:#0000CD;border:2px solid #00008B;border-radius:10px;font:bold 36px 'Arial';");
            btn->setEnabled(false);
        }
}