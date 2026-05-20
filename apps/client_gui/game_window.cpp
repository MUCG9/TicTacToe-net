#include "game_window.h"
#include <QVBoxLayout>      
#include <QGridLayout>      
#include "config/parser.h"
#include "net/protocol.h"
#include <QMessageBox>
#include <QApplication>
#include <QTimer>
// ... остальное без изменений

GameWindow::GameWindow(const QString& host, quint16 port, QWidget* parent)
    : QMainWindow(parent), socket_(new QTcpSocket(this)),
      isConnected_(false), isMyTurn_(true) {
    setupUI();
    connect(socket_, &QTcpSocket::connected, this, &GameWindow::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &GameWindow::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead, this, &GameWindow::onReadyRead);
    connect(socket_, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &GameWindow::onSocketError);
    socket_->connectToHost(host, port);
}

GameWindow::~GameWindow() = default;

void GameWindow::setupUI() {
    auto central = new QWidget(this);
    setCentralWidget(central);
    auto mainLayout = new QVBoxLayout(central);

    statusLabel_ = new QLabel("Подключение...", this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(statusLabel_);

    auto grid = new QGridLayout();
    for (int r = 0; r < 3; ++r) {
        std::vector<QPushButton*> row;
        for (int c = 0; c < 3; ++c) {
            auto btn = new QPushButton("", this);
            btn->setFixedSize(80, 80);
            btn->setStyleSheet("font: bold 32px;");
            connect(btn, &QPushButton::clicked, this, [this, r, c]() { onCellClicked(r, c); });
            grid->addWidget(btn, r, c);
            row.push_back(btn);
        }
        cells_.push_back(std::move(row));
    }
    mainLayout->addLayout(grid);
    setFixedSize(300, 350);
    setWindowTitle("Крестики-нолики (Qt Client)");
}

void GameWindow::onConnected() {
    isConnected_ = true;
    statusLabel_->setText("Подключено. Ждём начала игры...");
    resetBoardUI();
}

void GameWindow::onDisconnected() {
    isConnected_ = false;
    statusLabel_->setText("Сервер отключился. Переподключение...");
    QTimer::singleShot(2000, this, &GameWindow::tryReconnect);
}

void GameWindow::onSocketError(QAbstractSocket::SocketError error) {
    statusLabel_->setText("Ошибка сети: " + socket_->errorString());
    if (!isConnected_) QTimer::singleShot(3000, this, &GameWindow::tryReconnect);
}

void GameWindow::tryReconnect() {
    if (!socket_->isOpen()) socket_->connectToHost(socket_->peerName(), socket_->peerPort());
}

void GameWindow::onReadyRead() {
    QString data = QString::fromUtf8(socket_->readAll());
    parseServerMessage(data.trimmed());
}

void GameWindow::parseServerMessage(const QString& raw) {
    auto msg = ttt::net::Protocol::deserialize(raw.toStdString());
    
    if (msg.type == "YOU_ARE") {
        QString me = QString::fromStdString(msg.args[0]);
        statusLabel_->setText("Вы играете за: " + me + ". Ждём второго игрока...");
        return;
    }
    
    if (msg.type == "STATE") {
        pendingBoard_ = QString::fromStdString(msg.args.empty() ? "" : msg.args[0]);
        isMyTurn_ = true;
        statusLabel_->setText("Ваш ход");
        applyBoardState(pendingBoard_);
    } else if (msg.type == "ERROR") {
        statusLabel_->setText("Ошибка: " + QString::fromStdString(msg.args[0]));
    } else if (msg.type == "RESULT") {
        QString res = QString::fromStdString(msg.args[0]);
        statusLabel_->setText("Игра окончена: " + res);
        QMessageBox::information(this, "Результат", "Результат: " + res);
    }
}

void GameWindow::applyBoardState(const QString& state) {
    // state теперь выглядит как "X..O..X.." (9 символов)
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r * 3 + c;
            if (idx < state.length()) {
                QChar ch = state[idx];
                cells_[r][c]->setText(QString(ch));
                
                // Раскраска
                if (ch == 'X') 
                    cells_[r][c]->setStyleSheet("font: bold 32px; color: blue;");
                else if (ch == 'O') 
                    cells_[r][c]->setStyleSheet("font: bold 32px; color: red;");
                else 
                    cells_[r][c]->setStyleSheet("font: bold 32px; color: black;");

                // Доступность кнопки
                cells_[r][c]->setEnabled(ch == '.' && isMyTurn_);
            }
        }
    }
}

void GameWindow::onCellClicked(int row, int col) {
    if (!isConnected_ || !isMyTurn_) return;
    auto msg = ttt::net::Protocol::make_move(row, col);
    socket_->write(QString::fromStdString(ttt::net::Protocol::serialize(msg)).toUtf8());
    isMyTurn_ = false;
    statusLabel_->setText("Ход отправлен...");
    cells_[row][col]->setEnabled(false);
}

void GameWindow::resetBoardUI() {
    for (auto& row : cells_)
        for (auto btn : row) {
            btn->setText(".");
            btn->setEnabled(true);
            btn->setStyleSheet("font: bold 32px;");
        }
}