#include "connection_dialog.h"
#include "game_window.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle("Fusion");

    ConnectionDialog dialog;
    // exec() блокирует выполнение, пока диалог не будет закрыт (принят или отклонен)
    if (dialog.exec() == QDialog::Accepted) {
        GameWindow window(
            dialog.releaseSocket(),
            dialog.getOpponentName(),
            dialog.getSymbol(),
            nullptr
        );
        window.show();
        return app.exec();
    }
    return 0;
}