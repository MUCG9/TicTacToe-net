#include "game_window.h"
#include "config/parser.h"
#include <QApplication>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    ttt::config::Parser cfg;
    try { cfg.load_file("config/default.conf"); }
    catch (const std::exception& e) { std::cerr << "Config warning: " << e.what() << "\n"; }
    for (int i = 1; i < argc; ++i) {
        QString arg = argv[i];
        int eq = arg.indexOf('=');
        if (eq > 0) cfg.set_override(arg.left(eq).toStdString(), arg.mid(eq + 1).toStdString());
    }

    QString host = QString::fromStdString(cfg.get("client.server_host", "127.0.0.1"));
    quint16 port = static_cast<quint16>(std::stoi(cfg.get("client.server_port", "9090")));

    GameWindow w(host, port);
    w.show();
    return app.exec();
}