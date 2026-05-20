#include "connection_dialog.h"
#include "net/protocol.h"
#include <QMessageBox>

ConnectionDialog::ConnectionDialog(QWidget* parent) : QDialog(parent) {
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

    try {
        socket_ = std::make_unique<ttt::net::TcpSocket>();
        socket_->connect(hostEdit->text().toStdString(), portEdit->text().toInt());

        roomId_ = roomEdit->text();
        playerName_ = nameEdit->text();
        
        std::string joinMsg = "JOIN " + roomId_.toStdString() + " " + playerName_.toStdString();
        socket_->send_data(joinMsg);

        // Читаем ответы сервера
        while (true) {
            auto raw = socket_->recv_data();
            auto msg = ttt::net::Protocol::deserialize(raw);
            
            if (msg.type == "YOU_ARE") {
                symbol_ = (msg.args[0] == "X") ? ttt::core::Player::X : ttt::core::Player::O;
            } else if (msg.type == "OPPONENT_NAME") {
                opponentName_ = QString::fromStdString(msg.args[0]);
            } else if (msg.type == "WAITING") {
                statusLabel->setText("Ожидание второго игрока в комнате " + roomId_ + "...");
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
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Ошибка сети", e.what());
        connectBtn->setEnabled(true);
        statusLabel->setText("Готово к подключению");
    }
}

std::unique_ptr<ttt::net::TcpSocket> ConnectionDialog::releaseSocket() {
    return std::move(socket_);
}