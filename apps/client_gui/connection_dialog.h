#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <memory>
#include "core/board.h"
#include "net/socket.h"

class ConnectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectionDialog(QWidget* parent = nullptr);
    std::unique_ptr<ttt::net::TcpSocket> releaseSocket();
    QString getPlayerName() const { return playerName_; }
    QString getRoomId() const { return roomId_; }
    ttt::core::Player getSymbol() const { return symbol_; }
    QString getOpponentName() const { return opponentName_; }

private slots:
    void onConnect();

signals:
    void connectedSuccessfully();

private:
    QLineEdit* hostEdit;
    QLineEdit* portEdit;
    QLineEdit* roomEdit;
    QLineEdit* nameEdit;
    QPushButton* connectBtn;
    QLabel* statusLabel;
    
    std::unique_ptr<ttt::net::TcpSocket> socket_;
    QString playerName_;
    QString roomId_;
    QString opponentName_;
    ttt::core::Player symbol_ = ttt::core::Player::X;
};