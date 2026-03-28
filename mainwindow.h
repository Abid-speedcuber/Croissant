#pragma once
#include <QMainWindow>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QPointer>

class Sq1Widget;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;
class QProgressBar;

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
    void onRankErgoToggled(bool checked);

private:
    void buildUI();
    void buildStyles();
    QStringList buildArgList();

    Sq1Widget*    cubeWidget;
    QCheckBox*    chkTwist;
    QCheckBox*    chkAllOptimal;
    QLineEdit*    txtSuboptimal;
    QCheckBox*    chkDepths;
    QLineEdit*    txtDepths;
    QCheckBox*    chkGenerator;
    QCheckBox*    chk2gen;
    QCheckBox*    chkPseudo2gen;
    QCheckBox*    chkCubeshape;
    QCheckBox*    chkKarnotation;
    QCheckBox*    chkMaxX;
    QLineEdit*    txtMaxX;
    QCheckBox*    chkMaxY;
    QLineEdit*    txtMaxY;
    QCheckBox*    chkMaxTotal;
    QLineEdit*    txtMaxTotal;
    QLineEdit*    txtCommand;
    QPushButton*  btnSolve;
    QPushButton*  btnCopy;
    QPushButton*  btnReset;
    QTextEdit*    txtOutput;
    QLabel*       lblStatus;
    QProgressBar* progressBar;
    QCheckBox*    chkRankErgo;

    QPointer<SolverWorker> worker;  // QPointer auto-nulls on deletion — safe for repeated solves
    QStringList   m_rawLines;
    QStringList   m_solutionLines;
    QString       m_posHex;          // position hex captured at solve time for ergo rating
};