#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    app.setApplicationName("Solve-A-Squan");
    app.setApplicationVersion("2.1");
    app.setWindowIcon(QIcon(":/icon.ico"));
    MainWindow w;
    w.showMaximized();
    return app.exec();
}