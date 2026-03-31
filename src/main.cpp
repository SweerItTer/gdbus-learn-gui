#include "MainWindow.hpp"

#include <QApplication>

// The demo intentionally opens two windows in one process so the user can
// immediately verify synchronized state across differently sized clients.
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    syncdemo::MainWindow window_a(QStringLiteral("Window A"));
    syncdemo::MainWindow window_b(QStringLiteral("Window B"));

    window_a.resize(960, 540);
    window_b.resize(720, 420);
    window_a.show();
    window_b.show();

    return app.exec();
}
