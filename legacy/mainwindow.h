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
#include <QMap>
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
class QGraphicsOpacityEffect;

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
    void closeEvent(QCloseEvent* event) override; // persist settings on exit

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
    void setSolverRunning(bool running); // enable/disable all controls during solve
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
    QButtonGroup*  m_twoGenGroup{nullptr};  // 0=2Gen 1=Pseudo2Gen 2=None(default)
    QCheckBox*    chkCubeshape;
    QCheckBox*    chkIgnoreEquator;     // -m: ignore equator
    QCheckBox*    chkKarnotation;
    QButtonGroup*  m_angleGroup{nullptr};   // 0=Both 1=Top 2=Bottom 3=None(default)
    QButtonGroup*  m_normalizeAbfGroup{nullptr}; // 0=Both 1=PreABF 2=PostABF 3=None(default)
    QWidget*       m_normalizeAbfRow{nullptr};
    QWidget*       m_metricRadioRow{nullptr};
    QWidget*       m_twoGenRadioRow{nullptr};
    QWidget*       m_angleRadioRow{nullptr};
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
    QWidget*      m_moveButtonsWidget{nullptr}; // U/U'/D/D'/Slice grid container
    QPushButton*  btnUndo;
    QPushButton*  btnRedo;
    QLabel*       lblCommandError;  // red error shown below the command line
    QLabel*       lblSuboptLabel{nullptr}; // "+suboptimal:" label next to spnSuboptimal
    QWidget*      m_allOptRow{nullptr};    // the whole "all optimal + suboptimal" row
    QPushButton*  btnExpand;        // ⤢ / ⤡ expand-shrink toggle
    QPushButton*  btnCopyTerminal;  // copy terminal contents
    QPushButton*  btnFavorites{nullptr};  // open favorites bin modal
    QTimer*   m_outputIdleTimer{nullptr};
    bool      m_outputBtnsFullOpacity{true};
    void      setOutputBtnsOpacity(qreal target, int durationMs = 200);
    void      onOutputMouseActive();
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
    QSet<QString> m_seenNormalizedAlgs; // dedup on normalizeABF output
    QString       m_posHex;         // position hex captured at solve time for ergo rating
    bool          m_stopped{false}; // true when user hit Stop (vs natural finish)
    qint64        m_firstSolutionMs{0};
    bool          m_hadFirstSolution{false};
    QStringList   m_debugBuffer;
    QStringList   m_algAnnLines;   // per-alg rating annotation (empty if no rating), parallel to m_solutionLines
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
    int           m_preIgnoreMidState{1}; // equator state before "ignore middle" was turned on

    // Documentation content is fetched at run-time; no caching

    struct CubeSnapshot { QString posStr; };
    QVector<CubeSnapshot> m_undoStack;
    QVector<CubeSnapshot> m_redoStack;
    QVector<CubeSnapshot> m_slicePending;

    struct TableRow { QString alg; int moves; int slices; int angle; double ergo; QString rawLine; };
    QVector<TableRow> m_pendingTableRows;
    QTimer*           m_tableFillTimer{nullptr};
    int               m_tableFilledCount{0};
    void              fillNextTableBatch();
    void pushUndoState();
    void showAboutModal();
    void debugLine(const QString &msg);

    // settings stuffs
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
    bool          m_debugOutput{false};
    QWidget*      m_inputBarOuter{nullptr};
    void showSettingsModal();
    void showHowToUseModal();
    void showFavoritesModal();
    void openSidebar();
    void closeSidebar();
    void showAlgContextMenu(const QPoint &globalPos, const QString &rawLine);
    // Copy the terminal selection, substituting the clean alg for any solution
    // line the selection touches. Returns false if there is nothing to copy.
    bool copyTerminalSelection();
    void addToFavoritesBin(const QString &algLine);
    void applyRunConfig(const QString &key);
    void saveSettings();
    void loadSettings();
    QMap<QString, QStringList> m_favorites;
    QMap<QString, QString>     m_favNames;   // config_key -> custom display name
    QString m_lastRunKey;
    std::function<void()> m_updateLogo;
    QString buildStyleSheet();
    QString convertLine(const QString& rawLine);
    // Documentation popups (readme style popups for v3/docs)
    void showReadDocsPopup();
    void showOldDocsPopup();
    // Helper to load document text from docs/ directory
    QString loadDocText(const QString& fileName);
};
