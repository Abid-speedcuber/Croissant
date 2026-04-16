#include "mainwindow.h"
#include "sq1widget.h"
#include "sq1-core/sq1-logic.h"
#include "sq1-core/karnotation.h"
#include "styles/theme.h"
#include "sq1-core/output-converter.h"
#include "styles/stylesheet.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QStyledItemDelegate>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QSpinBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QClipboard>
#include <QSet>
#include <QProcess>
#include <QCoreApplication>
#include <QProxyStyle>
#include <QToolTip>
#include <QHelpEvent>
#include <QTextBlock>
#include <QScrollBar>
#include <QMenu>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QDialog>
#include <QShortcut>
#include <QDateTime>
#include <QTextBrowser>
#include <QString>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <QPainter>

// ============================================================
// TightCheckBox — only shows tooltip when hovering over indicator+text
// ============================================================
class TightCheckBox : public QCheckBox {
public:
    using QCheckBox::QCheckBox;
    bool event(QEvent *e) override {
        if (e->type() == QEvent::ToolTip) {
            QStyleOptionButton opt;
            opt.initFrom(this);
            QRect textRect = style()->subElementRect(QStyle::SE_CheckBoxContents, &opt, this);
            QRect indRect  = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, this);
            QRect activeRect = textRect.united(indRect);
            QHelpEvent *he = static_cast<QHelpEvent*>(e);
            if (!activeRect.contains(he->pos()))
                return true; // swallow
        }
        return QCheckBox::event(e);
    }
};

// ============================================================
// SelectableDelegate — makes table cells selectable by mouse drag
// ============================================================
class SelectableDelegate : public QStyledItemDelegate {
public:
    explicit SelectableDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override {
        QLineEdit *ed = new QLineEdit(parent);
        ed->setReadOnly(true);
        ed->setFrame(false);
        ed->setStyleSheet("QLineEdit { background: transparent; border: none; padding: 0 4px; selection-background-color: #3a6ea8; selection-color: #ffffff; }");
        ed->setCursor(Qt::IBeamCursor);
        return ed;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override {
        static_cast<QLineEdit*>(editor)->setText(index.data().toString());
    }
    void setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const override {}
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override {
        editor->setGeometry(option.rect);
    }
};

// ============================================================
// FadingTooltip — singleton tooltip with fade-in, shown via hover timer
// ============================================================
class FadingTooltip : public QWidget {
public:
    // Call on MouseMove over a checkbox active area: arms the timer.
    // Call with empty text or from outside active area: dismisses.
    static void arm(const QString &text, const QPoint &globalPos, QWidget *parent) {
        inst(parent).armImpl(text, globalPos);
    }
    static void dismiss(QWidget *parent) {
        inst(parent).dismissImpl();
    }

private:
    explicit FadingTooltip(QWidget *parent) : QWidget(parent, Qt::SubWindow) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);

        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        m_label->setMaximumWidth(320);
        m_label->setContentsMargins(8, 5, 8, 5);

        QVBoxLayout *l = new QVBoxLayout(this);
        l->setContentsMargins(0,0,0,0);
        l->addWidget(m_label);

        m_effect = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(m_effect);
        m_effect->setOpacity(0.0);

        m_hoverTimer = new QTimer(this);
        m_hoverTimer->setSingleShot(true);
        connect(m_hoverTimer, &QTimer::timeout, this, &FadingTooltip::showNow);

        m_closeTimer = new QTimer(this);
        m_closeTimer->setSingleShot(true);
        connect(m_closeTimer, &QTimer::timeout, this, &FadingTooltip::dismissImpl);

        hide();
    }

    static FadingTooltip &inst(QWidget *parent) {
        static QPointer<FadingTooltip> s_inst;
        if (!s_inst) s_inst = new FadingTooltip(parent->window());
        return *s_inst;
    }

    void armImpl(const QString &text, const QPoint &globalPos) {
        // If already showing the same text, do nothing
        if (isVisible() && m_currentText == text) return;
        m_pendingText = text;
        m_pendingPos  = globalPos;
        if (!m_hoverTimer->isActive())
            m_hoverTimer->start(350);
    }

    void dismissImpl() {
        m_hoverTimer->stop();
        m_closeTimer->stop();
        if (!isVisible()) return;
        QPropertyAnimation *anim = new QPropertyAnimation(m_effect, "opacity", this);
        anim->setDuration(200);
        anim->setStartValue(m_effect->opacity());
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InCubic);
        connect(anim, &QPropertyAnimation::finished, this, &QWidget::hide);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
        m_currentText.clear();
    }

    void showNow() {
        m_currentText = m_pendingText;
        // Apply theme-matching style
        m_label->setStyleSheet(QString(
            "QLabel { background: transparent; color: %1; font-size: 11px; }")
            .arg(Theme::fadingTooltipText()));
        setStyleSheet(QString(
            "FadingTooltip { background: %1; border: 1px solid %2; border-radius: 5px; }")
            .arg(Theme::fadingTooltipBg(), Theme::fadingTooltipBorder()));
        m_label->setText(m_currentText);
        m_label->adjustSize();
        adjustSize();

        // Position relative to parent window
        QWidget *win = parentWidget();
        QPoint local = win->mapFromGlobal(m_pendingPos) + QPoint(14, 18);
        // Keep inside window bounds
        local.setX(qMin(local.x(), win->width()  - width()  - 8));
        local.setY(qMin(local.y(), win->height() - height() - 8));
        move(local);
        raise();
        show();

        // Fade in
        m_effect->setOpacity(0.0);
        QPropertyAnimation *anim = new QPropertyAnimation(m_effect, "opacity", this);
        anim->setDuration(320);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_closeTimer->start(8000);
    }

    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(Theme::fadingTooltipBg()));
        p.setPen(QColor(Theme::fadingTooltipBorder()));
        p.drawRoundedRect(rect().adjusted(0,0,-1,-1), 5, 5);
    }

    QLabel                 *m_label{nullptr};
    QGraphicsOpacityEffect *m_effect{nullptr};
    QTimer                 *m_hoverTimer{nullptr};
    QTimer                 *m_closeTimer{nullptr};
    QString                 m_pendingText;
    QPoint                  m_pendingPos;
    QString                 m_currentText;
};

// ============================================================
// FastTipStyle — QProxyStyle that makes tooltips appear instantly.
// Also extends the fall-asleep delay so tooltips linger naturally.
// ============================================================
class FastTipStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;
    int styleHint(StyleHint hint, const QStyleOption *opt = nullptr,
                  const QWidget *widget = nullptr,
                  QStyleHintReturn *ret = nullptr) const override
    {
        if (hint == QStyle::SH_ToolTip_WakeUpDelay)
            return 350;
        if (hint == QStyle::SH_ToolTip_FallAsleepDelay)
            return 8000; // linger 8 s
        return QProxyStyle::styleHint(hint, opt, widget, ret);
    }
};

// -------------------------------------------------------
// SolverWorker
// -------------------------------------------------------
void SolverWorker::requestStop()
{
    // Safe to call from any thread: m_proc is atomic.
    QProcess *p = m_proc.load();
    if (p)
        p->kill();
}

void SolverWorker::run()
{
    QString exePath = QCoreApplication::applicationDirPath() + "/solver-core/sq1opt";
#ifdef Q_OS_WIN
    exePath += ".exe";
#endif
    QProcess proc;
    // Publish pointer so requestStop() can kill it from the main thread.
    m_proc.store(&proc);

    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.setWorkingDirectory(QCoreApplication::applicationDirPath() + "/solver-core");
    QStringList args;
    args << "-v5";
    args.append(flags);
    args << positionStr;
    proc.start(exePath, args);
    if (!proc.waitForStarted(3000))
    {
        m_proc.store(nullptr);
        emit lineReady("ERROR: Could not start sq1opt. Make sure sq1opt.exe is in the solver-core folder.");
        emit finished(-1);
        return;
    }

    QByteArray buf;
    auto drainLines = [&]()
    {
        int nl;
        while ((nl = buf.indexOf('\n')) != -1)
        {
            QString line = QString::fromUtf8(buf.left(nl)).trimmed();
            buf.remove(0, nl + 1);
            if (!line.isEmpty())
                emit lineReady(line);
        }
    };

    while (true)
    {
        bool gotData = proc.waitForReadyRead(200);
        if (gotData)
            buf += proc.readAll();
        drainLines();
        if (!gotData && proc.state() == QProcess::NotRunning)
            break;
    }
    buf += proc.readAll();
    drainLines();
    buf = buf.trimmed();
    if (!buf.isEmpty())
        emit lineReady(QString::fromUtf8(buf));

    // Null the pointer BEFORE proc is destroyed so requestStop can't fire on a dead object.
    m_proc.store(nullptr);
    proc.waitForFinished(1000);
    emit finished(proc.exitCode());
}

// -------------------------------------------------------
// MainWindow
// -------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Solve-A-Squan");
    setMinimumSize(720, 560);
    resize(860, 700);

    // ── Instant tooltips ──────────────────────────────────────────────────────
    // Capture the current style name so FastTipStyle wraps a fresh copy of the
    // same base style, avoiding ownership issues with the old style pointer.
    {
        QString base = QApplication::style()->objectName();
        QApplication::setStyle(new FastTipStyle(base.isEmpty() ? "Fusion" : base));
    }

    // ── Global key intercept + txtDepths focus events ─────────────────────────
    qApp->installEventFilter(this);

    buildUI();
    buildStyles();
    updateCommand();

    m_sliceTimer = new QTimer(this);
    m_sliceTimer->setSingleShot(true);
    connect(m_sliceTimer, &QTimer::timeout, this, [this]
            {
                int saves = (m_sliceCount % 2 == 0) ? 2 : 1;
                m_undoStack.append(m_slicePending.first());
                if (saves == 2 && m_slicePending.size() >= 2)
                    m_undoStack.append(m_slicePending.last());
                while (m_undoStack.size() > 64) m_undoStack.removeFirst();
                btnUndo->setEnabled(true);
                m_redoStack.clear();
                btnRedo->setEnabled(false);
                m_sliceCount = 0;
                m_slicePending.clear(); });
}

static QString invertScrambleStr(const QString &str)
{
    if (str.trimmed().isEmpty())
        return str;
    QStringList parts = str.trimmed().split('/');
    std::reverse(parts.begin(), parts.end());
    QStringList result;
    for (QString part : parts)
    {
        part = part.trimmed();
        QString inner = part;
        inner.remove('(');
        inner.remove(')');
        inner = inner.trimmed();
        if (inner.contains(','))
        {
            QStringList nums = inner.split(',');
            if (nums.size() == 2)
            {
                bool ok1, ok2;
                int a = nums[0].trimmed().toInt(&ok1);
                int b = nums[1].trimmed().toInt(&ok2);
                if (ok1 && ok2)
                {
                    result << QString("%1,%2").arg(-a).arg(-b);
                    continue;
                }
            }
        }
        else if (!inner.isEmpty())
        {
            bool ok1;
            int a = inner.toInt(&ok1);
            if (ok1)
            {
                result << QString::number(-a);
                continue;
            }
        }
        result << part;
    }
    return result.join("/");
}

