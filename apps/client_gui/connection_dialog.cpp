#include "connection_dialog.h"
#include "net/protocol.h"
#include <QMessageBox>
#include <QApplication>
#include <QEventLoop>

ConnectionDialog::ConnectionDialog(QWidget* parent) : QDialog(parent), socket_(new QTcpSocket(this)) {
    setWindowTitle("mipt.ttt - Подключение");
    setFixedSize(320, 220);

    auto layout = new QVBoxLayout(this);
    auto form = new QFormLayout();

    hostEdit = new QLineEdit("127.0.0.1");
    portEdit = new QLineEdit("9090");
    roomEdit = new QLineEdit("test"); // По умолчанию test для удобства
    nameEdit = new QLineEdit("Player1");

    form->addRow("Сервер:", hostEdit);
    form->addRow("Порт:", portEdit);
    form->addRow("Комната:", roomEdit);
    form->addRow("Ваше имя:", nameEdit);
    layout->addLayout(form);

    statusLabel = new QLabel("Введите данные и нажмите подключиться");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    layout->addWidget(statusLabel);

    connectBtn = new QPushButton("Подключиться / Создать комнату");
    connect(connectBtn, &QPushButton::clicked, this, &ConnectionDialog::onConnect);
    layout->addWidget(connectBtn);
}

void ConnectionDialog::onConnect() {
    connectBtn->setEnabled(false);
    statusLabel->setText("Подключение к серверу...");
    QApplication::processEvents();

    socket_->connectToHost(hostEdit->text(), portEdit->text().toInt());
    
    // Ждем подключения с таймаутом
    if (!socket_->waitForConnected(5000)) {
        QMessageBox::critical(this, "Ошибка сети", "Не удалось подключиться: " + socket_->errorString());
        connectBtn->setEnabled(true);
        statusLabel->setText("Готово к подключению");
        return;
    }

    roomId_ = roomEdit->text();
    playerName_ = nameEdit->text();
    
    // Отправляем JOIN
    std::string joinMsg = "JOIN " + roomId_.toStdString() + " " + playerName_.toStdString();
    socket_->write(joinMsg.c_str());
    socket_->waitForBytesWritten();
    
    statusLabel->setText("Обработка ответа сервера...");
    QApplication::processEvents();

    // Используем QEventLoop для ожидания нужного события, не блокируя UI полностью жестким циклом
    QEventLoop loop;
    bool gameReady = false;
    bool errorOccurred = false;
    QString errorMsg;

    // Подключаемся к сигналу readyRead
    connect(socket_, &QTcpSocket::readyRead, [&]() {
        QByteArray data = socket_->readAll();
        QString raw = QString::fromUtf8(data).trimmed();
        
        // Сервер шлет сообщения через \n
        QStringList messages = raw.split('\n', Qt::SkipEmptyParts);
        
        for (const QString& msgStr : messages) {
            if (msgStr.isEmpty()) continue;
            
            try {
                auto msg = ttt::net::Protocol::deserialize(msgStr.toStdString());
                
                if (msg.type == "YOU_ARE") {
                    symbol_ = (msg.args[0] == "X") ? ttt::core::Player::X : ttt::core::Player::O;
                } else if (msg.type == "OPPONENT_NAME") {
                    opponentName_ = QString::fromStdString(msg.args[0]);
                } else if (msg.type == "WAITING") {
                    statusLabel->setText("Ожидание второго игрока...\nКомната: " + roomId_);
                } else if (msg.type == "ROOM_READY") {
                    gameReady = true;
                    loop.quit(); // Выходим из цикла ожидания
                } else if (msg.type == "ERROR") {
                    errorOccurred = true;
                    errorMsg = QString::fromStdString(msg.args[0]);
                    loop.quit();
                }
            } catch (...) {
                // Игнорируем битые пакеты
            }
        }
    });

    // Также следим за разрывом соединения
    connect(socket_, &QTcpSocket::disconnected, [&]() {
        if (!gameReady && !errorOccurred) {
            errorOccurred = true;
            errorMsg = "Соединение разорвано сервером.";
            loop.quit();
        }
    });

    // Запускаем цикл обработки событий, пока не получим ROOM_READY или ошибку
    loop.exec();

    if (errorOccurred) {
        QMessageBox::critical(this, "Ошибка", errorMsg);
        connectBtn->setEnabled(true);
        statusLabel->setText("Готово к подключению");
        return;
    }

    if (gameReady) {
        accept(); // Закрываем диалог с кодом Accepted
    }
}

QTcpSocket* ConnectionDialog::releaseSocket() {
    // Отцепляем сокет от диалога, чтобы он не закрылся при удалении диалога
    socket_->setParent(nullptr);
    return socket_;
}