#include "mainwindow.h"
#include "sq1widget.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QGroupBox>
#include <QClipboard>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>

// -------------------------------------------------------
// SolverWorker: runs sq1opt in-process by calling main()
// We redirect stdout via a pipe trick so we can stream output
// Actually simplest: just call sq1opt's logic directly.
// For now we use QProcess to call the exe (see note below).
// -------------------------------------------------------

// NOTE: The cleanest approach for a vibe coder:
// Compile sq1opt.cpp into a static library, then call its solve() from here.
// BUT for simplicity, we run the compiled sq1opt.exe as a child process
// and stream its stdout back into our text box.
// This means you compile TWO things: sq1opt.exe (as before) + this GUI app.

void SolverWorker::run() {
    // Build exe path - look next to the GUI exe
    QString exePath = QCoreApplication::applicationDirPath() + "/sq1opt";
#ifdef Q_OS_WIN
    exePath += ".exe";
#endif

    QProcess proc;
    proc.setWorkingDirectory(QCoreApplication::applicationDirPath());

    QStringList args;
    args << "-v5"; // verbosity
    args.append(flags);
    args << positionStr;

    proc.start(exePath, args);
    if(!proc.waitForStarted(3000)) {
        emit lineReady("ERROR: Could not start sq1opt. Make sure sq1opt.exe is in the same folder as this app.");
        emit finished(-1);
        return;
    }

    while(proc.waitForReadyRead(100) || proc.state()==QProcess::Running) {
        while(proc.canReadLine()) {
            QString line = QString::fromUtf8(proc.readLine()).trimmed();
            if(!line.isEmpty()) emit lineReady(line);
        }
    }
    // flush remaining
    proc.waitForFinished(5000);
    while(proc.canReadLine()) {
        QString line = QString::fromUtf8(proc.readLine()).trimmed();
        if(!line.isEmpty()) emit lineReady(line);
    }
    emit finished(proc.exitCode());
}

// -------------------------------------------------------
// MainWindow
// -------------------------------------------------------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Square-1 Optimizer");
    setMinimumSize(700, 520);
    buildUI();
    buildStyles();
    updateCommand();
}