void MainWindow::buildUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QWidget *outerWidget = new QWidget(this);
    QVBoxLayout *outerLayout = new QVBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    setCentralWidget(outerWidget);

    // ── Top bar ───────────────────────────────────────────────────────────────
    QWidget *topBar = new QWidget();
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(52);
    QHBoxLayout *topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 0, 16, 0);

    QLabel *logoLabel = new QLabel();
    auto updateLogo = [this, logoLabel]()
    {
        QString primary = Theme::textPrimary(m_lightTheme);
        QString muted = Theme::textMuted(m_lightTheme);
        logoLabel->setText(QString("<span style='font-size:17px;font-weight:bold;color:%1;letter-spacing:1px;'>SOLVE-A-SQUAN</span>"
                                   "<br><span style='font-size:10px;color:%2;'>by Abid and Matt</span>")
                               .arg(primary, muted));
    };
    updateLogo();
    m_updateLogo = updateLogo;
    logoLabel->setObjectName("logoLabel");

    btnHamburger = new QPushButton("☰");
    btnHamburger->setObjectName("btnHamburger");
    btnHamburger->setFixedSize(30, 30);
    btnHamburger->setToolTip("Menu");
    connect(btnHamburger, &QPushButton::clicked, this, &MainWindow::openSidebar);

    topBarLayout->addWidget(btnHamburger);
    topBarLayout->addSpacing(10);
    topBarLayout->addWidget(logoLabel);
    topBarLayout->addStretch();
    outerLayout->addWidget(topBar);

    // ── Full-width input bar ──────────────────────────────────────────────────
    QWidget *inputBarOuter = new QWidget();
    inputBarOuter->setObjectName("inputBarOuter");
    m_inputBarOuter = inputBarOuter;

    QHBoxLayout *inputBarLay = new QHBoxLayout(inputBarOuter);
    inputBarLay->setContentsMargins(0, 0, 0, 0);
    inputBarLay->setSpacing(0);

    m_inputMode = new QPushButton("SCRAMBLE");
    m_inputMode->setObjectName("btnInputMode");

    m_inputModeArrow = new QPushButton("▾");
    m_inputModeArrow->setObjectName("btnInputModeArrow");

    m_mainInput = new QLineEdit();
    m_mainInput->setObjectName("txtMainInput");
    m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)");

    btnApply = new QPushButton("Apply");
    btnApply->setObjectName("btnApply");

    inputBarLay->addWidget(m_inputMode);
    inputBarLay->addWidget(m_inputModeArrow);
    inputBarLay->addWidget(m_mainInput, 1);
    inputBarLay->addWidget(btnApply);

    outerLayout->addWidget(inputBarOuter);

    QWidget *contentWidget = new QWidget();
    outerLayout->addWidget(contentWidget, 1);

    QHBoxLayout *root = new QHBoxLayout(contentWidget);
    root->setSpacing(12);
    root->setContentsMargins(12, 12, 12, 12);

    // ---- LEFT: cube widget + move buttons ----
    QWidget *leftContainer = new QWidget();
    leftContainer->setMinimumWidth(316);
    QVBoxLayout *leftCol = new QVBoxLayout(leftContainer);
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(4);
    cubeWidget = new Sq1Widget(this);
    connect(cubeWidget, &Sq1Widget::positionChanged, this, &MainWindow::updateCommand);
    connect(cubeWidget, &Sq1Widget::userInteracted, this, &MainWindow::pushUndoState);
    {
        // cubeWithReset fills the full width of leftContainer; reset button overlaps absolutely
        QWidget *cubeWithReset = new QWidget();
        cubeWithReset->setObjectName("cubeWithReset");
        cubeWithReset->setAttribute(Qt::WA_StyledBackground, true);
        // Height: cube height + some padding for the reset button row at top
        cubeWithReset->setFixedHeight(cubeWidget->height() + 8);
        // Width is NOT fixed — it will stretch to fill leftContainer

        // cubeWrapper is centered inside cubeWithReset via absolute positioning after layout
        QWidget *cubeWrapper = new QWidget(cubeWithReset);
        cubeWrapper->setObjectName("cubeWrapper");
        cubeWrapper->setAttribute(Qt::WA_StyledBackground, true);
        cubeWrapper->setFixedSize(cubeWidget->width(), cubeWidget->height());
        cubeWidget->setParent(cubeWrapper);
        cubeWidget->move(0, 0);
        // cubeWrapper will be centered in cubeWithReset after show (via event filter / resize)

        btnReset = new QPushButton("Reset", cubeWithReset);
        btnReset->setObjectName("btnReset");
        btnReset->setToolTip("Reset  [Esc]");
        // Position reset button at top-right; it overlaps the cube area
        btnReset->move(cubeWithReset->width() - 58, 6);
        btnReset->raise();

        // Use a resize event filter to keep cubeWrapper centered and btnReset at top-right
        struct CubeResizeFilter : public QObject {
            QWidget *cubeWithReset;
            QWidget *cubeWrapper;
            QPushButton *btnReset;
            CubeResizeFilter(QWidget *p, QWidget *cwr, QWidget *cwrap, QPushButton *btn)
                : QObject(p), cubeWithReset(cwr), cubeWrapper(cwrap), btnReset(btn) {}
            bool eventFilter(QObject *watched, QEvent *e) override {
                if (e->type() == QEvent::Resize && watched == cubeWithReset) {
                    int cx = (cubeWithReset->width() - cubeWrapper->width()) / 2;
                    cubeWrapper->move(cx, 9);
                    btnReset->move(cubeWithReset->width() - 58, 6);
                }
                return false;
            }
        };
        cubeWithReset->installEventFilter(
            new CubeResizeFilter(cubeWithReset, cubeWithReset, cubeWrapper, btnReset));

        // Initial position (will be corrected on first resize)
        cubeWrapper->move((300 - cubeWidget->width()) / 2, 9);

        QHBoxLayout *centerRow = new QHBoxLayout();
        centerRow->setContentsMargins(0, 0, 0, 0);
        centerRow->addWidget(cubeWithReset);  // stretches to fill
        leftCol->addLayout(centerRow);
    }

    // Grid: U' | Slice (rowspan 2) | U
    //        D |                   | D'
    QGridLayout *moveGrid = new QGridLayout();
    moveGrid->setSpacing(4);

    QPushButton *btnUP = new QPushButton("U'");
    QPushButton *btnU = new QPushButton("U");
    QPushButton *btnD = new QPushButton("D");
    QPushButton *btnDP = new QPushButton("D'");
    QPushButton *btnSlice = new QPushButton();
    btnSlice->setText("Slice [I/K]");
    btnSlice->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    moveGrid->addWidget(btnUP, 0, 0);
    moveGrid->addWidget(btnSlice, 0, 1, 2, 1);
    moveGrid->addWidget(btnU, 0, 2);
    moveGrid->addWidget(btnD, 1, 0);
    moveGrid->addWidget(btnDP, 1, 2);

    moveGrid->setColumnStretch(0, 1);
    moveGrid->setColumnStretch(1, 1);
    moveGrid->setColumnStretch(2, 1);
    moveGrid->setRowStretch(0, 1);
    moveGrid->setRowStretch(1, 1);

    leftCol->addLayout(moveGrid);

    // Row 3: Undo | Redo
    QHBoxLayout *undoResetRedoRow = new QHBoxLayout();
    undoResetRedoRow->setSpacing(4);
    btnUndo = new QPushButton("Undo (Z)");
    btnUndo->setObjectName("btnUndo");
    btnUndo->setEnabled(false);
    btnUndo->setToolTip("Undo  [Z]");
    btnRedo = new QPushButton("Redo (Y)");
    btnRedo->setObjectName("btnRedo");
    btnRedo->setEnabled(false);
    btnRedo->setToolTip("Redo  [Y]");
    undoResetRedoRow->addWidget(btnUndo, 1);
    undoResetRedoRow->addWidget(btnRedo, 1);
    leftCol->addLayout(undoResetRedoRow);

    btnSolve = new QPushButton("▶  Solve");
    btnSolve->setObjectName("btnSolve");
    btnSolve->setFixedHeight(48);
    leftCol->addWidget(btnSolve);

    // Stub out old scramble widgets so references don't break
    btnScrambleMode = new QPushButton();
    btnScrambleMode->setVisible(false);
    txtScramble = new QLineEdit();
    txtScramble->setVisible(false);
    btnApplyScramble = new QPushButton();
    btnApplyScramble->setVisible(false);
    lblScrambleError = new QLabel("");
    lblScrambleError->setObjectName("lblScrambleError");
    lblScrambleError->setWordWrap(true);
    lblScrambleError->setVisible(false);
    leftCol->addStretch();

    leftContainer->setObjectName("leftPanel");
    leftScroll = new QScrollArea();
    leftScroll->setObjectName("leftScroll");
    m_leftPanel = leftScroll;
    leftScroll->setWidget(leftContainer);
    leftScroll->setWidgetResizable(false);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setMinimumWidth(320);
    leftScroll->setMaximumWidth(380);

    connect(btnU, &QPushButton::clicked, cubeWidget, [this]
            { pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_J,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnUP, &QPushButton::clicked, cubeWidget, [this]
            { pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_F,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnSlice, &QPushButton::clicked, cubeWidget, [this]
            {
                cubeWidget->setFocus();
                m_sliceCount++;
                m_slicePending.append({cubeWidget->getPositionString()});
                QKeyEvent e(QEvent::KeyPress, Qt::Key_I, Qt::NoModifier);
                QApplication::sendEvent(cubeWidget, &e);
                m_sliceTimer->start(600); });
    connect(btnD, &QPushButton::clicked, cubeWidget, [this]
            { pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_S,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnDP, &QPushButton::clicked, cubeWidget, [this]
            { pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_L,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnReset, &QPushButton::clicked, this, [this]
            {
                QString before = cubeWidget->getPositionString();
                cubeWidget->reset();
                QString after = cubeWidget->getPositionString();
                if (before != after) {
                    m_undoStack.append({before});
                    if (m_undoStack.size() > 64) m_undoStack.removeFirst();
                    btnUndo->setEnabled(true);
                    m_redoStack.clear();
                    btnRedo->setEnabled(false);
                }
                onReset(); });
    connect(btnUndo, &QPushButton::clicked, this, [this]
            {
                if (m_undoStack.isEmpty()) return;
                m_redoStack.append({cubeWidget->getPositionString()});
                if (m_redoStack.size() > 64) m_redoStack.removeFirst();
                btnRedo->setEnabled(true);
                CubeSnapshot snap = m_undoStack.takeLast();
                cubeWidget->setPositionFromString(snap.posStr);
                updateCommand();
                btnUndo->setEnabled(!m_undoStack.isEmpty()); });
    connect(btnRedo, &QPushButton::clicked, this, [this]
            {
                if (m_redoStack.isEmpty()) return;
                m_undoStack.append({cubeWidget->getPositionString()});
                if (m_undoStack.size() > 64) m_undoStack.removeFirst();
                btnUndo->setEnabled(true);
                CubeSnapshot snap = m_redoStack.takeLast();
                cubeWidget->setPositionFromString(snap.posStr);
                updateCommand();
                btnRedo->setEnabled(!m_redoStack.isEmpty()); });

    root->addWidget(leftScroll);

    // ---- RIGHT: options + output ----
    QVBoxLayout *rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);

    QGroupBox *grpOptions = new QGroupBox("Options");
    grpOptions->setMinimumHeight(200);

    QWidget *optionsInner = new QWidget();
    QGridLayout *grid = new QGridLayout(optionsInner);
    grid->setVerticalSpacing(2);

    // ── Widgets ──────────────────────────────────────────────────────────────
    chkSlice = new TightCheckBox("Slice metric");
    chkSlice->setToolTip("If selected, only slices count as \"moves\", else layer turns count too.");

    chkAllOptimal = new TightCheckBox("All optimal");
    chkAllOptimal->setToolTip("Find all the optimal solutions, not just the first one.");

    spnSuboptimal = new QSpinBox();
    spnSuboptimal->setRange(0, 9);
    spnSuboptimal->setValue(0);
    spnSuboptimal->setFixedWidth(48);
    spnSuboptimal->setFixedHeight(26);
    spnSuboptimal->setToolTip("Extra moves beyond optimal to *also* find (0 = optimal only).");

    QWidget *allOptRow = new QWidget();
    allOptRow->setFixedHeight(28);
    QHBoxLayout *allOptLayout = new QHBoxLayout(allOptRow);
    allOptLayout->setContentsMargins(0, 0, 0, 0);
    allOptLayout->setSpacing(4);
    allOptLayout->addWidget(chkAllOptimal);
    allOptLayout->addStretch(1);
    QLabel *lblSuboptLabel = new QLabel("+suboptimal:");
    lblSuboptLabel->setObjectName("lblSuboptLabel");
    allOptLayout->addWidget(lblSuboptLabel);
    allOptLayout->addWidget(spnSuboptimal);

    chkDepths = new TightCheckBox("Specific depths:");
    chkDepths->setToolTip("Search only the listed move depths instead of starting from 0 and going up.\n"
                          "Comma-separated, e.g.\"8,9\". \n"
                          "Write in the input box to toggle it on.");
    chkDepths->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    chkDepths->setFocusPolicy(Qt::NoFocus);

    txtDepths = new QLineEdit();
    txtDepths->setFixedWidth(80);
    txtDepths->setFixedHeight(26);
    txtDepths->setPlaceholderText("e.g. 8,9");
    txtDepths->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[0-9,]*"), txtDepths));
    txtDepths->setToolTip("Comma-separated list of depths to search, e.g. \"8,9\"");
    chkDepths->setStyleSheet("QCheckBox { color: #707090; } QCheckBox::indicator { opacity: 0.5; }");
    chkDepths->setCursor(Qt::ArrowCursor);

    chkGenerator = new TightCheckBox("Generator alg");
    chkGenerator->setToolTip("If selected, generated algs will set up to the case from a solved cube,\n"
                             "else the algs will solve the case.");

    chk2gen = new TightCheckBox("2Gen  (top layer + slices only)");
    chk2gen->setToolTip("Restrict to 2-gen moves: top-layer turns and slices only.\n"
                        "Requires the bottom left pieces to already be solved.\n"
                        "You cannot demand both 2-gen and stay-in-cubeshape.");

    chkPseudo2gen = new TightCheckBox("Pseudo 2Gen  (bottom: ±1 only)");
    chkPseudo2gen->setToolTip("Restrict bottom-layer turns to ±1 only (2-gen with bottom 1 moves).\n");

    chkCubeshape = new TightCheckBox("Stay in cubeshape");
    chkCubeshape->setToolTip("Only generate algs that keep the puzzle in cubeshape throughout.");

    chkIgnoreMid = new TightCheckBox("Ignore middle layer");
    chkIgnoreMid->setToolTip("Ignore bar states. Equivalent to clicking on the bar until it is gray.");

    chkKarnotation = new TightCheckBox("Karnotation output");
    chkKarnotation->setToolTip("Display solutions in karnotation instead of WCA notation.");

    chkSpecificAngle = new TightCheckBox("Generate alg from this specific angle");
    chkSpecificAngle->setObjectName("chkSpecificAngle");
    chkSpecificAngle->setToolTip("Generate algs from this angle and this angle only.\n"
                                 "Essentially restricting the move before the first slice to 1 moves only.");

    chkMaxX = new TightCheckBox("Max top turn:");
    chkMaxX->setToolTip("Limit the maximum top-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"4\", that means algs can do -4 to 4 on top.");
    spnMaxX = new QSpinBox();
    spnMaxX->setRange(0, 6);
    spnMaxX->setValue(3);
    spnMaxX->setFixedWidth(48);
    spnMaxX->setFixedHeight(26);
    spnMaxX->setToolTip("Maximum top-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"4\", that means algs can do -4 to 4 on top.");

    chkMaxY = new TightCheckBox("Max bottom turn:");
    chkMaxY->setToolTip("Limit the maximum bottom-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"3\", that means algs can do -3 to 3 on bottom.");
    spnMaxY = new QSpinBox();
    spnMaxY->setRange(0, 6);
    spnMaxY->setValue(3);
    spnMaxY->setFixedWidth(48);
    spnMaxY->setFixedHeight(26);
    spnMaxY->setToolTip("Maximum bottom-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"3\", that means algs can do -3 to 3 on bottom.");

    chkMaxTotal = new TightCheckBox("Max total turn:");
    chkMaxTotal->setToolTip("Limit the maximum combined |top|+|bottom| turn per move pair (1–12).\n"
                            "e.g. if you put \"6\", (3,-3) is allowed in algs (3+3<=6),\n"
                            "but (-5,-2) is not (5+2>6).");
    spnMaxTotal = new QSpinBox();
    spnMaxTotal->setRange(1, 12);
    spnMaxTotal->setValue(6);
    spnMaxTotal->setFixedWidth(48);
    spnMaxTotal->setFixedHeight(26);
    spnMaxTotal->setToolTip("Maximum combined |top|+|bottom| turn per move pair (1–12).\n"
                            "e.g. if you put \"6\", (3,-3) is allowed in algs (3+3<=6),\n"
                            "but (-5,-2) is not (5+2>6).");

    chkSlice->setChecked(true);
    chkKarnotation->setChecked(true);

    // ── Grid layout ──────────────────────────────────────────────────────────
    // Wrap full-width checkboxes in a container so they only occupy
    // natural width — prevents tooltip firing on empty space to the right.
    auto wrapCb = [](QCheckBox *cb) -> QWidget* {
        QWidget *w = new QWidget();
        w->setToolTip(""); // no tooltip on the spacer area
        QHBoxLayout *l = new QHBoxLayout(w);
        l->setContentsMargins(0,0,0,0);
        l->setSpacing(0);
        l->addWidget(cb);
        l->addStretch();
        return w;
    };

    int row = 0;
    grid->addWidget(wrapCb(chkSlice), row++, 0, 1, 2);
    grid->addWidget(allOptRow, row++, 0, 1, 2);
    grid->addWidget(chkDepths, row, 0);
    grid->addWidget(txtDepths, row++, 1);
    grid->addWidget(wrapCb(chkGenerator), row++, 0, 1, 2);
    grid->addWidget(wrapCb(chk2gen), row++, 0, 1, 2);
    grid->addWidget(wrapCb(chkPseudo2gen), row++, 0, 1, 2);
    grid->addWidget(wrapCb(chkCubeshape), row++, 0, 1, 2);
    grid->addWidget(wrapCb(chkIgnoreMid), row++, 0, 1, 2);
    grid->addWidget(wrapCb(chkSpecificAngle), row++, 0, 1, 2);
    grid->addWidget(chkMaxX, row, 0);
    grid->addWidget(spnMaxX, row++, 1);
    grid->addWidget(chkMaxY, row, 0);
    grid->addWidget(spnMaxY, row++, 1);
    grid->addWidget(chkMaxTotal, row, 0);
    grid->addWidget(spnMaxTotal, row++, 1);

    for (int r = 0; r < row; r++)
        grid->setRowMinimumHeight(r, 28);

    QScrollArea *optionsScroll = new QScrollArea();
    optionsScroll->setWidget(optionsInner);
    optionsScroll->setWidgetResizable(true);
    optionsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    optionsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    optionsScroll->setFrameShape(QFrame::NoFrame);
    optionsScroll->setMinimumHeight(120);
    grpOptions->setLayout(new QVBoxLayout());
    grpOptions->layout()->setContentsMargins(4, 16, 4, 4);
    grpOptions->layout()->addWidget(optionsScroll);

    // ── Connections ──────────────────────────────────────────────────────────
    auto upd = [this]
    { updateConstraints(); updateCommand(); };

    connect(chkSlice, &QCheckBox::toggled, this, upd);
    connect(chkAllOptimal, &QCheckBox::toggled, this, upd);
    connect(spnSuboptimal, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkDepths, &QCheckBox::clicked, this, [this](bool) {
        // Clicking directly is not allowed — state is driven by txtDepths content
        QString t = txtDepths->text().trimmed();
        bool valid = !t.isEmpty() && QRegularExpression("^[0-9]+(,[0-9]+)*$").match(t).hasMatch();
        chkDepths->blockSignals(true);
        chkDepths->setChecked(valid);
        chkDepths->blockSignals(false);
    });
    connect(txtDepths, &QLineEdit::textChanged, this, [this, upd](const QString &text) {
        QString t = text.trimmed();
        // Valid = non-empty and only digits and commas, at least one digit
        bool valid = !t.isEmpty() && QRegularExpression("^[0-9]+(,[0-9]+)*$").match(t).hasMatch();
        chkDepths->blockSignals(true);
        chkDepths->setChecked(valid);
        chkDepths->blockSignals(false);
        upd();
    });
    connect(chkGenerator, &QCheckBox::toggled, this, upd);
    connect(chk2gen, &QCheckBox::toggled, this, upd);
    connect(chkPseudo2gen, &QCheckBox::toggled, this, upd);
    connect(chkCubeshape, &QCheckBox::toggled, this, upd);
    connect(chkIgnoreMid, &QCheckBox::toggled, this, upd);
    connect(chkKarnotation, &QCheckBox::toggled, this, [this, upd](bool /*checked*/) {
        upd();
        // Rebuild views from cache — no re-solve needed
        if (!m_rawLines.isEmpty()) {
            if (m_tableVisible)
                rebuildTable();
            else if (chkRankErgo->isChecked())
                onRankErgoToggled(true);
            else
                rebuildTerminalView();
        }
    });
    connect(chkSpecificAngle, &QCheckBox::toggled, this, upd);
    connect(chkMaxX, &QCheckBox::toggled, this, upd);
    connect(spnMaxX, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxY, &QCheckBox::toggled, this, upd);
    connect(spnMaxY, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxTotal, &QCheckBox::toggled, this, upd);
    connect(spnMaxTotal, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);

    allOptRow->setToolTip("");

    // ── Pack options/command/solve/progress into one hideable wrapper ─────────
    m_topSection = new QWidget();
    QVBoxLayout *topLay = new QVBoxLayout(m_topSection);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->setSpacing(6);
    topLay->addWidget(grpOptions);

    txtCommand = new QLineEdit();
    txtCommand->setReadOnly(false);
    txtCommand->setObjectName("txtCommand");
    txtCommand->setVisible(false);

    btnCopy = new QPushButton("⎘");
    btnCopy->setObjectName("btnCopy");
    btnCopy->setFixedWidth(32);
    btnCopy->setFixedHeight(24);
    btnCopy->setToolTip("Copy command");
    btnCopy->setVisible(false);

    // Enter/Shift+Enter are handled in eventFilter; returnPressed is not used.

    m_mainInput->installEventFilter(this);

    connect(btnApply, &QPushButton::clicked, this, [this]
            {
                const QString text = m_mainInput->text().trimmed();
                if (text.isEmpty() && m_inputModeIndex == 2) return;

                if (m_inputModeIndex == 2) {
                    pushUndoState();
                    bool ok = cubeWidget->setPositionFromString(text);
                    if (!ok) {
                        m_mainInput->setProperty("hasError", true);
                        style()->polish(m_mainInput);
                        m_undoStack.removeLast();
                        btnUndo->setEnabled(!m_undoStack.isEmpty());
                    } else {
                        m_mainInput->setProperty("hasError", false);
                        style()->polish(m_mainInput);
                        updateCommand();
                    }
                    return;
                }

                pushUndoState();

                if (m_applyFromSolved) {
                    cubeWidget->reset();
                }

                QString raw = text.trimmed().isEmpty() ? "0,0" : text;
                bool leadingSlash  = raw.trimmed().startsWith('/') || raw.trimmed().startsWith('\\');
                bool trailingSlash = raw.trimmed().endsWith('/')   || raw.trimmed().endsWith('\\');
                if (m_inputModeIndex == 1) {
                    raw = invertScrambleStr(raw);
                }

                // addCommas rules
                {
                    raw.replace('/', ' ').replace('\\', ' ');
                    raw = raw.simplified();
                    QStringList tokens = raw.split(' ', Qt::SkipEmptyParts);
                    for (QString &tok : tokens) {
                        QString t = tok;
                        t.remove('('); t.remove(')');
                        if (t.isEmpty() || t.contains(',')) continue;
                        bool allNumeric = true;
                        for (QChar ch : t)
                            if (ch != '-' && !ch.isDigit()) { allNumeric = false; break; }
                        if (!allNumeric) continue;
                        if (t.size() == 1)
                            tok = t + ",0";
                        else if (t.size() == 2)
                            tok = (t[0] == '-') ? t + ",0"
                                                : QString(t[0]) + "," + t[1];
                        else if (t.size() == 3)
                            tok = (t[0] == '-') ? t.left(2) + "," + t[2]
                                                : QString(t[0]) + "," + t.mid(1);
                        else if (t.size() == 4)
                            tok = t.left(2) + "," + t.mid(2);
                    }
                    raw = tokens.join(' ');
                }
                // Step 3: unkarnify — KARN_TO_WCA dict replacement + shorthand resolution.
                // replaceShorthands(unkarnifyHelp(s)) mirrors the JS step 8 pipeline:
                //   unkarnifyHelp applies KARN_TO_WCA on the space-separated token stream
                //   and normalises to slash-separated output;
                //   replaceShorthands then resolves alignment-dependent shorthand tokens
                //   (bjj, fv10, kk0-1, …) which are not in KARN_TO_WCA directly.
                {
                    std::string s = raw.toStdString();
                    if (leadingSlash && s.front() != '/') s = "/" + s;
                    if (trailingSlash && s.back() != '/') s = s + "/";
                    s = replaceShorthands(unkarnifyHelp(s));
                    raw = QString::fromStdString(s);
                }

                struct Move { bool isSlice; int x, y; };
                QVector<Move> moves;
                bool ok = true;

                QStringList segments = raw.split('/');
                for (int si = 0; si < segments.size() && ok; si++) {
                    QString seg = segments[si].trimmed();
                    seg.remove('('); seg.remove(')');

                    if (si > 0) moves.append({true, 0, 0});

                    if (seg.isEmpty()) continue;

                    if (seg.contains(',')) {
                        QStringList parts = seg.split(',');
                        if (parts.size() != 2) { ok = false; break; }
                        bool ok1, ok2;
                        int x = parts[0].trimmed().toInt(&ok1);
                        int y = parts[1].trimmed().toInt(&ok2);
                        if (!ok1 || !ok2) { ok = false; break; }
                        moves.append({false, x, y});
                    } else {
                        bool ok1;
                        int x = seg.toInt(&ok1);
                        if (!ok1) { ok = false; break; }
                        moves.append({false, x, 0});
                    }
                }

                if (!ok) {
                    m_mainInput->setProperty("hasError", true);
                    style()->polish(m_mainInput);
                    m_undoStack.removeLast();
                    btnUndo->setEnabled(!m_undoStack.isEmpty());
                    return;
                }

                int pos[24] = {};
                int mid = 0;
                {
                    std::string s = cubeWidget->getPositionString().toStdString();
                    int j = 0, nextPC = -3, nextPE = 18;
                    for (int i = 0; i < 16 && j < 24; i++) {
                        int k = (unsigned char)s[i];
                        if (k >= 'a' && k <= 'z') k += ('A'-'a');
                        if      (k>='A'&&k<='H') k-='A';
                        else if (k>='1'&&k<='8') k-=('1'-8);
                        else if (k=='U'||k=='V') { k=nextPC; nextPC-=3; }
                        else if (k=='W')         { k=nextPC; nextPC-=3; }
                        else if (k=='X'||k=='Y') { k=nextPE; nextPE+=3; }
                        else if (k=='Z')         { k=nextPE; nextPE+=3; }
                        pos[j++]=k;
                        if (k>=0&&k<8) pos[j++]=k;
                    }
                    mid = (s.size()>=17) ? (s[16]=='/'?1:0) : (!s.empty()&&s.back()=='/'?1:0);
                }

                auto doTop = [&](int m){ m=((m%12)+12)%12; for(int mv=0;mv<m;mv++){ int c=pos[11]; for(int i=11;i>0;i--) pos[i]=pos[i-1]; pos[0]=c; } };
                auto doBot = [&](int m){ m=((m%12)+12)%12; for(int mv=0;mv<m;mv++){ int c=pos[23]; for(int i=23;i>12;i--) pos[i]=pos[i-1]; pos[12]=c; } };
                auto canSlice = [&](){ return pos[0]!=pos[11]&&pos[5]!=pos[6]&&pos[12]!=pos[23]&&pos[17]!=pos[18]; };
                auto doSlice = [&](){ if(!canSlice()) return; for(int i=6;i<12;i++) std::swap(pos[i],pos[i+6]); mid=1-mid; };

                for (const Move& mv : moves) {
                    if (mv.isSlice) doSlice();
                    else { doTop(mv.x); doBot(mv.y); }
                }

                // ── Sliceability guard ────────────────────────────────────────────────
                // The resulting position must be sliceable: no corner piece can straddle
                // the cut line. canSlice() checks the four boundary index pairs (0/11
                // and 5/6 on top; 12/23 and 17/18 on bottom) — if any pair holds the
                // same corner value, that piece is split across the cut.
                if (!canSlice()) {
                    const QString errMsg =
                        "Position after applying this alg is not sliceable — "
                        "a corner is split across the cut line. "
                        "The alg may be incomplete or incorrect.";
                    QToolTip::showText(
                        m_mainInput->mapToGlobal(QPoint(0, m_mainInput->height())),
                        errMsg, m_mainInput, {}, 4000);
                    m_mainInput->setProperty("hasError", true);
                    style()->polish(m_mainInput);
                    m_undoStack.removeLast();
                    btnUndo->setEnabled(!m_undoStack.isEmpty());
                    return;
                }

                const QString pieceChars = "ABCDEFGH12345678";
                QString posStr;
                for (int i = 0; i < 24; i++) {
                    posStr += pieceChars[pos[i]];
                    if (pos[i] < 8) i++;
                }
                posStr += (mid == 0 ? '-' : '/');

                bool applied = cubeWidget->setPositionFromString(posStr);
                if (!applied) {
                    m_mainInput->setProperty("hasError", true);
                    style()->polish(m_mainInput);
                    m_undoStack.removeLast();
                    btnUndo->setEnabled(!m_undoStack.isEmpty());
                } else {
                    m_mainInput->setProperty("hasError", false);
                    QToolTip::hideText();
                    style()->polish(m_mainInput);
                    updateCommand();
                } });

    connect(m_inputMode, &QPushButton::clicked, this, [this]
            {
                qDebug() << "inputMode clicked, new index:" << ((m_inputModeIndex + 1) % 3);
                m_inputModeIndex = (m_inputModeIndex + 1) % 3;
                if (m_inputModeIndex == 0) { m_inputMode->setText("SCRAMBLE"); m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); }
                else if (m_inputModeIndex == 1) { m_inputMode->setText("ALG");  m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); }
                else                           { m_inputMode->setText("POSITION"); m_mainInput->setPlaceholderText("ABCDEFGH12345678-"); }
                m_mainInput->clear();
                lblScrambleError->setVisible(false); });

    connect(m_inputModeArrow, &QPushButton::clicked, this, [this]
            {
                qDebug() << "inputModeArrow clicked";
                QMenu* menu = new QMenu(this);
                menu->setStyleSheet(QString(
                    "QMenu { background: %1; border: 1px solid %2; border-radius: 6px; padding: 4px; color: %3; font-size: 12px; }"
                    "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
                    "QMenu::item:selected { background: %4; }"
                    "QMenu::item:checked { color: %5; font-weight: bold; }"
                    ).arg(Theme::menuBg(), Theme::menuBorder(), Theme::fadingTooltipText(),
                          Theme::menuItemSelected(), Theme::menuItemChecked()));
                QAction* aScram = menu->addAction("Scramble");
                QAction* aAlg   = menu->addAction("Alg");
                QAction* aPos   = menu->addAction("Position");
                aScram->setCheckable(true); aScram->setChecked(m_inputModeIndex == 0);
                aAlg->setCheckable(true);   aAlg->setChecked(m_inputModeIndex == 1);
                aPos->setCheckable(true);   aPos->setChecked(m_inputModeIndex == 2);
                connect(aScram, &QAction::triggered, this, [this]{ m_inputModeIndex = 0; m_inputMode->setText("SCRAMBLE"); m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); m_mainInput->clear(); lblScrambleError->setVisible(false); });
                connect(aAlg,   &QAction::triggered, this, [this]{ m_inputModeIndex = 1; m_inputMode->setText("ALG");      m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); m_mainInput->clear(); lblScrambleError->setVisible(false); });
                connect(aPos,   &QAction::triggered, this, [this]{ m_inputModeIndex = 2; m_inputMode->setText("POSITION"); m_mainInput->setPlaceholderText("ABCDEFGH12345678-");                        m_mainInput->clear(); lblScrambleError->setVisible(false); });
                menu->exec(m_inputModeArrow->mapToGlobal(QPoint(0, m_inputModeArrow->height()))); });

    connect(m_mainInput, &QLineEdit::textChanged, this, [this](const QString &text)
            {
                lblScrambleError->setVisible(false);
                m_mainInput->setProperty("hasError", false);
                style()->polish(m_mainInput);
                if (m_inputModeIndex == 2) {
                    if (text.trimmed().isEmpty()) {
                        m_mainInput->setProperty("hasError", false);
                        style()->polish(m_mainInput);
                        return;
                    }
                    return;
                } else {
                    return; {
                        if (false) {

                            QString raw = text.trimmed();
                            if (m_inputModeIndex == 1) {
                                raw = invertScrambleStr(raw);
                            }

                            std::string karnStr = raw.toStdString();
                            bool hasAlpha = false;
                            for (char c : karnStr) if (std::isalpha((unsigned char)c)) { hasAlpha = true; break; }
                            if (hasAlpha) {
                                std::string converted = unkarnify(karnStr);
                                raw = QString::fromStdString(converted);
                            }

                            raw.replace('\\', '/');

                            struct Move { bool isSlice; int x, y; };
                            QVector<Move> moves;
                            bool ok = true;

                            QStringList segments = raw.split('/');
                            for (int si = 0; si < segments.size() && ok; si++) {
                                QString seg = segments[si].trimmed();
                                seg.remove('('); seg.remove(')');

                                if (si > 0) moves.append({true, 0, 0});

                                if (seg.isEmpty()) continue;

                                if (seg.contains(',')) {
                                    QStringList parts = seg.split(',');
                                    if (parts.size() != 2) { ok = false; break; }
                                    bool ok1, ok2;
                                    int x = parts[0].trimmed().toInt(&ok1);
                                    int y = parts[1].trimmed().toInt(&ok2);
                                    if (!ok1 || !ok2) { ok = false; break; }
                                    moves.append({false, x, y});
                                } else {
                                    bool ok1;
                                    int x = seg.toInt(&ok1);
                                    if (!ok1) { ok = false; break; }
                                    moves.append({false, x, 0});
                                }
                            }

                            if (!ok) {
                                m_mainInput->setProperty("hasError", true);
                                style()->polish(m_mainInput);
                                cubeWidget->reset();
                                updateCommand();
                                return;
                            }

                            int pos[24] = {};
                            int mid = 0;
                            {
                                std::string s = cubeWidget->getPositionString().toStdString();
                                int j = 0, nextPC = -3, nextPE = 18;
                                for (int i = 0; i < 16 && j < 24; i++) {
                                    int k = (unsigned char)s[i];
                                    if (k >= 'a' && k <= 'z') k += ('A'-'a');
                                    if      (k>='A'&&k<='H') k-='A';
                                    else if (k>='1'&&k<='8') k-=('1'-8);
                                    else if (k=='U'||k=='V') { k=nextPC; nextPC-=3; }
                                    else if (k=='W')         { k=nextPC; nextPC-=3; }
                                    else if (k=='X'||k=='Y') { k=nextPE; nextPE+=3; }
                                    else if (k=='Z')         { k=nextPE; nextPE+=3; }
                                    pos[j++]=k;
                                    if (k>=0&&k<8) pos[j++]=k;
                                }
                                mid = (s.size()>=17) ? (s[16]=='/'?1:0) : (!s.empty()&&s.back()=='/'?1:0);
                            }

                            auto doTop = [&](int m){ m=((m%12)+12)%12; for(int mv=0;mv<m;mv++){ int c=pos[11]; for(int i=11;i>0;i--) pos[i]=pos[i-1]; pos[0]=c; } };
                            auto doBot = [&](int m){ m=((m%12)+12)%12; for(int mv=0;mv<m;mv++){ int c=pos[23]; for(int i=23;i>12;i--) pos[i]=pos[i-1]; pos[12]=c; } };
                            auto canSlice = [&](){ return pos[0]!=pos[11]&&pos[5]!=pos[6]&&pos[12]!=pos[23]&&pos[17]!=pos[18]; };
                            auto doSlice = [&](){ if(!canSlice()) return; for(int i=6;i<12;i++) std::swap(pos[i],pos[i+6]); mid=1-mid; };

                            for (const Move& mv : moves) {
                                if (mv.isSlice) doSlice();
                                else { doTop(mv.x); doBot(mv.y); }
                            }

                            const QString pieceChars = "ABCDEFGH12345678";
                            QString posStr;
                            for (int i = 0; i < 24; i++) {
                                posStr += pieceChars[pos[i]];
                                if (pos[i] < 8) i++;
                            }
                            posStr += (mid == 0 ? '-' : '/');

                            bool applied = cubeWidget->setPositionFromString(posStr);
                            if (!applied) {
                                m_mainInput->setProperty("hasError", true);
                                style()->polish(m_mainInput);
                                cubeWidget->reset();
                            }
                            updateCommand();
                        } }
                } });

    lblCommandError = new QLabel("");
    lblCommandError->setObjectName("lblCommandError");
    lblCommandError->setWordWrap(true);
    lblCommandError->setVisible(false);
    topLay->addWidget(lblCommandError);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 0);
    progressBar->setVisible(false);
    progressBar->setFixedHeight(6);
    topLay->addWidget(progressBar);

    rightCol->addWidget(m_topSection);

    // Output stack wrapper with floating buttons
    QWidget *outputWrapper = new QWidget();
    outputWrapper->setMinimumHeight(120);
    QVBoxLayout *outputWrapperLay = new QVBoxLayout(outputWrapper);
    outputWrapperLay->setContentsMargins(0, 0, 0, 0);
    outputWrapperLay->setSpacing(0);

    txtOutput = new QTextEdit(outputWrapper);
    txtOutput->setReadOnly(true);
    txtOutput->setObjectName("txtOutput");
    txtOutput->setMinimumHeight(120);
    txtOutput->setStyleSheet("QTextEdit { background: #000000; }");
    txtOutput->document()->setDefaultStyleSheet("div, span { background: transparent !important; }");

    btnExpand = new QPushButton("⤢", outputWrapper);
    btnExpand->setObjectName("btnExpand");
    btnExpand->setFixedSize(22, 22);
    btnExpand->setToolTip("Expand terminal");

    btnCopyTerminal = new QPushButton("⎘", outputWrapper);
    btnCopyTerminal->setObjectName("btnCopyTerminal");
    btnCopyTerminal->setFixedSize(22, 22);
    btnCopyTerminal->setToolTip("Copy terminal contents");

    btnTableMode = new QPushButton("⊞", outputWrapper);
    btnTableMode->setObjectName("btnTableMode");
    btnTableMode->setFixedSize(22, 22);
    btnTableMode->setToolTip("Switch to table view");

    btnExpand->setVisible(false);
    btnCopyTerminal->setVisible(false);
    btnTableMode->setVisible(false);

    outputWrapperLay->addWidget(txtOutput);

    m_tableContainer = new QWidget();
    m_tableContainer->setVisible(false);
    m_tableContainer->setMinimumHeight(120);
    QVBoxLayout *tableLay = new QVBoxLayout(m_tableContainer);
    tableLay->setContentsMargins(0, 0, 0, 0);
    tableLay->setSpacing(4);

    m_solutionTable = new QTableWidget();
    m_solutionTable->setObjectName("m_solutionTable");
    m_solutionTable->setColumnCount(4);
    m_solutionTable->setHorizontalHeaderLabels({"#", "Solution", "Moves", "Slices"});
    m_solutionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_solutionTable->verticalHeader()->setVisible(false);
    m_solutionTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_solutionTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_solutionTable->setFocusPolicy(Qt::NoFocus);
    m_solutionTable->setShowGrid(false);
    m_solutionTable->setAlternatingRowColors(false);
    m_solutionTable->setTextElideMode(Qt::ElideNone);
    m_solutionTable->setContextMenuPolicy(Qt::NoContextMenu);
    m_solutionTable->viewport()->setCursor(Qt::ArrowCursor);
    tableLay->addWidget(m_solutionTable, 1);

    outputWrapperLay->addWidget(m_tableContainer);
    outputWrapperLay->addWidget(m_tableContainer);
    m_outputWrapper = outputWrapper;
    outputWrapper->installEventFilter(this);
    rightCol->addWidget(outputWrapper, 1);

    chkRankErgo = new TightCheckBox("Roughly rank algs based on relative ergonomics");
    chkRankErgo->setCursor(Qt::PointingHandCursor);
    chkRankErgo->setEnabled(false);
    chkRankErgo->setObjectName("chkRankErgo");
    rightCol->addWidget(chkKarnotation);
    rightCol->addWidget(chkRankErgo);

    lblStatus = new QLabel("");
    lblStatus->setObjectName("lblStatus");
    lblStatus->setVisible(false);

    root->addLayout(rightCol, 1);

    // ── Button connections ────────────────────────────────────────────────────
    connect(btnSolve, &QPushButton::clicked, this, &MainWindow::onSolveButtonClicked);
    connect(btnCopy, &QPushButton::clicked, this, &MainWindow::onCopy);
    connect(btnExpand, &QPushButton::clicked, this, &MainWindow::toggleExpand);
    connect(btnCopyTerminal, &QPushButton::clicked, this, [this]
            {
                QApplication::clipboard()->setText(txtOutput->toPlainText());
                appendStatusLine("Terminal copied to clipboard!"); });
    connect(btnTableMode, &QPushButton::clicked, this, [this]
            {
                m_tableVisible = !m_tableVisible;
                txtOutput->setVisible(!m_tableVisible);
                m_tableContainer->setVisible(m_tableVisible);
                btnTableMode->setText(m_tableVisible ? "▤" : "⊞");
                btnTableMode->setToolTip(m_tableVisible ? "Switch to terminal view" : "Switch to table view");
                if (m_tableVisible) rebuildTable();
                else if (chkRankErgo->isChecked()) onRankErgoToggled(true);
                else rebuildTerminalView(); });
    connect(chkRankErgo, &QCheckBox::toggled, this, &MainWindow::onRankErgoToggled);
    connect(txtCommand, &QLineEdit::textEdited, this, [this](const QString &text)
            {
                auto showCmdError = [this](const QString& msg) {
                    lblCommandError->setText(msg);
                    lblCommandError->setVisible(true);
                    txtCommand->setStyleSheet(
                        "QLineEdit#txtCommand { font-family: monospace; color: #ff5555; font-size: 12px; border-color: #ff5555; border-right: none; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; }");
                };
                auto clearCmdError = [this]() {
                    lblCommandError->setVisible(false);
                    txtCommand->setStyleSheet(
                        "QLineEdit#txtCommand { font-family: monospace; color: #7fdbff; font-size: 12px; border-right: none; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; }");
                };

                QStringList parts = text.trimmed().split(' ', Qt::SkipEmptyParts);
                if (parts.isEmpty()) { clearCmdError(); return; }

                QString pos = parts.last();

                if (pos.startsWith('-') || pos.contains(',')) {
                    showCmdError("No position string at end of command — last token looks like a flag.");
                    return;
                }
                if (pos.length() < 15 || pos.length() > 17) {
                    if (pos.length() > 0)
                        showCmdError(QString("Position string should be 15–17 characters, got %1.").arg(pos.length()));
                    else
                        clearCmdError();
                    return;
                }

                static const QString validChars = "ABCDEFGHabcdefgh12345678UVWXYZuvwxyz-/";
                bool validCharsOk = true;
                for (int ci = 0; ci < pos.length(); ci++) {
                    if (!validChars.contains(pos[ci])) {
                        validCharsOk = false;
                        break;
                    }
                }
                if (!validCharsOk) {
                    showCmdError("Invalid character in position string.");
                    return;
                }

                bool applied = cubeWidget->setPositionFromString(pos);
                if (!applied) {
                    showCmdError("Invalid position string — duplicate or unrecognised pieces.");
                    return;
                }
                clearCmdError();
                syncFlagsFromCommand(text); });

    QTimer::singleShot(0, this, [this]
                       {
                           int w = m_outputWrapper->width();
                           int margin = 6; int bw = 22;
                           btnExpand->move(w - margin - bw, margin);
                           btnTableMode->move(w - margin - bw*2 - 4, margin);
                           btnCopyTerminal->move(w - margin - bw*3 - 8, margin);
                           btnExpand->raise(); btnTableMode->raise(); btnCopyTerminal->raise(); });

    const auto allBtns = findChildren<QPushButton *>();
    for (auto *b : allBtns)
        b->setCursor(Qt::PointingHandCursor);
    const auto allChks = findChildren<QCheckBox *>();
    for (auto *c : allChks)
        c->setCursor(Qt::PointingHandCursor);
    m_solutionTable->setCursor(Qt::ArrowCursor);

    updateConstraints();
    rebuildTerminalView();
}
// -------------------------------------------------------
// toggleExpand — expand / shrink the output terminal
// -------------------------------------------------------
void MainWindow::toggleExpand()
{
    m_expanded = !m_expanded;
    m_topSection->setVisible(!m_expanded);
    m_leftPanel->setVisible(!m_expanded);
    if (m_expanded)
    {
        btnExpand->setText("⤡");
        btnExpand->setToolTip("Shrink terminal");
    }
    else
    {
        btnExpand->setText("⤢");
        btnExpand->setToolTip("Expand terminal");
    }
    // Rerender output/table with expanded styles
    if (m_tableVisible)
        rebuildTable();
    else if (chkRankErgo->isChecked())
        onRankErgoToggled(true);
    else
        rebuildTerminalView();
}

