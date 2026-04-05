#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Square-1 Optimizer");
    app.setApplicationVersion("2.1");
    MainWindow w;
    w.show();
    return app.exec();
}