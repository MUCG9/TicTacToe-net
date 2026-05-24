#include "game_window.h"
#include "net/protocol.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QApplication>
#include <QStringList>
#include <iostream>

GameWindow::GameWindow(QTcpSocket* socket, const QString& opponentName,
                       ttt::core::Player symbol, QWidget* parent)
    : QMainWindow(parent), socket_(socket), 
      isMyTurn_(symbol == ttt::core::Player::X), mySymbol_(symbol) {
    setupUI();
    
    // Явно показываем, за кого играет пользователь
    roleLabel_->setText("Вы играете за: " + QString(symbol == ttt::core::Player::X ? "X (Крестики)" : "O (Нолики)"));
    infoLabel_->setText("Противник: " + opponentName);
    statusLabel_->setText(isMyTurn_ ? "Сейчас ходит X" : "Сейчас ходит O");
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
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    roleLabel_ = new QLabel("Загрузка...", this);
    roleLabel_->setAlignment(Qt::AlignCenter);
    roleLabel_->setStyleSheet("font-size: 16px; color: #FFFFFF;");
    mainLayout->addWidget(roleLabel_);

    infoLabel_ = new QLabel("Противник: ...", this);
    infoLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(infoLabel_);

    statusLabel_ = new QLabel("Ожидание хода...", this);
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
    QStringList msgs = data.split('\n', Qt::SkipEmptyParts);
    for (const QString& msg : msgs) {
        parseServerMessage(msg.trimmed());
    }
}

void GameWindow::parseServerMessage(const QString& raw) {
    auto msg = ttt::net::Protocol::deserialize(raw.toStdString());
    
    if (msg.type == "STATE") {
        applyBoardState(QString::fromStdString(msg.args.empty() ? "" : msg.args[0]));
    } 
    else if (msg.type == "ERROR") {
        statusLabel_->setText("Ошибка: " + QString::fromStdString(msg.args.empty() ? "" : msg.args[0]));
    } 
    else if (msg.type == "RESULT") {
        // Отладка в консоль, чтобы точно видеть, что приходит от сервера
        std::cout << "[CLIENT DEBUG] Received RESULT: ";
        for(const auto& arg : msg.args) std::cout << arg << " ";
        std::cout << std::endl;

        QString resRaw = msg.args.empty() ? "" : QString::fromStdString(msg.args[0]);
        QString message;
        
        if (resRaw == "DRAW") message = "🤝 Ничья! Дружба победила.";
        else if (resRaw == "X_WIN") message = " Победили крестики (X)!";
        else if (resRaw == "O_WIN") message = "🏆 Победили нолики (O)!";
        else message = "️ Игра завершена. Код: " + (resRaw.isEmpty() ? "UNKNOWN" : resRaw);

        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Результат игры");
        msgBox.setText(message);
        msgBox.setInformativeText("Нажмите OK, чтобы начать новую партию или закрыть окно.");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #F0F8FF; font-size: 16px; }"
            "QLabel { color: #000000; }"
            "QPushButton { background-color: #0000CD; color: white; border-radius: 5px; padding: 8px 20px; min-width: 80px; }"
            "QPushButton:hover { background-color: #00008B; }"
        );
        
        msgBox.exec(); // Модальное окно, блокирует интерфейс до нажатия OK
        
        statusLabel_->setText("Игра завершена. Нажмите 'Новая игра'");
        statusLabel_->setStyleSheet("color: #FF5555; font-weight: bold; font-size: 16px;");
        restartBtn_->setEnabled(true);
    }
}

void GameWindow::applyBoardState(const QString& state) {
    int xCount = 0;
    int oCount = 0;
    for (int i = 0; i < state.length(); ++i) {
        if (state[i] == 'X') xCount++;
        if (state[i] == 'O') oCount++;
    }

    bool isXTurn = (xCount == oCount);
    bool myTurnNow = (mySymbol_ == ttt::core::Player::X) ? isXTurn : !isXTurn;

    statusLabel_->setText(isXTurn ? "Сейчас ходит X" : "Сейчас ходит O");
    statusLabel_->setStyleSheet(myTurnNow ? "color: #00FF00; font-weight: bold; font-size: 16px;"
                                          : "color: #FFFF00; font-weight: bold; font-size: 16px;");

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r * 3 + c;
            QPushButton* btn = cells_[r][c];

            if (idx < state.length()) {
                QChar ch = state[idx];
                btn->setText(QString(ch));
                btn->setStyleSheet(""); 

                QString style;
                if (ch == 'X') 
                    style = "background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';";
                else if (ch == 'O') 
                    style = "background-color: white; color: #DC143C; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';";
                else 
                    style = "background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';";

                btn->setStyleSheet(style);
                btn->setEnabled(ch == '.' && myTurnNow);
            }
        }
    }
    this->update();
    QApplication::processEvents();
}

void GameWindow::onCellClicked(int row, int col) {
    if (!cells_[row][col]->isEnabled()) return;
    if (!socket_) return;

    auto msg = ttt::net::Protocol::make_move(row, col);
    socket_->write(QString::fromStdString(ttt::net::Protocol::serialize(msg)).toUtf8());
    
    cells_[row][col]->setEnabled(false);
    statusLabel_->setText("Отправка хода...");
}

void GameWindow::onRestartClicked() {
    if (!socket_) return;
    socket_->write("RESTART\n");
    isMyTurn_ = (mySymbol_ == ttt::core::Player::X);
    statusLabel_->setText("Ожидание перезапуска...");
    statusLabel_->setStyleSheet("color: #FFFFFF; font-weight: bold; font-size: 16px;");
}