void MainWindow::rebuildTerminalView()
{
    txtOutput->clear();
    // Choose which line list to display
    const QStringList &lines = chkKarnotation->isChecked() ? m_karnLines : m_rawLines;
    if (lines.isEmpty())
    {
        QTextCursor cur(txtOutput->document());
        QTextCharFormat fmt;
        fmt.setForeground(QColor(m_lightTheme ? "#888899" : "#2a2a3a"));
        fmt.setFontItalic(false);
        fmt.setFontPointSize(10);
        fmt.setFontFamily("monospace");
        cur.insertText("solution will be displayed here...", fmt);
        return;
    }
    QTextCursor cur(txtOutput->document());
    int solIdx = 0;
    for (const QString &line : std::as_const(lines))
    {
        bool isSol = line.contains('[') && line.contains(']');
        if (!cur.atStart())
            cur.insertBlock();
        QTextCharFormat fmt;
        if (isSol)
        {
            bool isAlt = (solIdx % 2 == 1);
            QString col = m_lightTheme
                              ? (isAlt ? Theme::solutionAltLight(true) : Theme::solutionPrimary(true))
                              : (isAlt ? Theme::solutionAltLight(false) : Theme::textSolution(false));
            fmt.setForeground(QColor(col));
            fmt.setFontWeight(m_expanded ? QFont::Bold : QFont::Normal);
            fmt.setFontPointSize(m_expanded ? 13 : 10);
            QTextBlockFormat blkFmt;
            blkFmt.setLineHeight(m_expanded ? 180 : 120, QTextBlockFormat::ProportionalHeight);
            cur.setBlockFormat(blkFmt);
            solIdx++;
        }
        else
        {
            QString col = Theme::textMuted(m_lightTheme);
            fmt.setForeground(QColor(col));
            fmt.setFontWeight(QFont::Normal);
            fmt.setFontPointSize(m_expanded ? 11 : 10);
            QTextBlockFormat blkFmt;
            blkFmt.setLineHeight(m_expanded ? 150 : 120, QTextBlockFormat::ProportionalHeight);
            cur.setBlockFormat(blkFmt);
        }
        cur.insertText(line, fmt);
    }
    txtOutput->verticalScrollBar()->setValue(0);
}