void MainWindow::buildUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QHBoxLayout* root = new QHBoxLayout(central);
    root->setSpacing(12);
    root->setContentsMargins(12,12,12,12);

    // ---- LEFT: cube widget + move buttons ----
    QVBoxLayout* leftCol = new QVBoxLayout();

    cubeWidget = new Sq1Widget(this);
    connect(cubeWidget, &Sq1Widget::positionChanged, this, &MainWindow::updateCommand);
    leftCol->addWidget(cubeWidget);

    QHBoxLayout* btnRow1 = new QHBoxLayout();
    QPushButton* btnUP = new QPushButton("U'");
    QPushButton* btnU  = new QPushButton("U");
    btnRow1->addWidget(btnUP); btnRow1->addWidget(btnU);
    leftCol->addLayout(btnRow1);

    QPushButton* btnSlice = new QPushButton("Slice  [I/K]");
    leftCol->addWidget(btnSlice);

    QHBoxLayout* btnRow2 = new QHBoxLayout();
    QPushButton* btnD  = new QPushButton("D");
    QPushButton* btnDP = new QPushButton("D'");
    btnRow2->addWidget(btnD); btnRow2->addWidget(btnDP);
    leftCol->addLayout(btnRow2);

    btnReset = new QPushButton("Reset  [Esc]");
    btnReset->setObjectName("btnReset");
    leftCol->addWidget(btnReset);

    connect(btnU,     &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_J,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnUP,    &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_F,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnSlice, &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_I,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnD,     &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_S,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnDP,    &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_L,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnReset, &QPushButton::clicked, cubeWidget, &Sq1Widget::reset);

    root->addLayout(leftCol);

    // ---- RIGHT: options + output ----
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);

    // Options group
    QGroupBox* grpOptions = new QGroupBox("Options");
    QGridLayout* grid = new QGridLayout(grpOptions);
    grid->setVerticalSpacing(4);

    chkTwist      = new QCheckBox("Twist metric");
    chkAllOptimal = new QCheckBox("All optimal sequences");
    txtSuboptimal = new QLineEdit("0"); txtSuboptimal->setFixedWidth(40);
    chkGenerator  = new QCheckBox("Generator alg");
    chk2gen       = new QCheckBox("2Gen");
    chkPseudo2gen = new QCheckBox("Pseudo 2Gen");
    chkCubeshape  = new QCheckBox("Stay in cubeshape");
    chkKarnotation= new QCheckBox("Karnotation");
    chkMaxX       = new QCheckBox("Max top turn:");
    txtMaxX       = new QLineEdit("3"); txtMaxX->setFixedWidth(40);
    chkMaxY       = new QCheckBox("Max bottom turn:");
    txtMaxY       = new QLineEdit("3"); txtMaxY->setFixedWidth(40);
    chkMaxTotal   = new QCheckBox("Max total turn:");
    txtMaxTotal   = new QLineEdit("6"); txtMaxTotal->setFixedWidth(40);

    int row=0;
    grid->addWidget(chkTwist,      row++, 0, 1, 2);
    grid->addWidget(chkAllOptimal, row,   0);
    grid->addWidget(txtSuboptimal, row++, 1);
    grid->addWidget(chkGenerator,  row++, 0, 1, 2);
    grid->addWidget(chk2gen,       row++, 0, 1, 2);
    grid->addWidget(chkPseudo2gen, row++, 0, 1, 2);
    grid->addWidget(chkCubeshape,  row++, 0, 1, 2);
    grid->addWidget(chkKarnotation,row++, 0, 1, 2);
    grid->addWidget(chkMaxX,       row,   0); grid->addWidget(txtMaxX,    row++, 1);
    grid->addWidget(chkMaxY,       row,   0); grid->addWidget(txtMaxY,    row++, 1);
    grid->addWidget(chkMaxTotal,   row,   0); grid->addWidget(txtMaxTotal,row++, 1);

    // Connect all option changes to updateCommand
    auto upd = [this]{ updateCommand(); };
    connect(chkTwist,      &QCheckBox::toggled, this, upd);
    connect(chkAllOptimal, &QCheckBox::toggled, this, upd);
    connect(txtSuboptimal, &QLineEdit::textChanged, this, upd);
    connect(chkGenerator,  &QCheckBox::toggled, this, upd);
    connect(chk2gen,       &QCheckBox::toggled, this, upd);
    connect(chkPseudo2gen, &QCheckBox::toggled, this, upd);
    connect(chkCubeshape,  &QCheckBox::toggled, this, upd);
    connect(chkKarnotation,&QCheckBox::toggled, this, upd);
    connect(chkMaxX,       &QCheckBox::toggled, this, upd);
    connect(txtMaxX,       &QLineEdit::textChanged, this, upd);
    connect(chkMaxY,       &QCheckBox::toggled, this, upd);
    connect(txtMaxY,       &QLineEdit::textChanged, this, upd);
    connect(chkMaxTotal,   &QCheckBox::toggled, this, upd);
    connect(txtMaxTotal,   &QLineEdit::textChanged, this, upd);

    rightCol->addWidget(grpOptions);

    // Command preview
    QLabel* lblCmd = new QLabel("Command:");
    rightCol->addWidget(lblCmd);
    QHBoxLayout* cmdRow = new QHBoxLayout();
    txtCommand = new QLineEdit();
    txtCommand->setReadOnly(true);
    txtCommand->setObjectName("txtCommand");
    btnCopy = new QPushButton("Copy");
    btnCopy->setFixedWidth(60);
    cmdRow->addWidget(txtCommand);
    cmdRow->addWidget(btnCopy);
    rightCol->addLayout(cmdRow);

    // Solve button
    btnSolve = new QPushButton("▶  Solve");
    btnSolve->setObjectName("btnSolve");
    btnSolve->setFixedHeight(38);
    rightCol->addWidget(btnSolve);

    // Progress
    progressBar = new QProgressBar();
    progressBar->setRange(0,0); // indeterminate
    progressBar->setVisible(false);
    progressBar->setFixedHeight(6);
    rightCol->addWidget(progressBar);

    // Output
    QLabel* lblOut = new QLabel("Results:");
    rightCol->addWidget(lblOut);
    txtOutput = new QTextEdit();
    txtOutput->setReadOnly(true);
    txtOutput->setObjectName("txtOutput");
    txtOutput->setMinimumHeight(120);
    rightCol->addWidget(txtOutput, 1);

    // Status
    lblStatus = new QLabel("Ready.");
    lblStatus->setObjectName("lblStatus");
    rightCol->addWidget(lblStatus);

    root->addLayout(rightCol, 1);

    // Connect solve/copy
    connect(btnSolve, &QPushButton::clicked, this, &MainWindow::onSolve);
    connect(btnCopy,  &QPushButton::clicked, this, &MainWindow::onCopy);
}

