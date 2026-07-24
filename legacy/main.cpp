#include "mainwindow.h"
#include "sq1-core/sq1opt-runner.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDir>

// Resolve <app dir>/pruning-tables, creating it if needed, and point the solver
// at it — matching where the GUI keeps its tables (SolverWorker::run).
static void useTableDirectory()
{
    QDir tableDir(QCoreApplication::applicationDirPath());
    if (!tableDir.exists("pruning-tables"))
        tableDir.mkpath("pruning-tables");
    tableDir.cd("pruning-tables");
    sq1optSetTableDirectory(tableDir.absolutePath().toStdString());
}

int main(int argc, char* argv[]) {
    // CLI mode: any command-line arguments mean "behave like sq1opt". Use a
    // GUI-less QCoreApplication (works headless) so the embedded solver runs
    // exactly as the standalone binary would, against the app's pruning tables.
    if (argc > 1)
    {
        QCoreApplication app(argc, argv);
        useTableDirectory();
        return sq1optMain(argc, argv);
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    app.setApplicationName("Solve-Da-Squan");
    app.setApplicationVersion("2.1");
    app.setWindowIcon(QIcon(":/icon.ico"));
    MainWindow w;
    w.showMaximized();
    return app.exec();
}
