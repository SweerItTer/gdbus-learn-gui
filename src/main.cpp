#include "MainWindow.hpp"

#include <QApplication>

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
