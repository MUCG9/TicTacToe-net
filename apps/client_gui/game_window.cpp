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
    // --- Глобальный стиль окна (Темно-синий фон, как на слайде) ---
    this->setStyleSheet(
        "QMainWindow {"
        "   background-color: #0000CD;" /* MediumBlue - основной цвет слайда */
        "   color: white;"
        "}"
        "QLabel {"
        "   color: white;"
        "   font-weight: bold;"
        "   font-size: 14px;"
        "}"
        "QPushButton {"
        "   background-color: white;"
        "   color: #0000CD;"
        "   border: 2px solid #00008B;" /* DarkBlue */
        "   border-radius: 10px;"
        "   font: bold 36px 'Arial';"
        "}"
        "QPushButton:hover {"
        "   background-color: #E6E6FA;" /* Lavender - легкий оттенок при наведении */
        "   border: 2px solid white;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #F0F8FF;"
        "   color: #A9A9A9;"
        "   border: 2px solid #DCDCDC;"
        "}"
    );

    auto central = new QWidget(this);
    setCentralWidget(central);
    
    // Основной вертикальный компоновщик
    auto mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // --- Верхняя панель статуса ---
    statusLabel_ = new QLabel("Подключение...", this);
    statusLabel_->setAlignment(Qt::AlignCenter);
    statusLabel_->setFixedHeight(30);
    mainLayout->addWidget(statusLabel_);

    // --- Сетка игрового поля ---
    auto grid = new QGridLayout();
    grid->setSpacing(15); // Расстояние между клетками

    for (int r = 0; r < 3; ++r) {
        std::vector<QPushButton*> row;
        for (int c = 0; c < 3; ++c) {
            auto btn = new QPushButton("", this);
            btn->setFixedSize(100, 100); // Чуть крупнее клетки
            
            // Подключаем клик
            connect(btn, &QPushButton::clicked, this, [this, r, c]() { 
                onCellClicked(r, c); 
            });
            
            grid->addWidget(btn, r, c);
            row.push_back(btn);
        }
        cells_.push_back(std::move(row));
    }
    
    mainLayout->addLayout(grid);
    
    // Растягиваем сетку к центру, если окно большое
    mainLayout->addStretch();

    // Настройки окна
    setFixedSize(360, 420);
    setWindowTitle("Крестики-нолики | TTT-Net");
    
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
        QString resRaw = QString::fromStdString(msg.args[0]);
        QString message;
        QMessageBox::Icon icon;

        // Красивые сообщения для разных исходов
        if (resRaw == "DRAW") {
            message = "Ничья! Дружба победила.";
            icon = QMessageBox::Information;
        } else if (resRaw == "X_WINS") {
            if (isMyTurn_ == false) { // Если сейчас не мой ход, значит ходил противник (O), а выиграл X (я или он?)
                // Логика простая: если мы играем за X и видим X_WINS -> мы выиграли
                // Но проще проверить, кто мы. Давайте упростим:
                message = "Победили КРЕСТИКИ (X)!";
                icon = QMessageBox::Information;
            } else {
                 message = "Победили КРЕСТИКИ (X)!";
                 icon = QMessageBox::Information;
            }
        } else if (resRaw == "O_WINS") {
            message = "Победили НОЛИКИ (O)!";
            icon = QMessageBox::Information;
        } else {
            message = "Игра завершена: " + resRaw;
            icon = QMessageBox::NoIcon;
        }

        // Показываем красивое всплывающее окно
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Результат игры");
        msgBox.setText(message);
        msgBox.setIcon(icon);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setStyleSheet(
            "QMessageBox {"
            "   background-color: #F0F8FF;" /* Светлый фон окна */
            "   font-size: 16px;"
            "}"
            "QPushButton {"
            "   background-color: #0000CD;" /* Синяя кнопка как в теме */
            "   color: white;"
            "   border-radius: 5px;"
            "   padding: 5px 15px;"
            "   min-width: 80px;"
            "}"
            "QPushButton:hover {"
            "   background-color: #00008B;"
            "}"
        );
        
        msgBox.exec(); // Ждем нажатия OK
    }
}

void GameWindow::applyBoardState(const QString& state) {
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            int idx = r * 3 + c;
            if (idx < state.length()) {
                QChar ch = state[idx];
                cells_[r][c]->setText(QString(ch));
                
                // Стилизация символов
                if (ch == 'X') {
                    // Крестик: Ярко-синий, как заголовок на слайде
                    cells_[r][c]->setStyleSheet(
                        "background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';"
                    );
                } else if (ch == 'O') {
                    // Нолик: Можно сделать красным или оставить синим, но другим оттенком
                    cells_[r][c]->setStyleSheet(
                        "background-color: white; color: #DC143C; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';"
                    );
                } else {
                    // Пустая клетка
                    cells_[r][c]->setStyleSheet(
                        "background-color: white; color: #0000CD; border: 2px solid #00008B; border-radius: 10px; font: bold 36px 'Arial';"
                    );
                }

                // Блокировка занятых клеток
                cells_[r][c]->setEnabled(ch == '.');
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