void MainWindow::buildStyles() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #1a1a2e; color: #e0e0e0; font-family: 'Segoe UI', Arial; font-size: 13px; }
        QGroupBox { border: 1px solid #444; border-radius: 6px; margin-top: 8px; padding-top: 8px; color: #aaa; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QCheckBox { spacing: 6px; }
        QCheckBox::indicator { width:14px; height:14px; border-radius:3px; border:1px solid #666; background:#2a2a3e; }
        QCheckBox::indicator:checked { background: #4a90d9; border-color: #4a90d9; }
        QLineEdit { background: #2a2a3e; border: 1px solid #555; border-radius: 4px; padding: 3px 6px; color: #fff; }
        QLineEdit#txtCommand { font-family: monospace; color: #7fdbff; font-size: 12px; }
        QTextEdit#txtOutput { background: #0d1117; border: 1px solid #444; border-radius: 4px;
                              font-family: monospace; font-size: 12px; color: #7ec8e3; }
        QPushButton { background: #2a2a3e; border: 1px solid #555; border-radius: 5px; padding: 5px 12px; color: #ddd; }
        QPushButton:hover { background: #3a3a5e; border-color: #777; }
        QPushButton:pressed { background: #1a1a2e; }
        QPushButton#btnSolve { background: #1a6b3c; border-color: #2db570; color: #fff; font-size: 15px; font-weight: bold; }
        QPushButton#btnSolve:hover { background: #227a47; }
        QPushButton#btnSolve:disabled { background: #333; border-color: #444; color: #666; }
        QPushButton#btnReset { background: #6b1a1a; border-color: #b52d2d; color: #fdd; }
        QProgressBar { border: none; background: #2a2a3e; border-radius: 3px; }
        QProgressBar::chunk { background: #4a90d9; border-radius: 3px; }
        QLabel#lblStatus { color: #888; font-size: 11px; }
    )");
}

QStringList MainWindow::buildArgList() {
    QStringList args;
    if(chkTwist->isChecked())      args << "-w";
    if(chkAllOptimal->isChecked()) {
        QString sub = txtSuboptimal->text().trimmed();
        bool ok; int n = sub.toInt(&ok);
        args << (ok && n>0 ? QString("-a%1").arg(n) : QString("-a"));
    }
    if(chkGenerator->isChecked())  args << "-g";
    if(chk2gen->isChecked())       args << "-2";
    if(chkPseudo2gen->isChecked()) args << "-p";
    if(chkCubeshape->isChecked())  args << "-c";
    if(chkKarnotation->isChecked())args << "-k";
    if(chkMaxX->isChecked()) {
        bool ok; int v=txtMaxX->text().toInt(&ok);
        if(ok && v>=0 && v<=6) args << QString("-X%1").arg(v);
    }
    if(chkMaxY->isChecked()) {
        bool ok; int v=txtMaxY->text().toInt(&ok);
        if(ok && v>=0 && v<=6) args << QString("-Y%1").arg(v);
    }
    if(chkMaxTotal->isChecked()) {
        bool ok; int v=txtMaxTotal->text().toInt(&ok);
        if(ok && v>=1 && v<=12) args << QString("-Z%1").arg(v);
    }
    return args;
}

void MainWindow::updateCommand() {
    QString pos = cubeWidget->getPositionString();
    QStringList args = buildArgList();
    txtCommand->setText("sq1opt " + args.join(" ") + " " + pos);
}

void MainWindow::onSolve() {
    if(worker && worker->isRunning()) return;

    txtOutput->clear();
    lblStatus->setText("Solving...");
    btnSolve->setEnabled(false);
    progressBar->setVisible(true);

    worker = new SolverWorker();
    worker->positionStr = cubeWidget->getPositionString();
    worker->flags = buildArgList();

    connect(worker, &SolverWorker::lineReady, this, &MainWindow::onSolverLine, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished,  this, &MainWindow::onSolverDone, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished,  worker, &QObject::deleteLater);
    worker->start();
}

void MainWindow::onSolverLine(QString line) {
    // Color-code solutions (lines with [ in them) vs search depth lines
    if(line.contains("[") && line.contains("]")) {
        txtOutput->append("<span style='color:#00ff88;font-weight:bold;'>" +
                          line.toHtmlEscaped() + "</span>");
    } else {
        txtOutput->append("<span style='color:#888;'>" + line.toHtmlEscaped() + "</span>");
    }
}

void MainWindow::onSolverDone(int code) {
    progressBar->setVisible(false);
    btnSolve->setEnabled(true);
    lblStatus->setText(code==0 ? "Done." : "Error (code " + QString::number(code) + ")");
}

void MainWindow::onReset() {
    cubeWidget->reset();
    txtOutput->clear();
    lblStatus->setText("Ready.");
}

void MainWindow::onCopy() {
    QApplication::clipboard()->setText(txtCommand->text());
    lblStatus->setText("Copied to clipboard!");
}