// -------------------------------------------------------
// updateConstraints
// -------------------------------------------------------
void MainWindow::updateConstraints()
{
    const bool is2gen = chk2gen->isChecked();
    const bool isPseudo = chkPseudo2gen->isChecked();
    const bool isAllOpt = chkAllOptimal->isChecked();
    const bool isDepths = chkDepths->isChecked();

    // Auto-deselect "Specific depths" when the input field is cleared.
    if (isDepths && txtDepths->text().trimmed().isEmpty())
    {
        chkDepths->blockSignals(true);
        chkDepths->setChecked(false);
        chkDepths->blockSignals(false);
    }
    const bool isDepthsNow = chkDepths->isChecked(); // re-read after possible uncheck

    auto disableCheck = [](QCheckBox *cb)
    {
        cb->setEnabled(false);
        if (cb->isChecked())
        {
            cb->blockSignals(true);
            cb->setChecked(false);
            cb->blockSignals(false);
        }
    };

    if (is2gen)
        disableCheck(chkCubeshape);
    else
        chkCubeshape->setEnabled(true);

    if (chkCubeshape->isChecked())
        disableCheck(chk2gen);
    else if (!is2gen)
        chk2gen->setEnabled(true);

    if (is2gen)
        disableCheck(chkPseudo2gen);
    else
        chkPseudo2gen->setEnabled(true);

    if (isPseudo)
        disableCheck(chk2gen);
    else if (!chkCubeshape->isChecked())
        chk2gen->setEnabled(true);

    spnSuboptimal->setVisible(isAllOpt && !isDepthsNow);
    if (QLabel *lbl = findChild<QLabel *>("lblSuboptLabel"))
        lbl->setVisible(isAllOpt && !isDepthsNow);

    // txtDepths is always enabled so the user can click into it and activate the option.
    // Style it to look inactive when the checkbox is off.
    if (isDepthsNow)
    {
        txtDepths->setStyleSheet(""); // revert to global style
    }
    else
    {
        txtDepths->setStyleSheet(QString(
                                     "QLineEdit { color: %1; background: %2; border-color: %3; }")
                                     .arg(Theme::textSecondary(m_lightTheme),
                                          Theme::disabledBg(m_lightTheme),
                                          Theme::borderDark(m_lightTheme)));
    }

    spnMaxX->setEnabled(chkMaxX->isChecked());
    spnMaxY->setEnabled(chkMaxY->isChecked());
    spnMaxTotal->setEnabled(chkMaxTotal->isChecked());

    updateRankErgoState();
}

// -------------------------------------------------------
// buildStyles
// -------------------------------------------------------
void MainWindow::buildStyles()
{
    setStyleSheet(buildStyleSheet());
}

QString MainWindow::buildStyleSheet()
{
    return ::buildStyleSheet(m_lightTheme);
}

// -------------------------------------------------------
// buildArgList
// -------------------------------------------------------
QStringList MainWindow::buildArgList()
{
    QStringList args;

    if (chkSlice->isChecked())
        args << "-w";

    if (chkAllOptimal->isChecked())
    {
        const bool useNumber = !chkDepths->isChecked() && spnSuboptimal->value() > 0;
        args << (useNumber ? QString("-a%1").arg(spnSuboptimal->value()) : QString("-a"));
    }

    if (chkDepths->isChecked())
    {
        QString dv = txtDepths->text().trimmed().remove(' ');
        if (!dv.isEmpty())
            args << QString("-d%1").arg(dv);
    }

    if (chkGenerator->isChecked())
        args << "-g";
    if (chk2gen->isChecked())
        args << "-2";
    if (chkPseudo2gen->isChecked())
        args << "-p";
    if (chkCubeshape->isChecked())
        args << "-c";
    if (chkIgnoreMid->isChecked())
        args << "-m";
    if (chkSpecificAngle->isChecked())
        args << "-n";

    if (chkMaxX->isChecked())
        args << QString("-X%1").arg(spnMaxX->value());
    if (chkMaxY->isChecked())
        args << QString("-Y%1").arg(spnMaxY->value());
    if (chkMaxTotal->isChecked())
        args << QString("-Z%1").arg(spnMaxTotal->value());

    return args;
}

void MainWindow::syncFlagsFromCommand(const QString &text)
{
    QStringList parts = text.trimmed().split(' ', Qt::SkipEmptyParts);
    // Remove the executable name and position string (first and last tokens)
    if (parts.size() >= 1 && parts[0] == "sq1opt")
        parts.removeFirst();
    if (!parts.isEmpty() && !parts.last().startsWith('-'))
        parts.removeLast();

    auto has = [&](const QString &flag)
    {
        return parts.contains(flag);
    };
    auto hasPrefix = [&](const QString &prefix) -> QString
    {
        for (const QString &p : std::as_const(parts))
            if (p.startsWith(prefix) && p.length() > prefix.length())
                return p.mid(prefix.length());
        return QString();
    };

    // Block all signals while we sync so updateCommand isn't re-triggered
    auto block = [](QObject *o, bool b)
    { o->blockSignals(b); };

    block(chkSlice, true);
    chkSlice->setChecked(has("-w"));
    block(chkSlice, false);
    block(chkGenerator, true);
    chkGenerator->setChecked(has("-g"));
    block(chkGenerator, false);
    block(chk2gen, true);
    chk2gen->setChecked(has("-2"));
    block(chk2gen, false);
    block(chkPseudo2gen, true);
    chkPseudo2gen->setChecked(has("-p"));
    block(chkPseudo2gen, false);
    block(chkCubeshape, true);
    chkCubeshape->setChecked(has("-c"));
    block(chkCubeshape, false);
    block(chkIgnoreMid, true);
    chkIgnoreMid->setChecked(has("-m"));
    block(chkIgnoreMid, false);
    block(chkSpecificAngle, true);
    chkSpecificAngle->setChecked(has("-n"));
    block(chkSpecificAngle, false);

    // -a / -a<n>
    bool hasA = false;
    int subopt = 0;
    for (const QString &p : std::as_const(parts))
    {
        if (p == "-a")
        {
            hasA = true;
            subopt = 0;
            break;
        }
        if (p.startsWith("-a") && p.length() > 2)
        {
            bool ok;
            int v = p.sliced(2).toInt(&ok);
            if (ok)
            {
                hasA = true;
                subopt = v;
                break;
            }
        }
    }
    block(chkAllOptimal, true);
    chkAllOptimal->setChecked(hasA);
    block(chkAllOptimal, false);
    block(spnSuboptimal, true);
    spnSuboptimal->setValue(subopt);
    block(spnSuboptimal, false);

    // -d<list>
    QString dval = hasPrefix("-d");
    block(chkDepths, true);
    block(txtDepths, true);
    chkDepths->setChecked(!dval.isEmpty());
    txtDepths->setText(dval);
    block(chkDepths, false);
    block(txtDepths, false);

    // -X -Y -Z
    QString xv = hasPrefix("-X"), yv = hasPrefix("-Y"), zv = hasPrefix("-Z");
    block(chkMaxX, true);
    block(spnMaxX, true);
    block(chkMaxY, true);
    block(spnMaxY, true);
    block(chkMaxTotal, true);
    block(spnMaxTotal, true);
    chkMaxX->setChecked(!xv.isEmpty());
    if (!xv.isEmpty())
    {
        bool ok;
        int v = xv.toInt(&ok);
        if (ok)
            spnMaxX->setValue(v);
    }
    chkMaxY->setChecked(!yv.isEmpty());
    if (!yv.isEmpty())
    {
        bool ok;
        int v = yv.toInt(&ok);
        if (ok)
            spnMaxY->setValue(v);
    }
    chkMaxTotal->setChecked(!zv.isEmpty());
    if (!zv.isEmpty())
    {
        bool ok;
        int v = zv.toInt(&ok);
        if (ok)
            spnMaxTotal->setValue(v);
    }
    block(chkMaxX, false);
    block(spnMaxX, false);
    block(chkMaxY, false);
    block(spnMaxY, false);
    block(chkMaxTotal, false);
    block(spnMaxTotal, false);

    updateConstraints();
}

void MainWindow::updateCommand()
{
    QString pos = cubeWidget->getPositionString();
    QStringList args = buildArgList();
    txtCommand->setText("sq1opt " + args.join(" ") + " " + pos);
    lblCommandError->setVisible(false);
    QString cyan = Theme::textCyan(m_lightTheme);
    txtCommand->setStyleSheet(QString("QLineEdit#txtCommand { font-family: monospace; color: %1; font-size: 12px; border-right: none; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; }").arg(cyan));
}

// -------------------------------------------------------
// onSolveButtonClicked — single entry-point for the Solve/Stop button
// -------------------------------------------------------
void MainWindow::onSolveButtonClicked()
{
    if (worker && worker->isRunning())
        stopSolver();
    else
        onSolve();
}

