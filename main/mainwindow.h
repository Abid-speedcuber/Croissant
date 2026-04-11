#pragma once
#include <QMainWindow>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QPointer>
#include <QSpinBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QKeyEvent>
#include <QScrollArea>
#include <atomic>
#include "stylesheet.h"

class QProcess;
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
    void requestStop();                     // kill the running process from any thread
signals:
    void lineReady(QString line);
    void finished(int exitCode);
private:
    std::atomic<QProcess*> m_proc{nullptr}; // set while process is live; null otherwise
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override; // global key routing + stop
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onSolveButtonClicked();    // dispatches to onSolve() or stopSolver()
    void onSolve();
    void onCopy();
    void onReset();
    void onSolverLine(QString line);
    void onSolverDone(int code);
    void updateCommand();
    void updateConstraints();       // enforces option incompatibilities + enables/disables fields
    void onRankErgoToggled(bool checked);
    void stopSolver();              // kill worker and update UI
    void onApplyScramble();
    void toggleExpand();            // expand / shrink the output terminal

private:
    void buildUI();
    void buildStyles();
    QStringList buildArgList();
    void updateRankErgoState();     // enable/disable chkRankErgo and refresh its tooltip
    void syncFlagsFromCommand(const QString& text);

    Sq1Widget*    cubeWidget;
    QCheckBox*    chkSlice;
    QCheckBox*    chkAllOptimal;
    QSpinBox*     spnSuboptimal;    // extra moves beyond optimal (0 = optimal only); hidden with -d
    QCheckBox*    chkDepths;
    QLineEdit*    txtDepths;
    QCheckBox*    chkGenerator;
    QCheckBox*    chk2gen;
    QCheckBox*    chkPseudo2gen;
    QCheckBox*    chkCubeshape;
    QCheckBox*    chkIgnoreMid;     // -m: ignore middle-layer shape
    QCheckBox*    chkKarnotation;
    QCheckBox*    chkSpecificAngle;
    QCheckBox*    chkMaxX;
    QSpinBox*     spnMaxX;
    QCheckBox*    chkMaxY;
    QSpinBox*     spnMaxY;
    QCheckBox*    chkMaxTotal;
    QSpinBox*     spnMaxTotal;
    QLineEdit*    txtCommand;
    QPushButton*  btnSolve;
    QPushButton*  btnCopy;
    QPushButton*  btnApply{nullptr};
    QPushButton*  btnReset;
    QPushButton*  btnUndo;
    QPushButton*  btnRedo;
    QLineEdit*    txtScramble;
    QPushButton*  btnApplyScramble;
    QPushButton*  btnScrambleMode;  // toggles scramble / algorithm mode
    QLabel*       lblScrambleError; // red error shown below the input bar
    QLabel*       lblCommandError;  // red error shown below the command line
    QPushButton*  btnExpand;        // ⤢ / ⤡ expand-shrink toggle
    QPushButton*  btnCopyTerminal;  // copy terminal contents
    QWidget*      m_topSection;     // options + command + solve + progress (hidden when expanded)
    QWidget*      m_leftPanel;      // cube widget column (hidden when expanded)
    QScrollArea*  leftScroll;       // scroll area for left panel
    QWidget*      m_outputWrapper;  // wrapper that holds terminal + floating btns
    QTextEdit*    txtOutput;
    QWidget*      m_tableContainer;
    QTableWidget* m_solutionTable;
    QPushButton*  btnTableMode;     // switches between table and terminal view
    QLabel*       lblStatus;
    QProgressBar* progressBar;
    QCheckBox*    chkRankErgo;
    bool          m_tableVisible{false};
    void          rebuildTerminalView();
    void          rebuildTable();
    void          appendStatusLine(const QString& msg);

    QPointer<SolverWorker> worker;
    QStringList   m_rawLines;
    QStringList   m_solutionLines;
    QSet<QString> m_seenSolutions;
    QString       m_posHex;         // position hex captured at solve time for ergo rating
    bool          m_stopped{false}; // true when user hit Stop (vs natural finish)
    qint64        m_firstSolutionMs{0};
    bool          m_hadFirstSolution{false};
    qint64        m_solveStartMs{0};
    bool          m_expanded{false};// true when output terminal is in full-screen mode
    bool          m_scrambleIsAlg{false};
    bool          m_applyFromSolved{false};
    int           m_inputModeIndex{0}; // 0=scramble, 1=alg, 2=position
    QPushButton*  m_inputMode{nullptr};
    QPushButton*  m_inputModeArrow{nullptr};
    QLineEdit*    m_mainInput{nullptr};
    int           m_sliceCount{0};
    QTimer*       m_sliceTimer{nullptr};    // true = input alg mode (inverts before applying)
    bool          m_cubeshapeWasActive{false}; // cubeshape state captured at solve time

    // Documentation content is fetched at run-time; no caching

    struct CubeSnapshot { QString posStr; };
    QVector<CubeSnapshot> m_undoStack;
    QVector<CubeSnapshot> m_redoStack;
    QVector<CubeSnapshot> m_slicePending;
    void pushUndoState();
    void showAboutModal();
    bool          m_lightTheme{false};
    QWidget*      m_sidebar{nullptr};
    QWidget*      m_sidebarOverlay{nullptr};
    bool          m_sidebarOpen{false};
    QPushButton*  btnHamburger{nullptr};
    QWidget*      m_inputBarOuter{nullptr};
    void showSettingsModal();
    void showHowToUseModal();
    void openSidebar();
    void closeSidebar();
    void applyTheme();
    std::function<void()> m_updateLogo;
    QString buildStyleSheet();
    // Documentation popups (readme style popups for v3/docs)
    void showReadDocsPopup();
    void showOldDocsPopup();
    // Helper to load document text from docs/ directory
    QString loadDocText(const QString& fileName);
};
