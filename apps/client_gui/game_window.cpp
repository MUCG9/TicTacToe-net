#include "game_window.h"
#include "net/protocol.h"
#include <QMessageBox>
#include <QApplication>
#include <QTimer>

GameWindow::GameWindow(std::unique_ptr<ttt::net::TcpSocket> sock, 
                       const QString& opponentName,
                       ttt::core::Player symbol,
                       QWidget* parent)
    : QMainWindow(parent), socket_(std::move(sock)), 
      isMyTurn_(symbol == ttt::core::Player::X), mySymbol_(symbol) {
    setupUI();
    infoLabel_->setText("Противник: " + opponentName);
    statusLabel_->setText(isMyTurn_ ? "Ваш ход (X)" : "Ход противника (O)");
    setWindowTitle("mipt.ttt");

    connect(socket_.get(), &ttt::net::TcpSocket::disconnected, this, &GameWindow::onDisconnected);
    connect(socket_.get(), &ttt::net::TcpSocket::readyRead, this, &GameWindow::onReadyRead);
}

GameWindow::~GameWindow() = default;

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
    parseServerMessage(data.trimmed());
}

void GameWindow::parseServerMessage(const QString& raw) {
    auto msg = ttt::net::Protocol::deserialize(raw.toStdString());
    
    if (msg.type == "STATE") {
        applyBoardState(QString::fromStdString(msg.args.empty() ? "" : msg.args[0]));
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
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r * 3 + c;
            if (idx < state.length()) {
                QChar ch = state[idx];
                cells_[r][c]->setText(QString(ch));
                
                if (ch == 'X') 
                    cells_[r][c]->setStyleSheet("background:white;color:#0000CD;border:2px solid #00008B;border-radius:10px;font:bold 36px 'Arial';");
                else if (ch == 'O') 
                    cells_[r][c]->setStyleSheet("background:white;color:#DC143C;border:2px solid #00008B;border-radius:10px;font:bold 36px 'Arial';");
                else 
                    cells_[r][c]->setStyleSheet("background:white;color:#0000CD;border:2px solid #00008B;border-radius:10px;font:bold 36px 'Arial';");

                bool isMyTurnNow = (mySymbol_ == ttt::core::Player::X) ? isMyTurn_ : !isMyTurn_;
                cells_[r][c]->setEnabled(ch == '.' && isMyTurnNow);
            }
        }
    }
}

void GameWindow::onCellClicked(int row, int col) {
    bool isMyTurnNow = (mySymbol_ == ttt::core::Player::X) ? isMyTurn_ : !isMyTurn_;
    if (!socket_ || !isMyTurnNow) return;
    
    auto msg = ttt::net::Protocol::make_move(row, col);
    socket_->send_data(QString::fromStdString(ttt::net::Protocol::serialize(msg)).toUtf8());
    isMyTurn_ = false;
    statusLabel_->setText("Ход отправлен...");
    cells_[row][col]->setEnabled(false);
}

void GameWindow::onRestartClicked() {
    if (!socket_) return;
    socket_->send_data("RESTART\n");
    isMyTurn_ = (mySymbol_ == ttt::core::Player::X);
    statusLabel_->setText("Ожидание перезапуска...");
    resetBoardUI();
}

void GameWindow::resetBoardUI() {
    for (auto& row : cells_)
        for (auto btn : row) {
            btn->setText(".");
            btn->setStyleSheet("background:white;color:#0000CD;border:2px solid #00008B;border-radius:10px;font:bold 36px 'Arial';");
        }
}