// -------------------------------------------------------
// onSolve
// -------------------------------------------------------
void MainWindow::onSolve()
{
    if (worker && worker->isRunning())
        return;

    m_stopped = false;
    txtOutput->clear();
    m_rawLines.clear();
    m_karnLines.clear();
    m_solutionLines.clear();
    m_karnSolutionLines.clear();
    m_seenSolutions.clear();
    // Always go back to terminal view while solving
    m_tableVisible = false;
    txtOutput->setVisible(true);
    m_tableContainer->setVisible(false);
    btnTableMode->setText("⊞");
    btnTableMode->setToolTip("Switch to table view");
    chkRankErgo->blockSignals(true);
    chkRankErgo->setChecked(false);
    chkRankErgo->blockSignals(false);
    updateRankErgoState();

    txtOutput->clear();
    appendStatusLine("Solving…");

    // Swap Solve → Stop appearance (muted dark red, not alarming).
    btnSolve->setText("■  Stop");
    btnSolve->setStyleSheet(QString(
                                "QPushButton#btnSolve {"
                                "  background: %1; border: 1px solid %2; padding-top: 0px; padding-bottom: 0px;"
                                "  color: %3; font-size: 12px; font-weight: bold; }"
                                "QPushButton#btnSolve:hover { background: %4; }")
                                .arg(Theme::buttonStopBg(m_lightTheme), Theme::buttonStopBorder(m_lightTheme), Theme::buttonStopText(m_lightTheme), Theme::buttonStopHover(m_lightTheme)));

    progressBar->setVisible(true);

    worker = new SolverWorker();
    worker->positionStr = cubeWidget->getPositionString();
    m_posHex = worker->positionStr;
    worker->flags = buildArgList();
    m_cubeshapeWasActive = chkCubeshape->isChecked();
    connect(worker, &SolverWorker::lineReady, this, &MainWindow::onSolverLine, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished, this, &MainWindow::onSolverDone, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished, worker, &QObject::deleteLater);
    m_solveStartMs = QDateTime::currentMSecsSinceEpoch();
    m_firstSolutionMs = 0;
    m_hadFirstSolution = false;
    worker->start();
}

// -------------------------------------------------------
// stopSolver — kill the running process and flag m_stopped
// -------------------------------------------------------
void MainWindow::stopSolver()
{
    m_stopped = true;
    if (worker && worker->isRunning())
        worker->requestStop();
}

// -------------------------------------------------------
// onSolverLine
// -------------------------------------------------------
void MainWindow::onSolverLine(QString line)
{
    bool isSolution = line.contains('[') && line.contains(']');
    QString karnLine = line; // default: non-solution lines are unchanged

    if (isSolution)
    {
        // Dedup on the raw WCA alg key (before any display conversion)
        int bracketPos = line.indexOf('[');
        QString algKey = (bracketPos >= 0) ? line.left(bracketPos).trimmed() : line.trimmed();
        if (m_seenSolutions.contains(algKey))
            return;
        m_seenSolutions.insert(algKey);

        // Build the karnified version from the raw line
        karnLine = convertLine(line);

        // Cache both versions
        m_solutionLines.append(line);
        m_karnSolutionLines.append(karnLine);
    }

    // Cache into raw and karn line lists (non-solution lines are identical in both)
    m_rawLines.append(line);
    m_karnLines.append(karnLine);

    // Which version to display live?
    const QString &displayLine = isSolution && chkKarnotation->isChecked() ? karnLine : line;

    if (isSolution)
    {
        if (!m_hadFirstSolution)
        {
            m_hadFirstSolution = true;
            m_firstSolutionMs = QDateTime::currentMSecsSinceEpoch();
        }
        btnExpand->setVisible(true);
        btnCopyTerminal->setVisible(true);
        btnTableMode->setVisible(true);
        updateRankErgoState();
        {
            bool isAlt = (m_solutionLines.size() % 2 == 0);
            QString col = m_lightTheme
                              ? (isAlt ? Theme::solutionAltLight(true) : Theme::solutionPrimary(true))
                              : (isAlt ? Theme::solutionAltLight(false) : Theme::textSolution(false));
            QTextCursor cur = txtOutput->textCursor();
            cur.movePosition(QTextCursor::End);
            if (!txtOutput->document()->isEmpty())
                cur.insertBlock();
            QTextBlockFormat blkFmt;
            blkFmt.setLineHeight(m_expanded ? 180 : 120, QTextBlockFormat::ProportionalHeight);
            cur.setBlockFormat(blkFmt);
            QTextCharFormat fmt;
            fmt.setForeground(QColor(col));
            fmt.setFontWeight(m_expanded ? QFont::Bold : QFont::Normal);
            fmt.setFontPointSize(m_expanded ? 13 : 10);
            cur.insertText(displayLine, fmt);
            txtOutput->setTextCursor(cur);
        }
    }
    else
    {
        QString col = Theme::textMuted(m_lightTheme);
        QTextCursor cur = txtOutput->textCursor();
        cur.movePosition(QTextCursor::End);
        if (!txtOutput->document()->isEmpty())
            cur.insertBlock();
        QTextBlockFormat blkFmt;
        blkFmt.setLineHeight(m_expanded ? 150 : 120, QTextBlockFormat::ProportionalHeight);
        cur.setBlockFormat(blkFmt);
        QTextCharFormat fmt;
        fmt.setForeground(QColor(col));
        fmt.setFontPointSize(m_expanded ? 11 : 10);
        cur.insertText(displayLine, fmt);
        txtOutput->setTextCursor(cur);
    }
}

// -------------------------------------------------------
// onSolverDone
// -------------------------------------------------------
void MainWindow::onSolverDone(int code)
{
    progressBar->setVisible(false);

    // Restore Solve button appearance.
    btnSolve->setText("▶  Solve");
    btnSolve->setStyleSheet(""); // revert to stylesheet-defined look

    const int n = m_solutionLines.size();
    double secs = (QDateTime::currentMSecsSinceEpoch() - m_solveStartMs) / 1000.0;
    QString secsStr = QString::number(secs, 'f', 2);

    if (m_stopped)
    {
        QString summary = QString("Stopped — %1 solution%2 found in %3s.")
                              .arg(n)
                              .arg(n == 1 ? "" : "s")
                              .arg(secsStr);
        appendStatusLine(summary);
    }
    else if (code == 0)
    {
        QString summary = QString("Done — %1 solution%2 found in %3s.")
                              .arg(n)
                              .arg(n == 1 ? "" : "s")
                              .arg(secsStr);
        appendStatusLine(summary);
    }
    else
    {
        appendStatusLine("Error (code " + QString::number(code) + ")");
    }

    updateRankErgoState();
    if (!m_solutionLines.isEmpty())
    {
        qint64 elapsed = m_hadFirstSolution
                             ? (QDateTime::currentMSecsSinceEpoch() - m_firstSolutionMs)
                             : 3000;
        int delay = (elapsed < 3000) ? 400 : 0;
        QTimer::singleShot(delay, this, [this]
                           {
                               m_tableVisible = true;
                               txtOutput->setVisible(false);
                               m_tableContainer->setVisible(true);
                               btnTableMode->setText("▤");
                               btnTableMode->setToolTip("Switch to terminal view");
                               rebuildTable(); });
    }
}

// -------------------------------------------------------
// pushUndoState
// -------------------------------------------------------
void MainWindow::pushUndoState()
{
    m_undoStack.append({cubeWidget->getPositionString()});
    if (m_undoStack.size() > 64)
        m_undoStack.removeFirst();
    btnUndo->setEnabled(true);
    m_redoStack.clear();
    btnRedo->setEnabled(false);
}

// -------------------------------------------------------
// onReset
// -------------------------------------------------------
void MainWindow::onReset()
{
    updateRankErgoState();
    updateCommand();
}

// -------------------------------------------------------
// keyPressEvent — the global eventFilter handles all routing;
// this is kept only as a fallback for events that slip through.
// -------------------------------------------------------
void MainWindow::onApplyScramble()
{
    QString raw = txtScramble->text().trimmed();

    // Empty input = reset to solved
    if (raw.isEmpty())
    {
        QString before = cubeWidget->getPositionString();
        cubeWidget->reset();
        QString after = cubeWidget->getPositionString();
        if (before != after)
        {
            m_undoStack.append({before});
            if (m_undoStack.size() > 64)
                m_undoStack.removeFirst();
            btnUndo->setEnabled(true);
            m_redoStack.clear();
            btnRedo->setEnabled(false);
        }
        onReset();
        return;
    }

    pushUndoState();
    // Read current cube state from widget
    QString curPosStr = cubeWidget->getPositionString();
    // Parse it back into pos[] and mid using the same logic as setPositionFromString
    int pos[24] = {};
    int mid = 0;
    {
        std::string s = curPosStr.toStdString();
        int j = 0;
        int nextPartialCorner = -3;
        int nextPartialEdge = 18;
        for (int i = 0; i < 16 && j < 24; i++)
        {
            int k = (unsigned char)s[i];
            if (k >= 'a' && k <= 'z')
                k += ('A' - 'a');
            if (k >= 'A' && k <= 'H')
                k -= 'A';
            else if (k >= '1' && k <= '8')
                k -= ('1' - 8);
            else if (k == 'U')
            {
                k = nextPartialCorner;
                nextPartialCorner -= 3;
            }
            else if (k == 'V')
            {
                k = nextPartialCorner;
                nextPartialCorner -= 3;
            }
            else if (k == 'W')
            {
                k = nextPartialCorner;
                nextPartialCorner -= 3;
            }
            else if (k == 'X')
            {
                k = nextPartialEdge;
                nextPartialEdge += 3;
            }
            else if (k == 'Y')
            {
                k = nextPartialEdge;
                nextPartialEdge += 3;
            }
            else if (k == 'Z')
            {
                k = nextPartialEdge;
                nextPartialEdge += 3;
            }
            pos[j++] = k;
            if (k >= 0 && k < 8)
                pos[j++] = k; // corner occupies two slots
        }
        if (s.size() >= 17)
            mid = (s[16] == '/') ? 1 : 0;
        else if (s.size() == 16)
            mid = (s.back() == '/') ? 1 : 0;
        // for position strings without trailing char, mid stays 0
        if (s.size() < 16)
        {
            // fallback: check last char of the 15/16 char string
            char last = s.back();
            mid = (last == '/') ? 1 : 0;
        }
    }

    auto doTop = [&](int m)
    {
        m = ((m % 12) + 12) % 12;
        for (int moves = 0; moves < m; moves++)
        {
            int c = pos[11];
            for (int i = 11; i > 0; i--)
                pos[i] = pos[i - 1];
            pos[0] = c;
        }
    };
    auto doBot = [&](int m)
    {
        m = ((m % 12) + 12) % 12;
        for (int moves = 0; moves < m; moves++)
        {
            int c = pos[23];
            for (int i = 23; i > 12; i--)
                pos[i] = pos[i - 1];
            pos[12] = c;
        }
    };
    auto isSliceable = [&]()
    {
        return pos[0] != pos[11] && pos[5] != pos[6] &&
               pos[12] != pos[23] && pos[17] != pos[18];
    };
    auto doSlice = [&]()
    {
        if (!isSliceable())
            return;
        for (int i = 6; i < 12; i++)
            std::swap(pos[i], pos[i + 6]);
        mid = 1 - mid;
    };

    // ── Parse the move sequence correctly ────────────────────────────────────
    // The format is a series of moves separated by '/'.
    // Each '/' is a slice. A turn (x,y) precedes the slash that follows it.
    // Examples:
    //   "(1,0)/(3,3)"  → turn(1,0), slice, turn(3,3)          [no trailing slice]
    //   "(1,0)/(3,3)/" → turn(1,0), slice, turn(3,3), slice
    //   "/(1,0)"       → slice, turn(1,0)
    //   "/"            → slice only
    //
    // Algorithm: scan character by character, collecting turn tokens and
    // counting '/' separators explicitly so leading/trailing slashes are preserved.

    struct Move
    {
        bool isSlice;
        int x, y;
    };
    QVector<Move> moves;

    QString s = raw;
    int idx = 0;
    bool ok = true;

    // If the sequence starts with '/' it means a leading slice before any turn.
    // We'll handle this by treating the string as a sequence of:
    //   [optional leading /] (turn /) * [optional trailing turn]
    // We do a character-level parse.

    while (idx < s.size() && ok)
    {
        // Skip whitespace
        while (idx < s.size() && s[idx].isSpace())
            idx++;
        if (idx >= s.size())
            break;

        if (s[idx] == '/')
        {
            // Slash = slice
            moves.append({true, 0, 0});
            idx++;
        }
        else if (s[idx] == '(' || s[idx].isDigit() || s[idx] == '-')
        {
            // Turn: optional '(' x ',' y optional ')'
            if (s[idx] == '(')
                idx++;
            // parse x
            while (idx < s.size() && s[idx].isSpace())
                idx++;
            int sign = 1;
            if (idx < s.size() && s[idx] == '-')
            {
                sign = -1;
                idx++;
            }
            int num = 0;
            bool hasDigit = false;
            while (idx < s.size() && s[idx].isDigit())
            {
                num = num * 10 + (s[idx].toLatin1() - '0');
                idx++;
                hasDigit = true;
            }
            if (!hasDigit)
            {
                ok = false;
                break;
            }
            int x = sign * num;
            while (idx < s.size() && s[idx].isSpace())
                idx++;
            if (idx >= s.size() || s[idx] != ',')
            {
                ok = false;
                break;
            }
            idx++; // skip ','
            // parse y
            while (idx < s.size() && s[idx].isSpace())
                idx++;
            sign = 1;
            if (idx < s.size() && s[idx] == '-')
            {
                sign = -1;
                idx++;
            }
            num = 0;
            hasDigit = false;
            while (idx < s.size() && s[idx].isDigit())
            {
                num = num * 10 + (s[idx].toLatin1() - '0');
                idx++;
                hasDigit = true;
            }
            if (!hasDigit)
            {
                ok = false;
                break;
            }
            int y = sign * num;
            while (idx < s.size() && s[idx].isSpace())
                idx++;
            if (idx < s.size() && s[idx] == ')')
                idx++;
            moves.append({false, x, y});
        }
        else
        {
            ok = false;
            break;
        }
    }

    if (!ok)
    {
        int approxPos = idx;
        QString ctx = raw.mid(qMax(0, approxPos - 6), 12).trimmed();
        QString msg = QString("Parse error near \"%1\" (col %2) — expected (x,y) or /.")
                          .arg(ctx)
                          .arg(approxPos);
        lblScrambleError->setText(msg);
        lblScrambleError->setVisible(true);
        txtScramble->setProperty("hasError", true);
        style()->polish(txtScramble);
        return;
    }

    // ── Optionally invert if "Input algorithm" mode ───────────────────────────
    if (m_scrambleIsAlg)
    {
        // Invert: reverse the move list and negate all turns
        QVector<Move> inv;
        for (int i = moves.size() - 1; i >= 0; i--)
        {
            Move mv = moves[i];
            if (!mv.isSlice)
            {
                mv.x = -mv.x;
                mv.y = -mv.y;
            }
            inv.append(mv);
        }
        moves = inv;
    }

    // ── Apply moves ───────────────────────────────────────────────────────────
    for (const Move &mv : std::as_const(moves))
    {
        if (mv.isSlice)
            doSlice();
        else
        {
            doTop(mv.x);
            doBot(mv.y);
        }
    }

    // Build position string
    const QString pieceChars = "ABCDEFGH12345678";
    QString posStr;
    for (int i = 0; i < 24; i++)
    {
        posStr += pieceChars[pos[i]];
        if (pos[i] < 8)
            i++;
    }
    posStr += (mid == 0 ? '-' : '/');

    bool applied = cubeWidget->setPositionFromString(posStr);
    if (!applied)
    {
        lblScrambleError->setText("Resulting position is invalid — check your move sequence.");
        lblScrambleError->setVisible(true);
        txtScramble->setProperty("hasError", true);
        style()->polish(txtScramble);
        return;
    }
    // Clear any previous error
    lblScrambleError->setVisible(false);
    txtScramble->setProperty("hasError", false);
    style()->polish(txtScramble);
    updateCommand();
}

void MainWindow::appendStatusLine(const QString &msg)
{
    QString col = Theme::textTerminal(m_lightTheme);
    QTextCursor cur = txtOutput->textCursor();
    cur.movePosition(QTextCursor::End);
    if (!txtOutput->document()->isEmpty())
        cur.insertBlock();
    QTextCharFormat fmt;
    fmt.setForeground(QColor(col));
    fmt.setFontItalic(true);
    fmt.setFontPointSize(m_expanded ? 11 : 9);
    QTextBlockFormat blkFmt;
    blkFmt.setLineHeight(120, QTextBlockFormat::ProportionalHeight);
    cur.setBlockFormat(blkFmt);
    cur.insertText(msg, fmt);
    txtOutput->setTextCursor(cur);
}

