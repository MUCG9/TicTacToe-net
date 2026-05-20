#pragma once
#include <QMainWindow>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include <memory>
#include "core/board.h"
#include "net/socket.h"

class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;

class GameWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GameWindow(std::unique_ptr<ttt::net::TcpSocket> sock, 
                        const QString& opponentName,
                        ttt::core::Player symbol,
                        QWidget* parent = nullptr);
    ~GameWindow() override;

private slots:
    void onDisconnected();
    void onReadyRead();
    void onCellClicked(int row, int col);
    void onRestartClicked();

private:
    std::unique_ptr<ttt::net::TcpSocket> socket_;
    std::vector<std::vector<QPushButton*>> cells_;
    QLabel* statusLabel_;
    QLabel* infoLabel_;
    QPushButton* restartBtn_;
    bool isMyTurn_;
    ttt::core::Player mySymbol_;

    void setupUI();
    void resetBoardUI();
    void parseServerMessage(const QString& raw);
    void applyBoardState(const QString& state);
};