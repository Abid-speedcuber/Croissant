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
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QVector>
#include <atomic>
#include <functional>
#include "styles/stylesheet.h"
#include "sq1-core/output-converter.h"

class Sq1Widget;
class QCheckBox;
class QButtonGroup;
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
    void requestStop();
signals:
    void lineReady(QString line);
    void finished(int exitCode);
private:
    std::atomic_bool m_stopRequested{false};
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
    void toggleExpand();            // expand / shrink the output terminal

private:
    void buildUI();
    void buildStyles();
    QStringList buildArgList();
    void syncFlagsFromCommand(const QString& text);

    Sq1Widget*    cubeWidget;
    QButtonGroup*  m_metricGroup{nullptr};  // 0=Slice(default) 1=Move 2=Angle
    QCheckBox*    chkAllOptimal;
    QSpinBox*     spnSuboptimal;    // extra moves beyond optimal (0 = optimal only); hidden with -d
    QCheckBox*    chkDepths;
    QLineEdit*    txtDepths;
    QCheckBox*    chkGenerator;
    QCheckBox*    chk2gen;
    QCheckBox*    chkPseudo2gen;
    QCheckBox*    chkCubeshape;
    QCheckBox*    chkIgnoreEquator;     // -m: ignore equator
    QCheckBox*    chkKarnotation;
    QButtonGroup*  m_angleGroup{nullptr};   // 0=Both 1=Top 2=Bottom 3=None(default)
    QButtonGroup*  m_normalizeAbfGroup{nullptr}; // 0=Both 1=PreABF 2=PostABF 3=None(default)
    QWidget*       m_normalizeAbfRow{nullptr};
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
    bool          m_tableVisible{false};
    bool          m_autoScrollPaused{false};
    bool          m_solveFinishedWhilePaused{false};
    QPushButton*  btnScrollToBottom{nullptr};
    void          rebuildTerminalView();
    void          rebuildTable();
    void          appendStatusLine(const QString& msg);

    QPointer<SolverWorker> worker;
    // m_rawLines      — every line from the solver process, raw WCA numeric format
    // m_karnLines     — same lines after karnify() is applied to solution lines
    // m_solutionLines — subset of m_rawLines that are solution lines (WCA numeric)
    // m_karnSolutionLines — same subset after karnify()
    QStringList   m_rawLines;
    QStringList   m_karnLines;
    QStringList   m_solutionLines;         // slice-indicator-injected raw WCA lines
    QStringList   m_karnSolutionLines;     // slice-indicator-injected karn lines
    QStringList   m_solutionLinesForRating; // clean numeric WCA lines (no injection) for rateAlg input
    // Ergonomic rating cache — populated once in onSolverDone when cubeshape was active.
    // Each entry: {index into m_solutionLines, median-normalised score}.
    // Sorted highest-score first; NaN (unratable) entries at end.
    QVector<QPair<int,double>> m_cachedRatedOrder;
    bool          m_ratingsValid{false};
    QStringList   m_sliceIndicators;  // per-solution slice indicator ("/", "\", or "|"), parallel to m_solutionLines
    QVector<double> m_rawFinalScores; // unormalized FINAL scores from onSolverLine, parallel to m_solutionLinesForRating
    QSet<QString> m_seenSolutions;
    QString       m_posHex;         // position hex captured at solve time for ergo rating
    bool          m_stopped{false}; // true when user hit Stop (vs natural finish)
    qint64        m_firstSolutionMs{0};
    bool          m_hadFirstSolution{false};
    qint64        m_solveStartMs{0};
    bool          m_expanded{false};// true when output terminal is in full-screen mode
    qreal         m_zoomScale{1.0};
    QWidget*      m_mainWidget{nullptr}; // inner widget for zoom scaffold
    QGraphicsView*  m_zoomView{nullptr};
    QGraphicsScene* m_zoomScene{nullptr};
    QGraphicsProxyWidget* m_zoomProxy{nullptr};
    void          applyZoom();
    bool          m_scrambleIsAlg{false};
    bool          m_applyFromSolved{false};
    int           m_inputModeIndex{0}; // 0=scramble, 1=alg, 2=position
    QPushButton*  m_inputMode{nullptr};
    QPushButton*  m_inputModeArrow{nullptr};
    QLineEdit*    m_mainInput{nullptr};
    int           m_sliceCount{0};
    QTimer*       m_sliceTimer{nullptr};    // true = input alg mode (inverts before applying)
    bool          m_cubeshapeWasActive{false}; // cubeshape state captured at solve time
    int           m_preIgnoreMidState{0}; // equator state before "ignore middle" was turned on

    // Documentation content is fetched at run-time; no caching

    struct CubeSnapshot { QString posStr; };
    QVector<CubeSnapshot> m_undoStack;
    QVector<CubeSnapshot> m_redoStack;
    QVector<CubeSnapshot> m_slicePending;

    struct TableRow { QString alg; int moves; int slices; int angle; double ergo; };
    QVector<TableRow> m_pendingTableRows;
    QTimer*           m_tableFillTimer{nullptr};
    int               m_tableFilledCount{0};
    void              fillNextTableBatch();
    void pushUndoState();
    void showAboutModal();

    // settings stuffs
    bool          m_lightTheme{false};
    bool          m_smartKarn{true};
    bool          m_abidNotation{false};
    bool          m_ignoreTrans{false};
    int           m_normalizeAbfMode{3}; // 0=Both 1=PreABF 2=PostABF 3=None
    QButtonGroup* m_normalizeAbfDisplayGroup{nullptr}; // the new below-terminal pill
    QString       applyNormalizeAbf(const QString& rawAlgLine) const;
    QWidget*      m_sidebar{nullptr};
    QWidget*      m_sidebarOverlay{nullptr};
    bool          m_sidebarOpen{false};
    QPushButton*  btnHamburger{nullptr};
    QCheckBox*    chkSmartKarn{nullptr};
    QCheckBox*    chkIgnoreTransSetting{nullptr};
    QWidget*      m_inputBarOuter{nullptr};
    void showSettingsModal();
    void showHowToUseModal();
    void openSidebar();
    void closeSidebar();
    void applyTheme();
    std::function<void()> m_updateLogo;
    QString buildStyleSheet();
    QString convertLine(const QString& rawLine);
    // Documentation popups (readme style popups for v3/docs)
    void showReadDocsPopup();
    void showOldDocsPopup();
    // Helper to load document text from docs/ directory
    QString loadDocText(const QString& fileName);
};