void MainWindow::rebuildTable()
{
    const bool ergo = chkRankErgo->isChecked();
    const bool showErgo = m_cubeshapeWasActive;
    m_solutionTable->setColumnCount(showErgo ? 5 : 4);
    m_solutionTable->setHorizontalHeaderLabels(
        showErgo ? QStringList{"#", "Solution", "Moves", "Slices", "Ergo"}
                 : QStringList{"#", "Solution", "Moves", "Slices"});
    m_solutionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_solutionTable->setColumnWidth(0, 72);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    if (showErgo)
        m_solutionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_solutionTable->setRowCount(0);
    if (m_solutionLines.isEmpty())
        return;

    auto parseCounts = [](const QString &line, int &moves, int &slices)
    {
        moves = 0; slices = 0;
        int lb = line.lastIndexOf('[');
        int rb = line.lastIndexOf(']');
        if (lb < 0 || rb < 0) return;
        QString bracket = line.mid(lb + 1, rb - lb - 1);
        QStringList parts = bracket.split('|');
        if (parts.size() >= 2) {
            slices = parts[0].trimmed().toInt();
            moves  = parts[1].trimmed().toInt();
        }
    };

    auto stripBracket = [](const QString &line) -> QString
    {
        int lb = line.lastIndexOf('[');
        return lb > 0 ? line.left(lb).trimmed() : line.trimmed();
    };

    struct Row { QString alg; int moves; int slices; double ergo; };
    QVector<Row> rows;

    bool useKarn = chkKarnotation->isChecked();
    const QStringList &displayLines = useKarn ? m_karnSolutionLines : m_solutionLines;

    if (showErgo) {
        // rateAndSort always rates on raw numeric; display lines used only for alg text
        auto rated = rateAndSort(m_solutionLines, m_posHex, useKarn);
        // Build map from raw alg key -> display alg text
        QMap<QString, QString> displayAlgMap;
        for (int i = 0; i < m_solutionLines.size() && i < displayLines.size(); i++) {
            int lb = m_solutionLines[i].lastIndexOf('[');
            QString key = lb > 0 ? m_solutionLines[i].left(lb).trimmed() : m_solutionLines[i].trimmed();
            int dlb = displayLines[i].lastIndexOf('[');
            displayAlgMap[key] = dlb > 0 ? displayLines[i].left(dlb).trimmed() : displayLines[i].trimmed();
        }
        for (auto &[line, score] : rated) {
            int mv, sl;
            parseCounts(line, mv, sl);
            int lb = line.lastIndexOf('[');
            QString rawKey = lb > 0 ? line.left(lb).trimmed() : line.trimmed();
            QString displayAlg = displayAlgMap.value(rawKey, rawKey);
            rows.append({displayAlg, mv, sl, score});
        }
    } else {
        // No ergo rating — build rows from display lines without calling rateAndSort
        for (const QString &line : std::as_const(displayLines)) {
            int mv, sl;
            parseCounts(line, mv, sl);
            int lb = line.lastIndexOf('[');
            QString alg = lb > 0 ? line.left(lb).trimmed() : line.trimmed();
            rows.append({alg, mv, sl, 0.0});
        }
    }

    if (ergo && showErgo)
        std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) {
            bool aNaN = std::isnan(a.ergo), bNaN = std::isnan(b.ergo);
            if (aNaN && bNaN) return false;
            if (aNaN) return false;
            if (bNaN) return true;
            return a.ergo > b.ergo;
        });
    else
        std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b){
            if (a.slices != b.slices) return a.slices < b.slices;
            return a.moves < b.moves;
        });

    const QColor rowA = QColor(Theme::rowAltDark(m_lightTheme));
    const QColor rowB = m_lightTheme ? rowA : QColor(Theme::rowAltLight(m_lightTheme));
    const QColor textCol    = QColor(Theme::textSolution(m_lightTheme));
    const QColor metaCol    = QColor(Theme::textSecondary(m_lightTheme));
    const int rowH    = m_expanded ? 36 : 24;
    const int fontSize = m_expanded ? 15 : 12;

    m_solutionTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); i++) {
        const Row &r = rows[i];
        QColor bg = (i % 2 == 0) ? rowA : rowB;

        auto cell = [&](int col, const QString &txt, bool isMeta = false) {
            QTableWidgetItem *item = new QTableWidgetItem(txt);
            item->setBackground(bg);
            item->setForeground(isMeta ? metaCol : textCol);
            item->setFlags(Qt::ItemIsEnabled);  // not selectable
            if (m_expanded) {
                QFont f = item->font();
                f.setPointSize(fontSize);
                item->setFont(f);
            }
            item->setTextAlignment(Qt::AlignCenter);
            m_solutionTable->setItem(i, col, item);
        };

        // SI no column — fixed font, no italic
        QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(i + 1));
        numItem->setBackground(bg);
        numItem->setForeground(metaCol);
        numItem->setFlags(Qt::ItemIsEnabled);
        numItem->setTextAlignment(Qt::AlignCenter);
        {
            QFont f = numItem->font();
            f.setPointSize(m_expanded ? fontSize - 2 : 10);
            f.setItalic(false);
            numItem->setFont(f);
        }
        m_solutionTable->setItem(i, 0, numItem);

        // Solution column: selectable QLabel
        QLabel *algLabel = new QLabel(r.alg);
        algLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        algLabel->setCursor(Qt::ArrowCursor);
        algLabel->setContentsMargins(4, 0, 4, 0);
        algLabel->setStyleSheet(QString("QLabel { background: %1; color: %2; %3 }")
            .arg(bg.name(),
                 textCol.name(),
                 m_expanded ? QString("font-size: %1pt;").arg(fontSize) : QString()));
        // Install event filter to show IBeam only when hovering over the text itself
        algLabel->installEventFilter(this);
        m_solutionTable->setCellWidget(i, 1, algLabel);

        cell(2, QString::number(r.moves), true);
        cell(3, QString::number(r.slices), true);
        if (showErgo) {
            if (std::isnan(r.ergo)) {
                // Rating failed for this alg — show a red warning icon instead of a score
                auto *warn = new QLabel("⚠");
                warn->setAlignment(Qt::AlignCenter);
                warn->setStyleSheet(QString(
                    "QLabel { color: #cc2020; background: %1; font-size: 14px; }")
                    .arg(bg.name()));
                m_solutionTable->setCellWidget(i, 4, warn);
            } else {
                cell(4, QString::number(r.ergo, 'f', 1), true);
            }
        }
        m_solutionTable->setRowHeight(i, rowH);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    int w = event->size().width();
    // Scale left panel: 320px base, grow a little above 1000px, cap at 400px
    int leftW = qBound(320, 320 + (w - 860) / 6, 400);
    leftScroll->setFixedWidth(leftW);
    if (auto *lc = leftScroll->widget())
        lc->setFixedWidth(leftW - 4);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
}

// -------------------------------------------------------
// onCopy
// -------------------------------------------------------
void MainWindow::onCopy()
{
    QApplication::clipboard()->setText(txtCommand->text());
    appendStatusLine("Copied to clipboard!");
}

// -------------------------------------------------------
// onRankErgoToggled
// -------------------------------------------------------
void MainWindow::onRankErgoToggled(bool checked)
{
    if (!checked)
    {
        rebuildTerminalView();
        appendStatusLine("Done.");
        if (m_tableVisible)
            rebuildTable();
        return;
    }
    if (m_solutionLines.isEmpty())
        return;
    // Ergo rating is only meaningful for cubeshape solves
    if (!m_cubeshapeWasActive)
        return;
    lblStatus->setText("Rating algorithms…");

    // Always rate on raw numeric lines; display alg in karn or raw per checkbox
    auto rated = rateAndSort(m_solutionLines, m_posHex, true);
    const bool useKarn = chkKarnotation->isChecked();

    // Build map from raw alg key -> display alg (karn or raw)
    const QStringList &displaySols = useKarn ? m_karnSolutionLines : m_solutionLines;
    QMap<QString, QString> displayAlgMap;
    for (int i = 0; i < m_solutionLines.size() && i < displaySols.size(); i++) {
        int lb = m_solutionLines[i].lastIndexOf('[');
        QString key = lb > 0 ? m_solutionLines[i].left(lb).trimmed() : m_solutionLines[i].trimmed();
        int dlb = displaySols[i].lastIndexOf('[');
        QString dispAlg = dlb > 0 ? displaySols[i].left(dlb).trimmed() : displaySols[i].trimmed();
        displayAlgMap[key] = dispAlg;
    }

    txtOutput->clear();
    QTextCursor cur(txtOutput->document());
    bool firstBlock = true;
    auto insertLine = [&](const QString &text, const QString &color, bool bold, int ptSize, int lineH)
    {
        if (!firstBlock)
            cur.insertBlock();
        firstBlock = false;
        QTextBlockFormat blkFmt;
        blkFmt.setLineHeight(lineH, QTextBlockFormat::ProportionalHeight);
        cur.setBlockFormat(blkFmt);
        QTextCharFormat fmt;
        fmt.setForeground(QColor(color));
        fmt.setFontWeight(bold ? QFont::Bold : QFont::Normal);
        fmt.setFontPointSize(ptSize);
        cur.insertText(text, fmt);
    };

    // Non-solution lines first (from the appropriate display list)
    const QStringList &displayLines = useKarn ? m_karnLines : m_rawLines;
    for (const QString &line : std::as_const(displayLines))
    {
        bool isSol = line.contains('[') && line.contains(']');
        if (!isSol)
        {
            QString col = Theme::textMuted(m_lightTheme);
            insertLine(line, col, false, m_expanded ? 11 : 10, m_expanded ? 150 : 120);
        }
    }

    // Rated solution lines (sorted by score, displayed in karn/raw per checkbox)
    int solIdx = 0;
    for (auto &[line, score] : rated)
    {
        int lb = line.lastIndexOf('[');
        QString rawKey = lb > 0 ? line.left(lb).trimmed() : line.trimmed();
        QString displayAlg = displayAlgMap.value(rawKey, rawKey);
        // Reconstruct bracket part from the rated line
        QString bracketPart = lb > 0 ? line.mid(lb) : QString();
        QString displayLine = displayAlg + (bracketPart.isEmpty() ? QString() : "  " + bracketPart.trimmed());

        bool isAlt = (solIdx % 2 == 1);
        QString col = m_lightTheme
                          ? (isAlt ? Theme::solutionAltLight(true) : Theme::solutionPrimary(true))
                          : (isAlt ? Theme::solutionAltLight(false) : Theme::textSolution(false));
        if (std::isnan(score)) {
            QString display = QString("%1  (⚠)").arg(displayLine);
            insertLine(display, "#cc2020", m_expanded, m_expanded ? 13 : 10, m_expanded ? 180 : 120);
        } else {
            QString display = QString("%1  (%2)").arg(displayLine).arg(score, 0, 'f', 2);
            insertLine(display, col, m_expanded, m_expanded ? 13 : 10, m_expanded ? 180 : 120);
        }
        solIdx++;
    }
    appendStatusLine(QString("Ranked %1 algs by ergonomics.").arg((int)rated.size()));
    txtOutput->verticalScrollBar()->setValue(0);
    if (m_tableVisible)
        rebuildTable();
}

// -------------------------------------------------------
// updateRankErgoState
// Decides whether chkRankErgo should be enabled, and sets
// a context-sensitive tooltip explaining why it's grayed out.
// -------------------------------------------------------
void MainWindow::updateRankErgoState()
{
    const bool cubeshapeActive = m_solutionLines.isEmpty() ? chkCubeshape->isChecked() : m_cubeshapeWasActive;
    const bool hasSolutions = !m_solutionLines.isEmpty();
    const bool solving = worker && worker->isRunning();
    const bool canRank = cubeshapeActive && hasSolutions && !solving;

    chkRankErgo->setEnabled(canRank);

    // Build tooltip: base description, then the relevant "why disabled" clause.
    QString tip = "Roughly rank algs based on relative ergonomics.";
    if (!cubeshapeActive)
        tip += "\n\nEnable 'Stay in cubeshape' to rank.";
    else if (!hasSolutions)
        tip += "\n\nRun the solver to rank algs.";
    chkRankErgo->setToolTip(tip);

    // If the checkbox was checked but the eligibility just dropped, uncheck and
    // restore the plain output so the user doesn't see a stale ranked view.
    if (!canRank && chkRankErgo->isChecked())
    {
        chkRankErgo->blockSignals(true);
        chkRankErgo->setChecked(false);
        chkRankErgo->blockSignals(false);
        onRankErgoToggled(false);
    }
}

// -------------------------------------------------------
// eventFilter — installed on qApp so it intercepts key events
// from every widget (spinboxes, txtDepths, etc.) before they
// are delivered.  Three responsibilities:
//   1. Ctrl+C  → stop solver if running.
//   2. Cube letter shortcuts → route to cubeWidget from any focus.
//   3. txtDepths digit → auto-enable "Specific depths" checkbox.
// -------------------------------------------------------
void MainWindow::showAboutModal()
{
    QWidget *central = this->centralWidget();

    QWidget *overlay = new QWidget(central);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background:rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    bool L = m_lightTheme;
    QString modalBg = Theme::primaryBg(L);
    QString modalBorder = Theme::borderGroup(L);
    QWidget *card = new QWidget(overlay);
    card->setObjectName("aboutCard");
    card->setFixedWidth(480);
    card->setStyleSheet(QString(
                            "QWidget#aboutCard { background:%1; border:1px solid %2; border-radius:10px; }")
                            .arg(modalBg, modalBorder));

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(10);

    QString textPrimary = Theme::textPrimary(L);
    QString textBody = Theme::textSecondary(L);
    QLabel *title = new QLabel("About Solve-A-Squan");
    title->setStyleSheet(QString("font-size:16px;font-weight:bold;color:%1;background:transparent;").arg(textPrimary));
    // Build the about body content with proper string building
    QLabel *body = new QLabel();
    body->setWordWrap(true);
    body->setTextFormat(Qt::RichText);
    body->setStyleSheet("background:transparent;");
    QString lnk = Theme::linkColor();
        QString aboutBody = QString(
                            "<span style='color:%1;font-size:12px;line-height:1.7;'>"
                            "This program stemmed from the optimal Square-1 solver by "
                            "<a href='https://www.jaapsch.net/puzzles/' style='color:%3;'>Jaap Scherphuis</a>."
                            "<br><br>"
                            "v2 was created by Michael Gottlieb "
                            "(<a href='https://github.com/qqwref' style='color:%3;'>GitHub</a>, "
                            "<a href='https://www.worldcubeassociation.org/persons/2006GOTT01' style='color:%3;'>WCA</a>), "
                            "who rewrote the solver with significant improvements and optimisations."
                            "<br><br>Read the old documentations <a href='read_old_docs' style='color:%3;'>here</a>. Note that it is largely not applicable within v3."
                            "<br><br>This is the official <b style='color:%4;'>v3</b>. New in v3:"
                            "<ul style='margin:4px 0 4px 16px;padding:0;color:%5;'>"
                            "<li>Actual graphical UI</li>"
                            "<li>Ability to generate a solution from a specific angle</li>"
                            "<li>Improved karnotation support</li>"
                            "<li>Algorithm ergonomics rater</li>"
                            "</ul>"
                            "v3 is created by "
                            "<a href='https://www.worldcubeassociation.org/persons/2024ASHR02' style='color:%3;'>Abid Ibn Ashraf</a>"
                            " and "
                            "<a href='https://www.worldcubeassociation.org/persons/2023MAOS01' style='color:%3;'>Matt Mao</a>."
                            "</span>")
                            .arg(textBody, QString(), lnk, textPrimary, Theme::textMuted(L));
    body->setText(aboutBody);
    // Enable clicking the in-text link to open the ReadDocs popup
    connect(body, &QLabel::linkActivated, this, [this](const QString &link)
            {
                if (link == "read_old_docs") {
                    showReadDocsPopup();
                } else {
                    QDesktopServices::openUrl(QUrl(link));
                } });

    lay->addWidget(title);
    lay->addWidget(body);

    card->show();
    card->adjustSize();

    auto centerCard = [overlay, card]()
    {
        overlay->setGeometry(overlay->parentWidget()->rect());
        int cx = (overlay->width() - card->width()) / 2;
        int cy = (overlay->height() - card->height()) / 2;
        card->move(cx, cy);
    };
    centerCard();
    card->raise();

    // Watch the centralWidget for resize events
    struct Filter : public QObject
    {
        QWidget *overlay;
        QWidget *card;
        std::function<void()> center;
        Filter(QWidget *o, QWidget *c, std::function<void()> fn)
            : QObject(o), overlay(o), card(c), center(fn) {}
        bool eventFilter(QObject *watched, QEvent *e) override
        {
            if (e->type() == QEvent::Resize && watched == overlay->parentWidget())
            {
                center();
                return false;
            }
            if (e->type() == QEvent::MouseButtonPress && watched == overlay)
            {
                QMouseEvent *me = static_cast<QMouseEvent *>(e);
                if (!card->geometry().contains(me->pos()))
                {
                    overlay->deleteLater();
                    return true;
                }
            }
            return false;
        }
    };

    Filter *f = new Filter(overlay, card, centerCard);
    central->installEventFilter(f); // watches parent for resize
    overlay->installEventFilter(f); // watches overlay for click-outside
}

