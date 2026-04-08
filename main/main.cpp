#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Solve-A-Squan");
    app.setApplicationVersion("2.1");
    app.setWindowIcon(QIcon(":/icon.ico"));
    MainWindow w;
    w.showMaximized();
    return app.exec();
}