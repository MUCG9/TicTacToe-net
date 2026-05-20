#include "connection_dialog.h"
#include "net/protocol.h"
#include <QMessageBox>
#include <QApplication>
#include <QTimer>

ConnectionDialog::ConnectionDialog(QWidget* parent) : QDialog(parent), socket_(new QTcpSocket(this)) {
    setWindowTitle("mipt.ttt - Подключение");
    setFixedSize(320, 220);

    auto layout = new QVBoxLayout(this);
    auto form = new QFormLayout();

    hostEdit = new QLineEdit("127.0.0.1");
    portEdit = new QLineEdit("9090");
    roomEdit = new QLineEdit("room1");
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
    if (!socket_->waitForConnected(5000)) {
        QMessageBox::critical(this, "Ошибка сети", "Не удалось подключиться к серверу.");
        connectBtn->setEnabled(true);
        statusLabel->setText("Готово к подключению");
        return;
    }

    roomId_ = roomEdit->text();
    playerName_ = nameEdit->text();
    
    std::string joinMsg = "JOIN " + roomId_.toStdString() + " " + playerName_.toStdString();
    socket_->write(joinMsg.c_str());
    socket_->waitForBytesWritten();

    // Читаем ответы сервера синхронно на этапе подключения
    while (true) {
        if (!socket_->waitForReadyRead(10000)) break;
        QString raw = QString::fromUtf8(socket_->readAll()).trimmed();
        auto msg = ttt::net::Protocol::deserialize(raw.toStdString());
        
        if (msg.type == "YOU_ARE") {
            symbol_ = (msg.args[0] == "X") ? ttt::core::Player::X : ttt::core::Player::O;
        } else if (msg.type == "OPPONENT_NAME") {
            opponentName_ = QString::fromStdString(msg.args[0]);
        } else if (msg.type == "WAITING") {
            statusLabel->setText("Ожидание второго игрока в комнате " + roomId_ + "...");
            QApplication::processEvents();
            continue;
        } else if (msg.type == "ROOM_READY") {
            statusLabel->setText("Игра началась!");
            QApplication::processEvents();
            emit connectedSuccessfully();
            accept();
            return;
        } else if (msg.type == "ERROR") {
            QMessageBox::critical(this, "Ошибка", QString::fromStdString(msg.args[0]));
            connectBtn->setEnabled(true);
            statusLabel->setText("Готово к подключению");
            return;
        }
    }
}

QTcpSocket* ConnectionDialog::releaseSocket() {
    socket_->setParent(nullptr); // Открепляем от диалога, чтобы не закрылся при его уничтожении
    return socket_;
}