// Load a document from docs/ directory, searching appDir first then CWD
QString MainWindow::loadDocText(const QString &fileName)
{
    auto readPath = [](const QString &p) -> QString
    {
        QFile f(p);
        if (!f.exists())
            return QString();
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString();
        QString s = QString::fromUtf8(f.readAll());
        f.close();
        return s;
    };

    QString appDir = QCoreApplication::applicationDirPath();
    QDir cwd = QDir::current();

    QList<QString> candidates;
    candidates << cwd.filePath("docs/" + fileName);
    candidates << appDir + "/docs/" + fileName;
    candidates << appDir + "/../docs/" + fileName;
    // ascend from current dir up to 6 levels looking for docs/
    QDir d = cwd;
    for (int i = 0; i < 6; ++i)
    {
        if (!d.cdUp())
            break;
        candidates << d.filePath("docs/" + fileName);
    }

    for (const QString &p : candidates)
    {
        QString s = readPath(p);
        if (!s.isEmpty())
            return s;
    }
    // Final hard fallback: root/docs if present
    QString rootLike = QDir::rootPath() + "/docs/" + fileName;
    QString s = readPath(rootLike);
    if (!s.isEmpty())
        return s;
    return QString();
}

// Read documents popup – shows sq1opt.txt with an inline link to the old-doc
void MainWindow::showReadDocsPopup()
{
    QWidget *central = this->centralWidget();
    QWidget *overlay = new QWidget(central);
    overlay->setObjectName("docsOverlay");
    overlay->setProperty("isDocsOverlay", true);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background:rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    bool L = m_lightTheme;
    QString modalBg = Theme::primaryBg(L);
    QString modalBorder = Theme::borderGroup(L);
    QString textColor = Theme::textMuted(L);
    QString titleColor = Theme::textPrimary(L);

    QWidget *card = new QWidget(overlay);
    card->setObjectName("docsCard");
    card->setStyleSheet(QString(
                            "QWidget#docsCard { background:%1; border:1px solid %2; border-radius:10px; }")
                            .arg(modalBg, modalBorder));

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 16, 24, 16);
    lay->setSpacing(6);

    QLabel *titleLbl = new QLabel("Sq1opt v2 Documentation");
    titleLbl->setStyleSheet(QString(
                                "font-size:14px;font-weight:bold;color:%1;background:transparent;")
                                .arg(titleColor));
    lay->addWidget(titleLbl);

    QString content = loadDocText("sq1opt.txt");
    if (content.isEmpty())
        content = "Could not load sq1opt.txt";

    // Build HTML line by line so text reflows to card width (no horizontal
    // scroll) while preserving leading-space indentation via &nbsp;.
    QString html;
    for (const QString &line : content.split('\n'))
    {
        QString esc = line.toHtmlEscaped();
        int spaces = 0;
        while (spaces < esc.size() && esc[spaces] == ' ')
            ++spaces;
        if (spaces > 0)
            esc = QString("&nbsp;").repeated(spaces) + esc.mid(spaces);
        esc.replace("sq1opt_old.txt",
                    "<a href='open_old_docs' style='color:#7abfe8;'>sq1opt_old.txt</a>");
        html += "<p style='margin:0;padding:0;'>" + esc + "</p>";
    }

    QTextBrowser *tb = new QTextBrowser(card);
    tb->setReadOnly(true);
    tb->setFrameShape(QFrame::NoFrame);
    tb->setOpenLinks(false);
    tb->setLineWrapMode(QTextEdit::WidgetWidth);
    tb->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    tb->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    tb->setStyleSheet(QString(
                          "QTextBrowser { background:transparent; border:none; color:%1; }")
                          .arg(textColor));
    tb->document()->setDefaultStyleSheet(
        QString("body, p { font-family:monospace; font-size:12px; color:%1; line-height:1.4; }"
                "a { color:#7abfe8; }")
            .arg(textColor));
    tb->setHtml("<body>" + html + "</body>");

    connect(tb, &QTextBrowser::anchorClicked, this, [this](const QUrl &url)
            {
                if (url.toString() == "open_old_docs") showOldDocsPopup();
                else QDesktopServices::openUrl(url); });

    lay->addWidget(tb, 1);

    card->show();
    auto centerCard = [overlay, card, central]()
    {
        overlay->setGeometry(overlay->parentWidget()->rect());
        int cw = qMin(640, central->width() - 40);
        int ch = qMin(520, central->height() - 40);
        card->setFixedSize(cw, ch);
        card->move((overlay->width() - card->width()) / 2,
                   (overlay->height() - card->height()) / 2);
    };
    centerCard();
    card->raise();

    // Event filter: resize re-centres the card; click OUTSIDE the card closes
    // all docs modals. The card-geometry check is critical — without it any
    // click that bubbles up from a non-interactive child would close the modal.
    struct F : public QObject
    {
        QWidget *overlay;
        QWidget *card;
        std::function<void()> fn;
        F(QWidget *o, QWidget *c, std::function<void()> f)
            : QObject(o), overlay(o), card(c), fn(f) {}
        bool eventFilter(QObject *w, QEvent *e) override
        {
            if (e->type() == QEvent::Resize && w == overlay->parentWidget())
            {
                fn();
                return false;
            }
            if (e->type() == QEvent::MouseButtonPress && w == overlay)
            {
                QMouseEvent *me = static_cast<QMouseEvent *>(e);
                if (!card->geometry().contains(me->pos()))
                    overlay->deleteLater();
                return true;
            }
            return false;
        }
    };
    F *f = new F(overlay, card, centerCard);
    central->installEventFilter(f); // watches parent for resize
    overlay->installEventFilter(f); // watches overlay for click-outside
}

// Old docs popup – shows sq1opt_old.txt
void MainWindow::showOldDocsPopup()
{
    QWidget *central = this->centralWidget();
    QWidget *overlay = new QWidget(central);
    overlay->setObjectName("docsOverlay");
    overlay->setProperty("isDocsOverlay", true);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background:rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    bool L = m_lightTheme;
    QString modalBg = Theme::primaryBg(L);
    QString modalBorder = Theme::borderGroup(L);
    QString textColor = Theme::textMuted(L);
    QString titleColor = Theme::textPrimary(L);

    QWidget *card = new QWidget(overlay);
    card->setObjectName("docsOldCard");
    card->setStyleSheet(QString(
                            "QWidget#docsOldCard { background:%1; border:1px solid %2; border-radius:10px; }")
                            .arg(modalBg, modalBorder));

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(24, 16, 24, 16);
    lay->setSpacing(6);

    QLabel *titleLbl = new QLabel("Sq1opt v1 Documentation");
    titleLbl->setStyleSheet(QString(
                                "font-size:14px;font-weight:bold;color:%1;background:transparent;")
                                .arg(titleColor));
    lay->addWidget(titleLbl);

    QTextBrowser *tb = new QTextBrowser(card);
    tb->setReadOnly(true);
    tb->setFrameShape(QFrame::NoFrame);
    tb->setOpenLinks(false);
    tb->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    tb->setStyleSheet(QString(
                          "QTextBrowser { background:transparent; border:none; color:%1; }")
                          .arg(textColor));
    tb->document()->setDefaultStyleSheet(
        QString("body, pre { font-family:monospace; font-size:12px; color:%1; }").arg(textColor));

    QString oldContent = loadDocText("sq1opt_old.txt");
    if (oldContent.isEmpty())
        tb->setPlainText("Could not load sq1opt_old.txt");
    else
        tb->setPlainText(oldContent);

    lay->addWidget(tb, 1);

    card->show();
    auto centerCard = [overlay, card, central]()
    {
        overlay->setGeometry(overlay->parentWidget()->rect());
        int cw = qMin(640, central->width() - 40);
        int ch = qMin(520, central->height() - 40);
        card->setFixedSize(cw, ch);
        card->move((overlay->width() - card->width()) / 2,
                   (overlay->height() - card->height()) / 2);
    };
    centerCard();
    card->raise();

    struct F : public QObject
    {
        QWidget *overlay;
        QWidget *card;
        std::function<void()> fn;
        F(QWidget *o, QWidget *c, std::function<void()> f)
            : QObject(o), overlay(o), card(c), fn(f) {}
        bool eventFilter(QObject *w, QEvent *e) override
        {
            if (e->type() == QEvent::Resize && w == overlay->parentWidget())
            {
                fn();
                return false;
            }
            if (e->type() == QEvent::MouseButtonPress && w == overlay)
            {
                QMouseEvent *me = static_cast<QMouseEvent *>(e);
                if (!card->geometry().contains(me->pos()))
                    overlay->deleteLater();
                return true;
            }
            return false;
        }
    };
    F *f = new F(overlay, card, centerCard);
    central->installEventFilter(f); // watches parent for resize
    overlay->installEventFilter(f); // watches overlay for click-outside
}

QString MainWindow::convertLine(const QString& rawLine)
{
    int lb = rawLine.lastIndexOf('[');
    int rb = rawLine.lastIndexOf(']');
    if (lb < 0 || rb < 0) return rawLine;

    QString algPart     = rawLine.left(lb).trimmed();
    QString bracketPart = rawLine.mid(lb).trimmed();

    std::string converted;
    if (m_smartKarn) {
        converted = karnifycs(algPart.toStdString(), m_posHex.toStdString(), chkGenerator->isChecked());
    } else {
        converted = karnify(algPart.toStdString());
    }
    return QString::fromStdString(converted) + "  " + bracketPart;
}

void MainWindow::applyTheme()
{
    setStyleSheet(buildStyleSheet());
    if (m_updateLogo)
        m_updateLogo();
    updateConstraints();
    QString termBg = Theme::secondaryBg(m_lightTheme);
    txtOutput->setStyleSheet(QString("QTextEdit { background: %1; }").arg(termBg));
    txtOutput->document()->setDefaultStyleSheet("div, span { background: transparent !important; }");
    if (!m_rawLines.isEmpty())
    {
        if (chkRankErgo->isChecked())
            onRankErgoToggled(true);
        else
            rebuildTerminalView();
    }
    if (m_tableVisible)
        rebuildTable();

    // Update command line color
    QString cyan = Theme::textCyan(m_lightTheme);
    txtCommand->setStyleSheet(QString(
                                  "QLineEdit#txtCommand { font-family: monospace; color: %1; font-size: 12px;"
                                  " border-right: none; border-radius: 0; border-top-left-radius: 4px;"
                                  " border-bottom-left-radius: 4px; }")
                                  .arg(cyan));

    // Input bar background
    if (m_inputBarOuter)
    m_inputBarOuter->setStyleSheet("");
    cubeWidget->setLightTheme(m_lightTheme);
}

void MainWindow::openSidebar()
{
    if (m_sidebarOpen)
        return;
    m_sidebarOpen = true;

    QWidget *central = this->centralWidget();

    m_sidebarOverlay = new QWidget(central);
    m_sidebarOverlay->setGeometry(central->rect());
    m_sidebarOverlay->setStyleSheet("background: rgba(0,0,0,120);");
    m_sidebarOverlay->show();
    m_sidebarOverlay->raise();

    bool L = m_lightTheme;
    QString sidebarBg = Theme::sidebarBg(L);
    QString sidebarBorder = Theme::sidebarBorder(L);
    QString textPrimary = Theme::textPrimary(L);
    QString textMuted = Theme::textMuted(L);
    QString hoverBg = Theme::hoverBg(L);
    QString btnBg = Theme::buttonBg(L);
    QString btnBorder = Theme::buttonBorder(L);

    m_sidebar = new QWidget(m_sidebarOverlay);
    m_sidebar->setFixedWidth(220);
    m_sidebar->setStyleSheet(QString(
                                 "QWidget { background: %1; border-right: 2px solid %2; }")
                                 .arg(sidebarBg, sidebarBorder));
    m_sidebar->setGeometry(-220, 0, 220, central->height());

    QVBoxLayout *slay = new QVBoxLayout(m_sidebar);
    slay->setContentsMargins(0, 0, 0, 0);
    slay->setSpacing(0);

    // Header
    QWidget *sHeader = new QWidget();
    sHeader->setFixedHeight(52);
    sHeader->setStyleSheet(QString("QWidget { background: %1; border-bottom: 1px solid %2; border-right: none; }").arg(Theme::darkBg(L), sidebarBorder));
    QHBoxLayout *shLay = new QHBoxLayout(sHeader);
    shLay->setContentsMargins(16, 0, 12, 0);
    QLabel *sTitle = new QLabel("Menu");
    sTitle->setStyleSheet(QString("font-size:15px;font-weight:bold;color:%1;background:transparent;border:none;").arg(textPrimary));
    QPushButton *closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(26, 26);
    closeBtn->setStyleSheet(QString(
                                "QPushButton { background:%1; border:1px solid %2; border-radius:13px; color:%3; font-size:12px; padding:0; }"
                                "QPushButton:hover { background:%4; }")
                                .arg(btnBg, btnBorder, textMuted, hoverBg));
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::closeSidebar);
    shLay->addWidget(sTitle);
    shLay->addStretch();
    shLay->addWidget(closeBtn);
    slay->addWidget(sHeader);

    // Menu items
    auto makeItem = [&](const QString &icon, const QString &label) -> QPushButton *
    {
        QPushButton *btn = new QPushButton(QString("  %1  %2").arg(icon, label));
        btn->setFixedHeight(48);
        btn->setStyleSheet(QString(
                               "QPushButton { background: transparent; border: none; border-bottom: 1px solid %1;"
                               " color: %2; font-size: 13px; text-align: left; padding-left: 8px; border-radius: 0; }"
                               "QPushButton:hover { background: %3; }"
                               "QPushButton:pressed { background: %4; }")
                               .arg(sidebarBorder, textPrimary, hoverBg, btnBg));
        return btn;
    };

    QPushButton *itemSettings = makeItem("⚙", "Settings");
    QPushButton *itemHowToUse = makeItem("?", "How to Use");
    QPushButton *itemAbout = makeItem("ℹ", "About");

    connect(itemSettings, &QPushButton::clicked, this, [this]
            { closeSidebar(); showSettingsModal(); });
    connect(itemHowToUse, &QPushButton::clicked, this, [this]
            { closeSidebar(); showHowToUseModal(); });
    connect(itemAbout, &QPushButton::clicked, this, [this]
            { closeSidebar(); showAboutModal(); });

    slay->addWidget(itemSettings);
    slay->addWidget(itemHowToUse);
    slay->addWidget(itemAbout);
    slay->addStretch();

    m_sidebar->show();
    m_sidebar->raise();

    // Animate slide-in
    QPropertyAnimation *anim = new QPropertyAnimation(m_sidebar, "geometry");
    anim->setDuration(320);
    anim->setStartValue(QRect(-220, 0, 220, central->height()));
    anim->setEndValue(QRect(0, 0, 220, central->height()));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // Close on overlay click
    struct SidebarFilter : public QObject
    {
        MainWindow *mw;
        QWidget *overlay;
        QWidget *sidebar;
        SidebarFilter(MainWindow *m, QWidget *o, QWidget *s)
            : QObject(o), mw(m), overlay(o), sidebar(s) {}
        bool eventFilter(QObject *watched, QEvent *e) override
        {
            if (e->type() == QEvent::MouseButtonPress && watched == overlay)
            {
                QMouseEvent *me = static_cast<QMouseEvent *>(e);
                if (!sidebar->geometry().contains(me->pos()))
                    mw->closeSidebar();
                return true;
            }
            if (e->type() == QEvent::Resize && watched == overlay->parentWidget())
            {
                overlay->setGeometry(overlay->parentWidget()->rect());
                sidebar->setFixedHeight(overlay->height());
                return false;
            }
            return false;
        }
    };
    SidebarFilter *sf = new SidebarFilter(this, m_sidebarOverlay, m_sidebar);
    central->installEventFilter(sf);
    m_sidebarOverlay->installEventFilter(sf);
}

