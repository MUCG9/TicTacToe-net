#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QVector>
#include <QLabel>
#include <QPushButton>

class QGridLayout;
class QVBoxLayout;
class QHBoxLayout;

class GameWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GameWindow(const QString& host, quint16 port, QWidget* parent = nullptr);
    ~GameWindow() override;

private slots:
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onCellClicked(int row, int col);
    void tryReconnect();

private:
    QTcpSocket* socket_;
    std::vector<std::vector<QPushButton*>> cells_;
    QLabel* statusLabel_;
    QLabel* boardPreview_;
    bool isConnected_;
    bool isMyTurn_;
    QString pendingBoard_;

    void setupUI();
    void resetBoardUI();
    void parseServerMessage(const QString& raw);
    void applyBoardState(const QString& state);
};