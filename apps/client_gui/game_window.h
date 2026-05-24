#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QVector>
#include <QLabel>
#include <QPushButton>
#include "core/board.h"

class QGridLayout;
class QVBoxLayout;

class GameWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GameWindow(QTcpSocket* socket, const QString& opponentName,
                        ttt::core::Player symbol, QWidget* parent = nullptr);
    ~GameWindow() override;

private slots:
    void onDisconnected();
    void onReadyRead();
    void onCellClicked(int row, int col);
    void onRestartClicked();

private:
    QTcpSocket* socket_;
    std::vector<std::vector<QPushButton*>> cells_;
    QLabel* statusLabel_;
    QLabel* infoLabel_;
    QLabel* roleLabel_;          // <-- NEW: Показывает ваш символ
    QPushButton* restartBtn_;
    bool isMyTurn_;
    ttt::core::Player mySymbol_;

    void setupUI();
    void parseServerMessage(const QString& raw);
    void applyBoardState(const QString& state);
};