void MainWindow::closeSidebar()
{
    if (!m_sidebarOpen || !m_sidebar)
        return;
    QWidget *sb = m_sidebar;
    QWidget *ov = m_sidebarOverlay;
    QPropertyAnimation *anim = new QPropertyAnimation(sb, "geometry");
    anim->setDuration(150);
    anim->setStartValue(sb->geometry());
    anim->setEndValue(QRect(-220, 0, 220, sb->height()));
    anim->setEasingCurve(QEasingCurve::InCubic);
    connect(anim, &QPropertyAnimation::finished, this, [ov, this]
            {
                if (ov) ov->deleteLater();
                m_sidebar = nullptr;
                m_sidebarOverlay = nullptr;
                m_sidebarOpen = false; });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::showSettingsModal()
{
    QWidget *central = this->centralWidget();
    bool L = m_lightTheme;
    QString modalBg = Theme::primaryBg(L);
    QString modalBorder = Theme::borderGroup(L);
    QString textPrimary = Theme::textPrimary(L);
    QString textMuted = Theme::textMuted(L);

    QWidget *overlay = new QWidget(central);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background: rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    QWidget *card = new QWidget(overlay);
    card->setObjectName("settingsCard");
    card->setFixedWidth(380);
    card->setStyleSheet(QString(
                            "QWidget#settingsCard { background:%1; border:1px solid %2; border-radius:10px; }")
                            .arg(modalBg, modalBorder));

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(14);

    QLabel *title = new QLabel("Settings");
    title->setStyleSheet(QString("font-size:16px;font-weight:bold;color:%1;background:transparent;").arg(textPrimary));
    lay->addWidget(title);

    // Theme toggle
    QCheckBox *chkLight = new QCheckBox("Light theme");
    chkLight->setChecked(m_lightTheme);
    chkLight->setStyleSheet(QString("color:%1;background:transparent;font-size:13px;").arg(textPrimary));
    connect(chkLight, &QCheckBox::toggled, this, [this, overlay](bool checked)
            {
                m_lightTheme = checked;
                applyTheme();
                // Rebuild overlay style so it doesn't look stale
                overlay->setStyleSheet("background: rgba(0,0,0,160);"); });
    lay->addWidget(chkLight);

    QCheckBox *chkSmart = new QCheckBox("Use smarter karnotation");
    chkSmart->setChecked(m_smartKarn);
    chkSmart->setToolTip("When 'Karnotation output' is on, use cubeshape-aware karnify.\n"
                         "Applies different karn rules depending on whether the puzzle\n"
                         "is in cubeshape at each move.");
    chkSmart->setStyleSheet(QString("color:%1;background:transparent;font-size:13px;").arg(textPrimary));
    connect(chkSmart, &QCheckBox::toggled, this, [this](bool checked) {
        m_smartKarn = checked;
        if (!m_rawLines.isEmpty()) {
            // Rebuild karn cache with new mode
            m_karnLines.clear();
            m_karnSolutionLines.clear();
            for (int i = 0; i < m_rawLines.size(); i++) {
                bool isSol = m_rawLines[i].contains('[') && m_rawLines[i].contains(']');
                QString karnLine = m_rawLines[i];
                if (isSol) {
                    karnLine = convertLine(m_rawLines[i]);
                    m_karnSolutionLines.append(karnLine);
                }
                m_karnLines.append(karnLine);
            }
            if (chkKarnotation->isChecked()) {
                if (m_tableVisible) rebuildTable();
                else if (chkRankErgo->isChecked()) onRankErgoToggled(true);
                else rebuildTerminalView();
            }
        }
    });
    lay->addWidget(chkSmart);

    QLabel *hint = new QLabel("More settings coming soon.");
    hint->setStyleSheet(QString("color:%1;font-size:11px;background:transparent;").arg(textMuted));
    lay->addWidget(hint);

    card->show();
    card->adjustSize();

    auto center = [overlay, card]()
    {
        overlay->setGeometry(overlay->parentWidget()->rect());
        card->move((overlay->width() - card->width()) / 2, (overlay->height() - card->height()) / 2);
    };
    center();
    card->raise();

    struct F : public QObject
    {
        QWidget *overlay;
        QWidget *card;
        std::function<void()> fn;
        F(QWidget *o, QWidget *c, std::function<void()> f) : QObject(o), overlay(o), card(c), fn(f) {}
        bool eventFilter(QObject *w, QEvent *e) override
        {
            if (e->type() == QEvent::Resize && w == overlay->parentWidget())
            {
                fn();
                return false;
            }
            if (e->type() == QEvent::MouseButtonPress && w == overlay)
            {
                if (!card->geometry().contains(static_cast<QMouseEvent *>(e)->pos()))
                    overlay->deleteLater();
                return true;
            }
            return false;
        }
    };
    F *f = new F(overlay, card, center);
    central->installEventFilter(f);
    overlay->installEventFilter(f);
}

void MainWindow::showHowToUseModal()
{
    QWidget *central = this->centralWidget();
    bool L = m_lightTheme;
    QString modalBg = Theme::primaryBg(L);
    QString modalBorder = Theme::borderGroup(L);
    QString textPrimary = Theme::textPrimary(L);
    QString textBody = Theme::textSecondary(L);
    QString textCyan = Theme::textCyan(L);
    QString scrollBg = Theme::scrollbarBg(L);
    QString scrollHandle = Theme::scrollbarHandle(L);

    QWidget *overlay = new QWidget(central);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background: rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    QWidget *card = new QWidget(overlay);
    card->setObjectName("howToCard");
    int cardW = qMin(560, central->width() - 60);
    int cardH = qMin(520, central->height() - 80);
    card->setFixedSize(cardW, cardH);
    card->setStyleSheet(QString(
                            "QWidget#howToCard { background:%1; border:1px solid %2; border-radius:10px; }")
                            .arg(modalBg, modalBorder));

    QVBoxLayout *lay = new QVBoxLayout(card);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Title bar
    QWidget *titleBar = new QWidget();
    titleBar->setFixedHeight(48);
    titleBar->setStyleSheet(QString(
                                "QWidget { background: %1; border-bottom: 1px solid %2;"
                                " border-top-left-radius: 10px; border-top-right-radius: 10px; border-bottom-left-radius:0; border-bottom-right-radius:0; }")
                                .arg(modalBg, modalBorder));
    QHBoxLayout *tbLay = new QHBoxLayout(titleBar);
    tbLay->setContentsMargins(20, 0, 16, 0);
    QLabel *titleLbl = new QLabel("How to Use");
    titleLbl->setStyleSheet(QString("font-size:15px;font-weight:bold;color:%1;background:transparent;").arg(textPrimary));
    tbLay->addWidget(titleLbl);
    tbLay->addStretch();
    lay->addWidget(titleBar);

    // Scrollable content
    QScrollArea *sa = new QScrollArea();
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setStyleSheet(QString(
                          "QScrollArea { background: transparent; }"
                          "QScrollBar:vertical { background: %1; width: 6px; border-radius: 3px; }"
                          "QScrollBar::handle:vertical { background: %2; border-radius: 3px; min-height: 20px; }"
                          "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; border:0; }")
                          .arg(scrollBg, scrollHandle));

    QLabel *body = new QLabel();
    body->setWordWrap(true);
    body->setOpenExternalLinks(false);
    body->setTextFormat(Qt::RichText);
    body->setContentsMargins(20, 16, 20, 16);
    body->setStyleSheet(QString("background: transparent; color: %1; font-size: 12px; line-height: 1.7;").arg(textBody));
    body->setText(QString(
                      "<b style='color:%1;font-size:13px;'>Cube Controls</b><br>"
                      "<b style='color:%1;'>Keyboard shortcuts:</b><br>"
                      "• <b style='color:%2;'>Z</b> = Undo &nbsp; <b style='color:%2;'>Y</b> = Redo &nbsp; <b style='color:%2;'>Esc</b> = Reset the cube to solved<br>"
                      "The below shortcuts are identical to those of the csTimer virtual squan.<br>"
                      "• <b style='color:%2;'>J</b> = U, but only by one piece &nbsp; <b style='color:%2;'>F</b> = U', but only by one piece <br>"
                      "• <b style='color:%2;'>S</b> = D, but only by one piece &nbsp; <b style='color:%2;'>L</b> = D', but only by one piece <br>"
                      "• <b style='color:%2;'>I</b> or <b style='color:%2;'>K</b> = Slice<br>"
                      "• <b style='color:%2;'>H</b> = 3,0 (U) &nbsp; <b style='color:%2;'>G</b> = -3,0 (U')<br>"
                      "• <b style='color:%2;'>W</b> = 0,3 (D) &nbsp; <b style='color:%2;'>O</b> = 0,-3 (D')<br><br>"
                      "<b style='color:%1;font-size:13px;'>Scramble / Alg Input</b><br>"
                      "Type some moves and hit <b>Apply</b>. Karn will be parsed correctly.<br>"
                      "Use the mode button (to the left of the alg input) to switch between <b>Scram</b> (applies moves forward) and <b>Alg</b> (inverts before applying).<br><br>"
                      "<b style='color:%1;font-size:13px;'>Options</b><br>"
                      "you can read the descriptions for the options by hovering over them, but here's a comprehensive list:<br>"
                      "• <b>Slice metric</b>: only count slices as moves (instead of also including U and D moves when counting the \"movecount\" of an alg).<br>"
                      "• <b>All optimal</b>: instead of stopping the solver after finding one of the shortest solutions, find all of them. (\"shortest\" means: the least \"moves\". change what a \"move\" mean with the slice metric option)<br>"
                      "• <b>+suboptimal</b>: on top of finding all the shortest solutions, also find solutions up to N moves longer than optimal.<br>"
                      "• <b>Specific depths</b>: search only for solutions that are these moves long (comma-separated). e.g. \"8,9\" will return all solutions that are 8 moves and 9 moves long.<br>"
                      "• <b>Generator alg</b>: instead of solving the case displayed in the app, output algs will set up the case.<br>"
                      "• <b>Stay in cubeshape</b>: restrict to algs that stay in cubeshape (CS) throughout.<br>"
                      "• <b>Karnotation output</b>: display solutions in karn instead of WCA notation.<br>"
                      "• <b>Max top / bottom / total turns</b>: limit how big the layer turns can be. Hover over the options to see details.<br><br>"
                      "<b style='color:%1;font-size:13px;'>Output</b><br>"
                      "Solutions will appear in the terminal. After you generated some algs, these buttons will appear:<br>"
                      "• the <b>⊞</b> button: switch between terminal view and table view.<br>"
                      "• the <b>⤢</b> button: expand the terminal to full screen.<br>"
                      "Right-click a row in the table view to copy the alg, or to copy the whole row.<br>"
                      "If <b>Stay in cubeshape</b> was active, you can click on <b>Roughly rank algs based on relative ergonomics</b> (located below the terminal area), which will sort the output algs by an estimate of their actual speed.<br>")
                      .arg(textPrimary, textCyan));

    sa->setWidget(body);
    lay->addWidget(sa, 1);

    card->show();

    auto center = [overlay, card, central]()
    {
        overlay->setGeometry(overlay->parentWidget()->rect());
        int cw = qMin(560, central->width() - 60);
        int ch = qMin(520, central->height() - 80);
        card->setFixedSize(cw, ch);
        card->move((overlay->width() - card->width()) / 2, (overlay->height() - card->height()) / 2);
    };
    center();
    card->raise();

    struct F : public QObject
    {
        QWidget *overlay;
        QWidget *card;
        std::function<void()> fn;
        F(QWidget *o, QWidget *c, std::function<void()> f) : QObject(o), overlay(o), card(c), fn(f) {}
        bool eventFilter(QObject *w, QEvent *e) override
        {
            if (e->type() == QEvent::Resize && w == overlay->parentWidget())
            {
                fn();
                return false;
            }
            if (e->type() == QEvent::MouseButtonPress && w == overlay)
            {
                if (!card->geometry().contains(static_cast<QMouseEvent *>(e)->pos()))
                    overlay->deleteLater();
                return true;
            }
            return false;
        }
    };
    F *f = new F(overlay, card, center);
    central->installEventFilter(f);
    overlay->installEventFilter(f);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // ── (0) Per-line tooltips for the output box ──────────────────────────────
    // QTextEdit delivers QHelpEvent to its internal viewport, not to itself.
    if (event->type() == QEvent::ToolTip && txtOutput && watched == txtOutput->viewport())
        return true;

    // ── Suppress all native tooltips; we handle them via MouseMove ───────────
    if (event->type() == QEvent::ToolTip)
        return true;

    // ── Pointing hand only over checkbox indicator+text, arrow elsewhere ──────
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::HoverMove) {
        QWidget *under = QApplication::widgetAt(QCursor::pos());
        if (QCheckBox *cb = qobject_cast<QCheckBox*>(under)) {
            QPoint localPos = cb->mapFromGlobal(QCursor::pos());
            QStyleOptionButton opt;
            opt.initFrom(cb);
            QRect indRect = cb->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, cb);
            QFontMetrics fm(cb->font());
            QRect textRect(indRect.right() + 6, 0, fm.horizontalAdvance(cb->text()), cb->height());
            QRect activeRect = indRect.united(textRect);
            if (activeRect.contains(localPos)) {
                if (cb != chkDepths)
                    cb->setCursor(Qt::PointingHandCursor);
                if (!cb->toolTip().isEmpty())
                    FadingTooltip::arm(cb->toolTip(), QCursor::pos(), this);
            } else {
                cb->setCursor(Qt::ArrowCursor);
                FadingTooltip::dismiss(this);
            }
        } else {
            FadingTooltip::dismiss(this);
        }
    }

    // ── IBeam cursor only over actual label text ──────────────────────────────
    if (event->type() == QEvent::MouseMove) {
        if (QLabel *lbl = qobject_cast<QLabel*>(watched)) {
            if (lbl->parent() && qobject_cast<QTableWidget*>(lbl->parent()->parent())) {
                QMouseEvent *me = static_cast<QMouseEvent*>(event);
                // Check if the mouse is within the bounding rect of the actual text
                QFontMetrics fm(lbl->font());
                QRect textRect = fm.boundingRect(
                    lbl->contentsRect(), Qt::AlignVCenter | Qt::AlignLeft | Qt::TextWordWrap,
                    lbl->text());
                textRect.adjust(4, 0, 0, 0); // match contentsMargins
                lbl->setCursor(textRect.contains(me->pos()) ? Qt::IBeamCursor : Qt::ArrowCursor);
                return false;
            }
        }
    }

    if (event->type() == QEvent::Resize && watched == m_outputWrapper)
    {
        int w = m_outputWrapper->width();
        int margin = 6;
        int bw = 22;
        btnExpand->move(w - margin - bw, margin);
        btnTableMode->move(w - margin - bw * 2 - 4, margin);
        btnCopyTerminal->move(w - margin - bw * 3 - 8, margin);
        return false;
    }

    if (event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(watched, event);

    QKeyEvent *ke = static_cast<QKeyEvent *>(event);

    // ── (1) Ctrl+C stops the solver ──────────────────────────────────────────
    if (ke->key() == Qt::Key_C && (ke->modifiers() & Qt::ControlModifier))
    {
        if (worker && worker->isRunning())
        {
            stopSolver();
            return true; // consume — don't copy
        }
        return QMainWindow::eventFilter(watched, event);
    }

    // ── Events already targeting cubeWidget: let cubeWidget handle them. ─────
    // (sendEvent below will re-enter here with watched == cubeWidget.)
    if (watched == cubeWidget)
        return QMainWindow::eventFilter(watched, event);

    // ── Enter / Shift+Enter in m_mainInput ───────────────────────────────────
    // Enter        = apply from solved state
    // Shift+Enter  = apply on current state
    if ((watched == m_mainInput) && ke->key() == Qt::Key_Return)
    {
        if (ke->modifiers() & Qt::ShiftModifier)
        {
            // Shift+Enter: apply on current state (do nothing if empty)
            if (m_mainInput->text().trimmed().isEmpty())
                return true;
            btnApply->click();
            return true;
        }
        // plain Enter: apply from solved state
        m_applyFromSolved = true;
        btnApply->click();
        m_applyFromSolved = false;
        return true;
    }

    // ── Text inputs get all keys — never steal from them ─────────────────────
    {
        QWidget *fw = QApplication::focusWidget();
        if (fw == txtCommand || fw == txtScramble || fw == txtDepths || fw == m_mainInput ||
            watched == txtCommand || watched == txtScramble || watched == txtDepths || watched == m_mainInput)
            return QMainWindow::eventFilter(watched, event);
    }

    // ── (2) Route cube shortcuts from any other widget ────────────────────────
    if (ke->modifiers() == Qt::NoModifier)
    {
        auto sendCube = [this](Qt::Key k)
        {
            QKeyEvent e(QEvent::KeyPress, k, Qt::NoModifier);
            QApplication::sendEvent(cubeWidget, &e);
        };
        bool handled = true;
        switch (ke->key())
        {
        case Qt::Key_I:
        case Qt::Key_K:
        {
            m_sliceCount++;
            m_slicePending.append({cubeWidget->getPositionString()});
            sendCube(static_cast<Qt::Key>(ke->key()));
            m_sliceTimer->start(600);
            break;
        }
        case Qt::Key_J:
            pushUndoState();
            sendCube(Qt::Key_J);
            break;
        case Qt::Key_F:
            pushUndoState();
            sendCube(Qt::Key_F);
            break;
        case Qt::Key_S:
            pushUndoState();
            sendCube(Qt::Key_S);
            break;
        case Qt::Key_L:
            pushUndoState();
            sendCube(Qt::Key_L);
            break;
        case Qt::Key_Escape:
            m_undoStack.clear();
            m_redoStack.clear();
            btnUndo->setEnabled(false);
            btnRedo->setEnabled(false);
            sendCube(Qt::Key_Escape);
            break;
        case Qt::Key_Z:
            if (!m_undoStack.isEmpty())
                btnUndo->click();
            break;
        case Qt::Key_Y:
            if (!m_redoStack.isEmpty())
                btnRedo->click();
            break;
        case Qt::Key_H:
            pushUndoState();
            sendCube(Qt::Key_J);
            sendCube(Qt::Key_J);
            break; // UU
        case Qt::Key_G:
            pushUndoState();
            sendCube(Qt::Key_F);
            sendCube(Qt::Key_F);
            break; // U'U'
        case Qt::Key_O:
            pushUndoState();
            sendCube(Qt::Key_L);
            sendCube(Qt::Key_L);
            break; // D'D'
        case Qt::Key_W:
            pushUndoState();
            sendCube(Qt::Key_S);
            sendCube(Qt::Key_S);
            break; // DD
        default:
            handled = false;
            break;
        }
        if (handled)
            return true; // consume — letter goes to cube, not to any text field
    }
    return QMainWindow::eventFilter(watched, event);
}
