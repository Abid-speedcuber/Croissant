#pragma once
#include <QMainWindow>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <QStringList>

// Forward declarations
class Sq1Widget;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;
class QProgressBar;

// -------------------------------------------------------
// Worker thread: runs the solver without freezing the UI
// -------------------------------------------------------
class SolverWorker : public QThread {
    Q_OBJECT
public:
    QString positionStr;
    QStringList flags;
    void run() override;
signals:
    void lineReady(QString line);
    void finished(int exitCode);
};

// -------------------------------------------------------
// Main Window
// -------------------------------------------------------
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

private slots:
    void onSolve();
    void onCopy();
    void onReset();
    void onSolverLine(QString line);
    void onSolverDone(int code);
    void updateCommand();

private:
    void buildUI();
    void buildStyles();
    QString buildCommandArgs();   // returns args string for display
    QStringList buildArgList();   // returns actual args list for solver

    // Canvas widget (the cube visualizer)
    Sq1Widget*   cubeWidget;

    // Controls
    QCheckBox*   chkTwist;
    QCheckBox*   chkAllOptimal;
    QLineEdit*   txtSuboptimal;
    QCheckBox*   chkGenerator;
    QCheckBox*   chk2gen;
    QCheckBox*   chkPseudo2gen;
    QCheckBox*   chkCubeshape;
    QCheckBox*   chkKarnotation;
    QCheckBox*   chkMaxX;
    QLineEdit*   txtMaxX;
    QCheckBox*   chkMaxY;
    QLineEdit*   txtMaxY;
    QCheckBox*   chkMaxTotal;
    QLineEdit*   txtMaxTotal;

    QLineEdit*   txtCommand;   // read-only preview
    QPushButton* btnSolve;
    QPushButton* btnCopy;
    QPushButton* btnReset;
    QTextEdit*   txtOutput;
    QLabel*      lblStatus;
    QProgressBar* progressBar;

    SolverWorker* worker = nullptr;
};