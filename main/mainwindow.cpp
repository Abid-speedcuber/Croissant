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
#include <QButtonGroup>
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
#include <QFontDatabase>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <QPainter>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsProxyWidget>

// ============================================================
// TightCheckBox — only shows tooltip when hovering over indicator+text
// ============================================================
class TightCheckBox : public QCheckBox
{
public:
    using QCheckBox::QCheckBox;
    bool event(QEvent *e) override
    {
        if (e->type() == QEvent::ToolTip)
        {
            QStyleOptionButton opt;
            opt.initFrom(this);
            QRect textRect = style()->subElementRect(QStyle::SE_CheckBoxContents, &opt, this);
            QRect indRect = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, this);
            QRect activeRect = textRect.united(indRect);
            QHelpEvent *he = static_cast<QHelpEvent *>(e);
            if (!activeRect.contains(he->pos()))
                return true; // swallow
        }
        return QCheckBox::event(e);
    }
};

// ============================================================
// SelectableDelegate — makes table cells selectable by mouse drag
// ============================================================
class SelectableDelegate : public QStyledItemDelegate
{
public:
    explicit SelectableDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        QLineEdit *ed = new QLineEdit(parent);
        ed->setReadOnly(true);
        ed->setFrame(false);
        ed->setStyleSheet("QLineEdit { background: transparent; border: none; padding: 0 4px; selection-background-color: #3a6ea8; selection-color: #ffffff; }");
        ed->setCursor(Qt::IBeamCursor);
        return ed;
    }
    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        static_cast<QLineEdit *>(editor)->setText(index.data().toString());
    }
    void setModelData(QWidget *, QAbstractItemModel *, const QModelIndex &) const override {}
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        editor->setGeometry(option.rect);
    }
};

// ============================================================
// FadingTooltip — singleton tooltip with fade-in, shown via hover timer
// ============================================================
class FadingTooltip : public QWidget
{
public:
    static void arm(const QString &text, const QPoint &globalPos, QWidget *parent)
    {
        inst(parent).armImpl(text, globalPos);
    }
    static void dismiss(QWidget *parent)
    {
        inst(parent).dismissImpl();
    }
    static void setLightTheme(bool light, QWidget *parent)
    {
        inst(parent).m_lightTheme = light;
    }

private:
    explicit FadingTooltip(QWidget *parent) : QWidget(parent, Qt::SubWindow)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);

        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        m_label->setMaximumWidth(320);
        m_label->setContentsMargins(8, 5, 8, 5);

        QVBoxLayout *l = new QVBoxLayout(this);
        l->setContentsMargins(0, 0, 0, 0);
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

    static FadingTooltip &inst(QWidget *parent)
    {
        static QPointer<FadingTooltip> s_inst;
        if (!s_inst)
            s_inst = new FadingTooltip(parent->window());
        return *s_inst;
    }

    void stopAnim()
    {
        if (m_anim)
        {
            m_anim->stop();
            m_anim = nullptr;
        }
    }

    void armImpl(const QString &text, const QPoint &globalPos)
    {
        m_pendingText = text;
        m_pendingPos = globalPos;
        // Same text already visible — nothing to do
        if (isVisible() && m_currentText == text)
            return;
        // Different text already visible (mouse moved between options):
        // skip delay, update immediately
        if (isVisible())
        {
            stopAnim();
            m_hoverTimer->stop();
            showNow();
            return;
        }
        // Not yet visible: always restart the timer so the position updates
        // as the user moves, and we always show from the correct final spot
        m_hoverTimer->start(200);
    }

    void dismissImpl()
    {
        m_hoverTimer->stop();
        m_closeTimer->stop();
        if (!isVisible())
            return;
        stopAnim();
        m_currentText.clear();
        QPropertyAnimation *anim = new QPropertyAnimation(m_effect, "opacity", this);
        m_anim = anim;
        anim->setDuration(150);
        anim->setStartValue(m_effect->opacity());
        anim->setEndValue(0.0);
        anim->setEasingCurve(QEasingCurve::InCubic);
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]
                {
            if (m_anim == anim) { hide(); m_anim = nullptr; } });
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void applyThemeStyle()
    {
        // Light: white card, subtle border, near-black text
        // Dark:  existing theme colours
        const bool L = m_lightTheme;
        m_cachedBg = L ? "#ffffff" : Theme::fadingTooltipBg();
        m_cachedBorder = L ? "#c4c8dc" : Theme::fadingTooltipBorder();
        QString textCol = L ? "#2a2a3a" : Theme::fadingTooltipText();
        m_label->setStyleSheet(QString(
                                   "QLabel { background: transparent; color: %1; font-size: 11px; }")
                                   .arg(textCol));
        setStyleSheet(QString(
                          "FadingTooltip { background: %1; border: 1px solid %2; border-radius: 5px; }")
                          .arg(m_cachedBg, m_cachedBorder));
        update();
    }

    void showNow()
    {
        m_currentText = m_pendingText;
        applyThemeStyle();
        m_label->setText(m_currentText);
        m_label->adjustSize();
        adjustSize();

        QWidget *win = parentWidget();
        QPoint local = win->mapFromGlobal(m_pendingPos) + QPoint(14, 18);
        local.setX(qMin(local.x(), win->width() - width() - 8));
        local.setY(qMin(local.y(), win->height() - height() - 8));
        move(local);
        raise();
        show();

        // Fade in from current opacity so switching tooltips never resets to 0
        stopAnim();
        double startOpacity = m_effect->opacity();
        QPropertyAnimation *anim = new QPropertyAnimation(m_effect, "opacity", this);
        m_anim = anim;
        anim->setDuration(static_cast<int>(160 * (1.0 - startOpacity)));
        anim->setStartValue(startOpacity);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, this, [this, anim]
                {
            if (m_anim == anim) m_anim = nullptr; });
        anim->start(QAbstractAnimation::DeleteWhenStopped);

        m_closeTimer->start(8000);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor(m_cachedBg.isEmpty() ? Theme::fadingTooltipBg() : m_cachedBg));
        p.setPen(QColor(m_cachedBorder.isEmpty() ? Theme::fadingTooltipBorder() : m_cachedBorder));
        p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 5, 5);
    }

    QLabel *m_label{nullptr};
    QGraphicsOpacityEffect *m_effect{nullptr};
    QTimer *m_hoverTimer{nullptr};
    QTimer *m_closeTimer{nullptr};
    QPropertyAnimation *m_anim{nullptr};
    QString m_pendingText;
    QPoint m_pendingPos;
    QString m_currentText;
    QString m_cachedBg;
    QString m_cachedBorder;
    bool m_lightTheme{false};
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
    txtOutput->viewport()->installEventFilter(this);

    // ── Load Abid's notation font (embedded resource) ─────────────────────────
    {
        int id = QFontDatabase::addApplicationFont(":/kompact-font.ttf");
        if (id != -1) {
            QStringList families = QFontDatabase::applicationFontFamilies(id);
            if (!families.isEmpty())
                m_abidFontFamily = families.first();
        }
    }
    buildStyles();
    if (m_mainWidget) m_mainWidget->setStyleSheet(buildStyleSheet());
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
    // ── Zoom scaffold: QGraphicsView wraps everything so we can scale the whole UI ──
    m_mainWidget = new QWidget();   // this is the real UI root
    QWidget *realInner = m_mainWidget;
    m_zoomScene = new QGraphicsScene(this);
    m_zoomView  = new QGraphicsView(m_zoomScene, this);
    m_zoomView->setFrameShape(QFrame::NoFrame);
    m_zoomView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_zoomView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_zoomView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_zoomView->setRenderHint(QPainter::Antialiasing);
    m_zoomView->setStyleSheet("background: transparent; border: none;");
    m_zoomProxy = m_zoomScene->addWidget(realInner);
    setCentralWidget(m_zoomView);

    QWidget *outerWidget = realInner;  // rest of buildUI builds into realInner
    QVBoxLayout *outerLayout = new QVBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

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
    connect(cubeWidget, &Sq1Widget::positionChanged, this, [this]() {
        bool cs = cubeWidget->inCubeshape();
        if (!cs && chkCubeshape->isChecked()) {
            chkCubeshape->blockSignals(true);
            chkCubeshape->setChecked(false);
            chkCubeshape->blockSignals(false);
            updateConstraints();
            updateCommand();
        }
        chkCubeshape->setEnabled(cs);
    });
    connect(cubeWidget, &Sq1Widget::middleStateChanged, this, [this](int state) {
        bool shouldBeChecked = (state == 2);
        if (chkIgnoreMid->isChecked() != shouldBeChecked) {
            if (!shouldBeChecked) {
                // leaving gray state — remember nothing, just uncheck
                m_preIgnoreMidState = 0;
            }
            chkIgnoreMid->blockSignals(true);
            chkIgnoreMid->setChecked(shouldBeChecked);
            chkIgnoreMid->blockSignals(false);
            updateConstraints();
            updateCommand();
        }
    });
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
        struct CubeResizeFilter : public QObject
        {
            QWidget *cubeWithReset;
            QWidget *cubeWrapper;
            QPushButton *btnReset;
            CubeResizeFilter(QWidget *p, QWidget *cwr, QWidget *cwrap, QPushButton *btn)
                : QObject(p), cubeWithReset(cwr), cubeWrapper(cwrap), btnReset(btn) {}
            bool eventFilter(QObject *watched, QEvent *e) override
            {
                if (e->type() == QEvent::Resize && watched == cubeWithReset)
                {
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
        centerRow->addWidget(cubeWithReset); // stretches to fill
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
    btnUndo = new QPushButton("Undo (Ctrl+Z)");
    btnUndo->setObjectName("btnUndo");
    btnUndo->setEnabled(false);
    btnUndo->setToolTip("Undo  [Ctrl+Z]");
    btnRedo = new QPushButton("Redo (Ctrl+Y)");
    btnRedo->setObjectName("btnRedo");
    btnRedo->setEnabled(false);
    btnRedo->setToolTip("Redo  [Ctrl+Y]");
    undoResetRedoRow->addWidget(btnUndo, 1);
    undoResetRedoRow->addWidget(btnRedo, 1);
    leftCol->addLayout(undoResetRedoRow);

    btnSolve = new QPushButton("▶  Solve  [Ctrl+↵]");
    btnSolve->setObjectName("btnSolve");
    btnSolve->setFixedHeight(48);
    leftCol->addWidget(btnSolve);

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
    optionsInner->setObjectName("optionsInner");
    QVBoxLayout *grid = new QVBoxLayout(optionsInner);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(0);

    auto makeRow = [](const QString &objName = "optionRow") -> QWidget *
    {
        QWidget *row = new QWidget();
        row->setObjectName(objName);
        row->setAttribute(Qt::WA_StyledBackground, true);
        QHBoxLayout *l = new QHBoxLayout(row);
        l->setContentsMargins(0, 0, 0, 0);
        l->setSpacing(0);
        return row;
    };
    auto rowLeft = [](QWidget *row) -> QHBoxLayout *
    {
        QWidget *left = new QWidget();
        left->setObjectName("optionRowLeft");
        left->setAttribute(Qt::WA_StyledBackground, true);
        QHBoxLayout *ll = new QHBoxLayout(left);
        ll->setContentsMargins(0, 0, 0, 0);
        ll->setSpacing(0);
        static_cast<QHBoxLayout *>(row->layout())->addWidget(left, 1);
        return ll;
    };
    auto rowRight = [](QWidget *row) -> QHBoxLayout *
    {
        QWidget *right = new QWidget();
        right->setObjectName("optionRowRight");
        right->setAttribute(Qt::WA_StyledBackground, true);
        QHBoxLayout *rl = new QHBoxLayout(right);
        rl->setContentsMargins(0, 0, 0, 0);
        rl->setSpacing(0);
        static_cast<QHBoxLayout *>(row->layout())->addWidget(right);
        return rl;
    };

    // ── Widgets ──────────────────────────────────────────────────────────────
    // ── Metric radio: Slice (default) | Move | Angle ─────────────────────────
    auto makeRadioRow = [this](const QString &labelText, const QStringList &opts,
                               int defaultId, QButtonGroup *&groupOut,
                               const QString &rowName, const QString &pillName) -> QWidget *
    {
        QWidget *row = new QWidget();
        row->setObjectName(rowName);
        row->setAttribute(Qt::WA_StyledBackground, true);
        QHBoxLayout *rLay = new QHBoxLayout(row);
        rLay->setContentsMargins(0, 0, 0, 0);
        rLay->setSpacing(0);

        QLabel *lbl = new QLabel(labelText);
        lbl->setObjectName(rowName + "_label");
        rLay->addWidget(lbl);
        rLay->addStretch();

        QWidget *pill = new QWidget();
        pill->setObjectName(pillName);
        pill->setAttribute(Qt::WA_StyledBackground, true);
        QHBoxLayout *pLay = new QHBoxLayout(pill);
        pLay->setContentsMargins(2, 2, 2, 2);
        pLay->setSpacing(0);

        groupOut = new QButtonGroup(this);
        groupOut->setExclusive(true);
        for (int i = 0; i < opts.size(); i++)
        {
            QPushButton *btn = new QPushButton(opts[i]);
            btn->setCheckable(true);
            btn->setChecked(i == defaultId);
            btn->setCursor(Qt::PointingHandCursor);
            if (i == 0)
                btn->setObjectName(pillName + "_first");
            else if (i == opts.size() - 1)
                btn->setObjectName(pillName + "_last");
            else
                btn->setObjectName(pillName + "_mid");
            groupOut->addButton(btn, i);
            pLay->addWidget(btn);
        }
        rLay->addWidget(pill);
        return row;
    };

    QWidget *metricRadioRow = makeRadioRow("Metric", {"Slice", "Move", "Angle"}, 0, m_metricGroup, "metricRadioRow", "metricPill");
    metricRadioRow->setToolTip("Choose how move length is counted:\n"
                               "Slice – only slices count\n"
                               "Move  – layer turns count too\n"
                               "Angle – turns weighted by angle amount");
    // (pill objectName set inside makeRadioRow)

    chkAllOptimal = new TightCheckBox("All optimal");
    chkAllOptimal->setObjectName("chkAllOptimal");
    chkAllOptimal->setToolTip("Find all the optimal solutions, not just the first one.");

    spnSuboptimal = new QSpinBox();
    spnSuboptimal->setObjectName("spnSuboptimal");
    spnSuboptimal->setRange(0, 9);
    spnSuboptimal->setValue(0);
    spnSuboptimal->setToolTip("Extra moves beyond optimal to *also* find (0 = optimal only).");

    QLabel *lblSuboptLabel = new QLabel("+suboptimal:");
    lblSuboptLabel->setObjectName("lblSuboptLabel");

    chkDepths = new TightCheckBox("Specific depths:");
    chkDepths->setObjectName("chkDepths");
    chkDepths->setToolTip("Search only the listed move depths instead of starting from 0 and going up.\n"
                          "Comma-separated, e.g.\"8,9\". \n"
                          "Write in the input box to toggle it on.");
    chkDepths->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    chkDepths->setFocusPolicy(Qt::NoFocus);

    txtDepths = new QLineEdit();
    txtDepths->setObjectName("txtDepths");
    txtDepths->setPlaceholderText("e.g. 8,9");
    txtDepths->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[0-9,]*"), txtDepths));
    txtDepths->setToolTip("Comma-separated list of depths to search, e.g. \"8,9\"");

    chkGenerator = new TightCheckBox("Generator alg");
    chkGenerator->setObjectName("chkGenerator");
    chkGenerator->setToolTip("If selected, generated algs will set up to the case from a solved cube,\n"
                             "else the algs will solve the case.");

    chk2gen = new TightCheckBox("2Gen  (top layer + slices only)");
    chk2gen->setObjectName("chk2gen");
    chk2gen->setToolTip("Restrict to 2-gen moves: top-layer turns and slices only.\n"
                        "Requires the bottom left pieces to already be solved.\n"
                        "You cannot demand both 2-gen and stay-in-cubeshape.");

    chkPseudo2gen = new TightCheckBox("Pseudo 2Gen  (bottom: ±1 only)");
    chkPseudo2gen->setObjectName("chkPseudo2gen");
    chkPseudo2gen->setToolTip("Restrict bottom-layer turns to ±1 only (2-gen with bottom 1 moves).\n");

    chkCubeshape = new TightCheckBox("Stay in cubeshape");
    chkCubeshape->setObjectName("chkCubeshape");
    chkCubeshape->setToolTip("Only generate algs that keep the puzzle in cubeshape throughout.");

    chkIgnoreMid = new TightCheckBox("Ignore middle layer");
    chkIgnoreMid->setObjectName("chkIgnoreMid");
    chkIgnoreMid->setToolTip("Ignore bar states. Equivalent to clicking on the bar until it is gray.");

    chkKarnotation = new TightCheckBox("Karnotation output");
    chkKarnotation->setObjectName("chkKarnotation");
    chkKarnotation->setToolTip("Display solutions in karnotation instead of WCA notation.");

    QWidget *angleRadioRow = makeRadioRow("Lock layer angle on preabf",
                                          {"Both", "Top", "Bottom", "None"}, 3, m_angleGroup, "angleRadioRow", "anglePill");
    angleRadioRow->setToolTip("Lock the pre-ABF angle move to ±1 (or 0).\n"
                              "Both   – restricts top and bottom\n"
                              "Top    – restricts top layer only\n"
                              "Bottom – restricts bottom layer only\n"
                              "None   – no restriction (default)");

    QWidget *normalizeAbfRow = makeRadioRow("Normalize ABF",
                                            {"Both", "PreABF", "PostABF", "None"}, 3, m_normalizeAbfGroup, "normalizeAbfRow", "normalizeAbfPill");
    // TODO: update this tooltip text with a precise description of what normalizing does
    normalizeAbfRow->setToolTip("Control which AUF moves are normalized in the output.\n"
                                "PreABF  – normalize the move before the first slice\n"
                                "PostABF – normalize the move after the last slice\n"
                                "Both    – normalize both ends\n"
                                "None    – no normalization (default)");

    chkMaxX = new TightCheckBox("Max top turn:");
    chkMaxX->setObjectName("chkMaxX");
    chkMaxX->setToolTip("Limit the maximum top-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"4\", that means algs can do -4 to 4 on top.");
    spnMaxX = new QSpinBox();
    spnMaxX->setObjectName("spnMaxX");
    spnMaxX->setRange(0, 6);
    spnMaxX->setValue(3);
    spnMaxX->setToolTip("Maximum top-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"4\", that means algs can do -4 to 4 on top.");

    chkMaxY = new TightCheckBox("Max bottom turn:");
    chkMaxY->setObjectName("chkMaxY");
    chkMaxY->setToolTip("Limit the maximum bottom-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"3\", that means algs can do -3 to 3 on bottom.");
    spnMaxY = new QSpinBox();
    spnMaxY->setObjectName("spnMaxY");
    spnMaxY->setRange(0, 6);
    spnMaxY->setValue(3);
    spnMaxY->setToolTip("Maximum bottom-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"3\", that means algs can do -3 to 3 on bottom.");

    chkMaxTotal = new TightCheckBox("Max total turn:");
    chkMaxTotal->setObjectName("chkMaxTotal");
    chkMaxTotal->setToolTip("Limit the maximum combined |top|+|bottom| turn per move pair (1–12).\n"
                            "e.g. if you put \"6\", (3,-3) is allowed in algs (3+3<=6),\n"
                            "but (-5,-2) is not (5+2>6).");
    spnMaxTotal = new QSpinBox();
    spnMaxTotal->setObjectName("spnMaxTotal");
    spnMaxTotal->setRange(1, 12);
    spnMaxTotal->setValue(6);
    spnMaxTotal->setToolTip("Maximum combined |top|+|bottom| turn per move pair (1–12).\n"
                            "e.g. if you put \"6\", (3,-3) is allowed in algs (3+3<=6),\n"
                            "but (-5,-2) is not (5+2>6).");

    chkKarnotation->setChecked(true);

    // ── Div-style rows ───────────────────────────────────────────────────────
    // Row: Metric radio (full width)
    {
        QWidget *row = makeRow("optionRow_metric");
        rowLeft(row)->addWidget(metricRadioRow, 1);
        grid->addWidget(row);
    }
    // Row: All optimal + suboptimal
    {
        QWidget *row = makeRow("optionRow_allopt");
        rowLeft(row)->addWidget(chkAllOptimal);
        QHBoxLayout *rr = rowRight(row);
        rr->addWidget(lblSuboptLabel);
        rr->addSpacing(4);
        rr->addWidget(spnSuboptimal);
        grid->addWidget(row);
    }
    // Row: Specific depths
    {
        QWidget *row = makeRow("optionRow_depths");
        rowLeft(row)->addWidget(chkDepths);
        rowRight(row)->addWidget(txtDepths);
        grid->addWidget(row);
    }
    // Row: Generator alg (full width)
    {
        QWidget *row = makeRow("optionRow_generator");
        rowLeft(row)->addWidget(chkGenerator);
        grid->addWidget(row);
    }
    // Row: 2Gen (full width)
    {
        QWidget *row = makeRow("optionRow_2gen");
        rowLeft(row)->addWidget(chk2gen);
        grid->addWidget(row);
    }
    // Row: Pseudo 2Gen (full width)
    {
        QWidget *row = makeRow("optionRow_pseudo2gen");
        rowLeft(row)->addWidget(chkPseudo2gen);
        grid->addWidget(row);
    }
    // Row: Stay in cubeshape (full width)
    {
        QWidget *row = makeRow("optionRow_cubeshape");
        rowLeft(row)->addWidget(chkCubeshape);
        grid->addWidget(row);
    }
    // Row: Ignore middle layer (full width)
    {
        QWidget *row = makeRow("optionRow_ignoremid");
        rowLeft(row)->addWidget(chkIgnoreMid);
        grid->addWidget(row);
    }
    // Row: Angle radio (full width)
    {
        QWidget *row = makeRow("optionRow_angle");
        rowLeft(row)->addWidget(angleRadioRow, 1);
        grid->addWidget(row);
    }
    // Row: Normalize ABF radio (full width)
    {
        QWidget *row = makeRow("optionRow_normalizeabf");
        rowLeft(row)->addWidget(normalizeAbfRow, 1);
        grid->addWidget(row);
    }
    // Row: Max top turn
    {
        QWidget *row = makeRow("optionRow_maxx");
        rowLeft(row)->addWidget(chkMaxX);
        rowRight(row)->addWidget(spnMaxX);
        grid->addWidget(row);
    }
    // Row: Max bottom turn
    {
        QWidget *row = makeRow("optionRow_maxy");
        rowLeft(row)->addWidget(chkMaxY);
        rowRight(row)->addWidget(spnMaxY);
        grid->addWidget(row);
    }
    // Row: Max total turn
    {
        QWidget *row = makeRow("optionRow_maxtotal");
        rowLeft(row)->addWidget(chkMaxTotal);
        rowRight(row)->addWidget(spnMaxTotal);
        grid->addWidget(row);
    }
    grid->addStretch();

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

    connect(m_metricGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [upd](int)
            { upd(); });
    connect(chkAllOptimal, &QCheckBox::toggled, this, upd);
    connect(spnSuboptimal, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkDepths, &QCheckBox::clicked, this, [this](bool)
            {
        // Clicking directly is not allowed — state is driven by txtDepths content
        QString t = txtDepths->text().trimmed();
        bool valid = !t.isEmpty() && QRegularExpression("^[0-9]+(,[0-9]+)*$").match(t).hasMatch();
        chkDepths->blockSignals(true);
        chkDepths->setChecked(valid);
        chkDepths->blockSignals(false); });
    connect(txtDepths, &QLineEdit::textChanged, this, [this, upd](const QString &text)
            {
        QString t = text.trimmed();
        // Valid = non-empty and only digits and commas, at least one digit
        bool valid = !t.isEmpty() && QRegularExpression("^[0-9]+(,[0-9]+)*$").match(t).hasMatch();
        chkDepths->blockSignals(true);
        chkDepths->setChecked(valid);
        chkDepths->blockSignals(false);
        upd(); });
    connect(chkGenerator, &QCheckBox::toggled, this, upd);
    connect(chk2gen, &QCheckBox::toggled, this, upd);
    connect(chkPseudo2gen, &QCheckBox::toggled, this, upd);
    connect(chkCubeshape, &QCheckBox::toggled, this, upd);
    connect(chkIgnoreMid, &QCheckBox::toggled, this, [this, upd](bool checked) {
        if (checked) {
            m_preIgnoreMidState = cubeWidget->getMiddleState();
            cubeWidget->setMiddleState(2); // gray / partial
        } else {
            cubeWidget->setMiddleState(m_preIgnoreMidState);
        }
        upd();
    });
    connect(chkKarnotation, &QCheckBox::toggled, this, [this, upd](bool /*checked*/)
            {
        upd();
        // Rebuild views from cache — no re-solve needed
        if (!m_rawLines.isEmpty()) {
            if (m_tableVisible)
                rebuildTable();
            else if (chkRankErgo->isChecked())
                onRankErgoToggled(true);
            else
                rebuildTerminalView();
        } });
    connect(m_angleGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [upd](int)
            { upd(); });
    connect(m_normalizeAbfGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [upd](int)
            { upd(); });
    connect(chkMaxX, &QCheckBox::toggled, this, upd);
    connect(spnMaxX, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxY, &QCheckBox::toggled, this, upd);
    connect(spnMaxY, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxTotal, &QCheckBox::toggled, this, upd);
    connect(spnMaxTotal, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);

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

    // Ctrl+Enter triggers Solve / Stop from anywhere in the window
    QShortcut *solveShortcut = new QShortcut(QKeySequence("Ctrl+Return"), this);
    connect(solveShortcut, &QShortcut::activated, this, &MainWindow::onSolveButtonClicked);

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
                raw = raw.trimmed();
                if (m_inputModeIndex == 1) {
                    raw = invertScrambleStr(raw);
                }

                // Capture leading/trailing slash from raw BEFORE the addComma pass
                // replaces all '/' with spaces. We restore them after unkarnifyHelp
                // so the move parser sees e.g. "/3,0/0,3/" correctly.
                bool leadingSlash  = !raw.isEmpty() && (raw[0] == '/' || raw[0] == '\\');
                bool trailingSlash = raw.size() > 1 && (raw[raw.size()-1] == '/' || raw[raw.size()-1] == '\\');

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

                // Restore leading/trailing slashes that were stripped by the addComma pass.
                // Karn tokens already produce their surrounding slashes via unkarnifyHelp,
                // so only add if not already present.
                if (leadingSlash  && !raw.startsWith('/')) raw.prepend('/');
                if (trailingSlash && !raw.endsWith('/'))   raw.append('/');

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
                m_inputModeIndex = (m_inputModeIndex + 1) % 3;
                if (m_inputModeIndex == 0) { m_inputMode->setText("SCRAMBLE"); m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); }
                else if (m_inputModeIndex == 1) { m_inputMode->setText("ALG");  m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); }
                else                           { m_inputMode->setText("POSITION"); m_mainInput->setPlaceholderText("ABCDEFGH12345678-"); }
                m_mainInput->clear(); });

    connect(m_inputModeArrow, &QPushButton::clicked, this, [this]
            {
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
                connect(aScram, &QAction::triggered, this, [this]{ m_inputModeIndex = 0; m_inputMode->setText("SCRAMBLE"); m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); m_mainInput->clear(); });
                connect(aAlg,   &QAction::triggered, this, [this]{ m_inputModeIndex = 1; m_inputMode->setText("ALG");      m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)"); m_mainInput->clear(); });
                connect(aPos,   &QAction::triggered, this, [this]{ m_inputModeIndex = 2; m_inputMode->setText("POSITION"); m_mainInput->setPlaceholderText("ABCDEFGH12345678-");                        m_mainInput->clear(); });
                menu->exec(m_inputModeArrow->mapToGlobal(QPoint(0, m_inputModeArrow->height()))); });

    connect(m_mainInput, &QLineEdit::textChanged, this, [this](const QString &text)
            {
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

    btnScrollToBottom = new QPushButton("↓", outputWrapper);
    btnScrollToBottom->setObjectName("btnScrollToBottom");
    btnScrollToBottom->setFixedSize(32, 32);
    btnScrollToBottom->setToolTip("Scroll to bottom / resume auto-scroll");
    btnScrollToBottom->setVisible(false);
    btnScrollToBottom->setCursor(Qt::PointingHandCursor);

    btnExpand->setVisible(false);
    btnCopyTerminal->setVisible(false);
    btnTableMode->setVisible(false);

    outputWrapperLay->addWidget(txtOutput);
    auto pauseAutoScroll = [this]() {
        if (!m_autoScrollPaused && worker && worker->isRunning()) {
            m_autoScrollPaused = true;
            btnScrollToBottom->setGraphicsEffect(nullptr);
            btnScrollToBottom->setText("⌄");
            btnScrollToBottom->setStyleSheet(""); // revert to QSS default
            int w = m_outputWrapper->width();
            int h = m_outputWrapper->height();
            btnScrollToBottom->move(w - 6 - 28 - 16, h - 6 - 32 - 6);
            btnScrollToBottom->raise();
            btnScrollToBottom->setVisible(true);
        }
    };
    connect(txtOutput->verticalScrollBar(), &QScrollBar::sliderPressed, this, pauseAutoScroll);
    connect(txtOutput->verticalScrollBar(), &QScrollBar::actionTriggered, this, [pauseAutoScroll](int action) {
        // SliderSingleStepAdd/Sub = arrow keys/buttons, SliderPageStepAdd/Sub = track click
        if (action == QAbstractSlider::SliderPageStepAdd ||
            action == QAbstractSlider::SliderPageStepSub ||
            action == QAbstractSlider::SliderMove)
            pauseAutoScroll();
    });

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
                // Always copy clean (non-abid) text regardless of display mode.
                const QStringList &lines = chkKarnotation->isChecked() ? m_karnLines : m_rawLines;
                QApplication::clipboard()->setText(lines.join('\n'));
                appendStatusLine("Terminal copied to clipboard!"); });
    connect(btnScrollToBottom, &QPushButton::clicked, this, [this]
            {
                // If solve is done and we were paused, transition to table
                if (m_solveFinishedWhilePaused) {
                    m_autoScrollPaused = false;
                    m_solveFinishedWhilePaused = false;
                    btnScrollToBottom->setVisible(false);
                    m_tableVisible = true;
                    txtOutput->setVisible(false);
                    m_tableContainer->setVisible(true);
                    btnTableMode->setText("▤");
                    btnTableMode->setToolTip("Switch to terminal view");
                    rebuildTable();
                    return;
                }
                // Otherwise just resume auto-scroll
                m_autoScrollPaused = false;
                m_solveFinishedWhilePaused = false;
                btnScrollToBottom->setVisible(false);
                btnScrollToBottom->setText("⌄");
                btnScrollToBottom->setGraphicsEffect(nullptr);
                btnScrollToBottom->setText("⌄");
                btnScrollToBottom->setStyleSheet(
                    "QPushButton#btnScrollToBottom {"
                    "  background: #2a2a4a; border: 1px solid #4a4a7a; border-radius: 16px;"
                    "  color: #aaaaff; font-size: 16px; font-weight: bold;"
                    "  padding: 0px; margin: 0px; text-align: center; line-height: 32px; }"
                    "QPushButton#btnScrollToBottom:hover { background: #3a3a6a; }");
                btnScrollToBottom->setToolTip("Scroll to bottom / resume auto-scroll");
                txtOutput->verticalScrollBar()->setValue(
                    txtOutput->verticalScrollBar()->maximum());
            });
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
                    // Error color is theme-dependent; pick the right value directly.
                    QString errCol = Theme::textError(m_lightTheme);
                    txtCommand->setStyleSheet(QString(
                        "QLineEdit#txtCommand { font-family: monospace; color: %1;"
                        " font-size: 12px; border-color: %1; border-right: none;"
                        " border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; }")
                        .arg(errCol));
                };
                auto clearCmdError = [this]() {
                    lblCommandError->setVisible(false);
                    txtCommand->setStyleSheet(""); // revert to QSS
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
                clearCmdError(); });

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

    // Helper: insert a solution line with optional Abid font on the alg portion.
    auto insertSolLine = [this](QTextCursor &cur, const QString &line, const QTextCharFormat &fmt) {
        if (!m_abidNotation || m_abidFontFamily.isEmpty()) {
            cur.insertText(line, fmt);
            return;
        }
        int lb = line.lastIndexOf('[');
        QString algPart     = lb > 0 ? line.left(lb).trimmed() : line;
        QString bracketPart = lb > 0 ? "  " + line.mid(lb).trimmed() : QString();
        QTextCharFormat abidFmt = fmt;
        abidFmt.setFontFamily(m_abidFontFamily);
        abidFmt.setFontPointSize(fmt.fontPointSize() + 2);
        cur.insertText(abidifyDisplay(algPart), abidFmt);
        if (!bracketPart.isEmpty())
            cur.insertText(bracketPart, fmt);
    };

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
            insertSolLine(cur, line, fmt);
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
            cur.insertText(line, fmt);
        }
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
        chkCubeshape->setEnabled(cubeWidget->inCubeshape());

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
    txtDepths->setProperty("inactive", !isDepthsNow);
    txtDepths->style()->polish(txtDepths);

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
    QString ss = ::buildStyleSheet(m_lightTheme);
    bool l = m_lightTheme;

    return ss;
}

// -------------------------------------------------------
// buildArgList
// -------------------------------------------------------
QStringList MainWindow::buildArgList()
{
    QStringList args;

    // Metric radio: 0=Slice (default/UI), 1=Move (solver default), 2=Angle
    // Slice is the UI default but the solver defaults to TURN_METRIC, so always emit the flag.
    {
        int id = m_metricGroup ? m_metricGroup->checkedId() : 0;
        if (id == 0)
            args << "-es"; // slice
        else if (id == 2)
            args << "-ea"; // angle
        // id == 1 (move/turn): no flag — solver default
    }

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
    // Angle lock radio: 0=Both, 1=Top, 2=Bottom, 3=None (default — no flag)
    {
        int id = m_angleGroup ? m_angleGroup->checkedId() : 3;
        if (id == 0)
            args << "-nb";
        else if (id == 1)
            args << "-nu";
        else if (id == 2)
            args << "-nd";
        // id == 3 (None): no flag
    }

    // Normalize ABF radio: 0=Both, 1=PreABF, 2=PostABF, 3=None (default — no flag)
    {
        int id = m_normalizeAbfGroup ? m_normalizeAbfGroup->checkedId() : 3;
        if (id == 0)
            args << "-ob";
        else if (id == 1)
            args << "-oe";
        else if (id == 2)
            args << "-os";
        // id == 3 (None): no flag
    }

    if (chkMaxX->isChecked())
        args << QString("-X%1").arg(spnMaxX->value());
    if (chkMaxY->isChecked())
        args << QString("-Y%1").arg(spnMaxY->value());
    if (chkMaxTotal->isChecked())
        args << QString("-Z%1").arg(spnMaxTotal->value());

    return args;
}

void MainWindow::updateCommand()
{
    QString pos = cubeWidget->getPositionString();
    QStringList args = buildArgList();
    txtCommand->setText("sq1opt " + args.join(" ") + " " + pos);
    lblCommandError->setVisible(false);
    txtCommand->setStyleSheet(""); // revert any error-state override to QSS
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
    m_autoScrollPaused = false;
    m_solveFinishedWhilePaused = false;
    btnScrollToBottom->setVisible(false);
    txtOutput->clear();
    m_rawLines.clear();
    m_karnLines.clear();
    m_solutionLines.clear();
    m_karnSolutionLines.clear();
    m_solutionLinesForRating.clear();
    m_sliceIndicators.clear();
    m_rawFinalScores.clear();
    m_cachedRatedOrder.clear();
    m_ratingsValid = false;
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
    btnSolve->setText("■  Stop  [Ctrl+↵]");
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
    chkKarnotation->setEnabled(false);
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
// injectSliceIndicatorDisplay (file-scope helper)
// Injects sliceStr at the first slice boundary of a display line (alg part only).
// The bracket "[x|y]" is never touched.
//
// Rules:
//   Raw WCA format  "1,0/..."  → replace first '/'  → "1,0\..."
//   Karn, numeric-first  "10 U ..."  → replace first ' ' → "10|U ..."
//   Karn, letter-first   "U e' ..."  → prepend before first token → "|U e' ..."
//     (a leading letter means the raw alg had a leading slash, so the implicit
//      slice is BEFORE the first karn token, not after it)
// -------------------------------------------------------
static QString injectSliceIndicatorDisplay(const QString &line, const QString &sliceStr)
{
    if (sliceStr.isEmpty())
        return line; // no indicator to inject

    int lb = line.lastIndexOf('[');
    QString algPart = lb > 0 ? line.left(lb).trimmed() : line.trimmed();
    QString rest = lb > 0 ? "  " + line.mid(lb).trimmed() : QString();

    if (algPart.isEmpty())
        return line;

    // Letter-first → implicit leading slice is before this token
    if (algPart[0].isLetter())
        return sliceStr + algPart + rest;

    // Numeric-first: prefer first '/' (raw WCA), fall back to first ' ' (karn)
    int p = algPart.indexOf('/');
    if (p < 0)
        p = algPart.indexOf(' ');
    if (p < 0)
        return line; // single-move alg — nowhere to inject

    return algPart.left(p) + sliceStr + algPart.mid(p + 1) + rest;
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

        // ── Step 1: store CLEAN line for normalisation pass in onSolverDone ─────
        // Must happen before any modification of 'line'.
        m_solutionLinesForRating.append(line);

        // ── Step 2: rating (cubeshape solves only) ────────────────────────────
        // One rateAlg call captures both sliceStart (for display) and FINAL score
        // (stored for the post-solve normalisation pass).  If cubeshape is off,
        // nothing rating-related happens at all — no injection, no indicators.
        QString sliceStr;
        if (m_cubeshapeWasActive)
        {
            bool initial_top_A = false;
            if (!m_posHex.isEmpty())
            {
                QChar first = m_posHex[0];
                initial_top_A = first.isDigit() || first == 'X' || first == 'Y' || first == 'Z';
            }
            double rawFinal = std::numeric_limits<double>::quiet_NaN();
            try
            {
                AlgRating rating = rateAlg(algKey.toStdString(), initial_top_A, 34, 100, 38, 10);
                if (rating.valid)
                {
                    sliceStr = QString::fromStdString(rating.sliceStart);
                    rawFinal = rating.FINAL;
                }
            }
            catch (...)
            {
            }
            m_sliceIndicators.append(sliceStr); // "" if rateAlg failed
            m_rawFinalScores.append(rawFinal);  // NaN if rateAlg failed
        }

        // ── Step 3: inject indicator into raw display line (cubeshape only) ──
        QString injectedLine = injectSliceIndicatorDisplay(line, sliceStr);

        // ── Step 4: karnify the CLEAN line, then inject (cubeshape only) ──────
        // convertLine receives the original (unmodified) 'line' so karnify always
        // gets valid '/' syntax.  Injection happens into the karn result afterward.
        karnLine = injectSliceIndicatorDisplay(convertLine(line), sliceStr);

        // Cache both display versions
        m_solutionLines.append(injectedLine);
        m_karnSolutionLines.append(karnLine);

        // Overwrite 'line' so the raw-display path below uses the injected version
        line = injectedLine;
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
            // Wipe the "Initializing…" / "searching depth N" / "Flags: …" / "Position: …"
            // lines from both the display and the raw/karn caches.  We keep only entries
            // that are themselves solution lines (contain '[' and ']').
            auto isSol = [](const QString &s)
            { return s.contains('[') && s.contains(']'); };
            m_rawLines.erase(std::remove_if(m_rawLines.begin(), m_rawLines.end(),
                                            [&](const QString &s)
                                            { return !isSol(s); }),
                             m_rawLines.end());
            m_karnLines.erase(std::remove_if(m_karnLines.begin(), m_karnLines.end(),
                                             [&](const QString &s)
                                             { return !isSol(s); }),
                              m_karnLines.end());
            txtOutput->clear();
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
            if (m_abidNotation && !m_abidFontFamily.isEmpty()) {
                int lb = displayLine.lastIndexOf('[');
                QString algPart     = lb > 0 ? displayLine.left(lb).trimmed() : displayLine;
                QString bracketPart = lb > 0 ? "  " + displayLine.mid(lb).trimmed() : QString();
                QTextCharFormat abidFmt = fmt;
                abidFmt.setFontFamily(m_abidFontFamily);
                abidFmt.setFontPointSize(fmt.fontPointSize() + 2);
                cur.insertText(abidifyDisplay(algPart), abidFmt);
                if (!bracketPart.isEmpty())
                    cur.insertText(bracketPart, fmt);
            } else {
                cur.insertText(displayLine, fmt);
            }
            {
                int saved = txtOutput->verticalScrollBar()->value();
                txtOutput->setTextCursor(cur);
                if (m_autoScrollPaused)
                    txtOutput->verticalScrollBar()->setValue(saved);
                else
                    txtOutput->verticalScrollBar()->setValue(
                        txtOutput->verticalScrollBar()->maximum());
            }
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
        {
            int saved = txtOutput->verticalScrollBar()->value();
            txtOutput->setTextCursor(cur);
            if (m_autoScrollPaused)
                txtOutput->verticalScrollBar()->setValue(saved);
            else
                txtOutput->verticalScrollBar()->setValue(
                    txtOutput->verticalScrollBar()->maximum());
        }
    }
}

// -------------------------------------------------------
// onSolverDone
// -------------------------------------------------------
void MainWindow::onSolverDone(int code)
{
    progressBar->setVisible(false);
    chkKarnotation->setEnabled(true);
    btnSolve->setText("▶  Solve  [Ctrl+↵]");
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

    // ── Ergonomic rating cache (cubeshape solves only) ────────────────────────
    // Scores were already computed per-line in onSolverLine (one rateAlg call each).
    // Here we only normalise and sort — no rateAlg calls.
    if (m_cubeshapeWasActive && !m_rawFinalScores.isEmpty())
    {
        // Collect valid (non-NaN) scores for median computation
        std::vector<double> valid;
        valid.reserve(m_rawFinalScores.size());
        for (double s : m_rawFinalScores)
            if (!std::isnan(s))
                valid.push_back(s);

        // Median-normalise
        double median = 0.0;
        if (!valid.empty())
        {
            std::sort(valid.begin(), valid.end());
            size_t nv = valid.size();
            median = (nv % 2 == 0)
                         ? (valid[nv / 2 - 1] + valid[nv / 2]) / 2.0
                         : valid[nv / 2];
        }

        QVector<QPair<int, double>> indexScores;
        indexScores.reserve(m_rawFinalScores.size());
        for (int i = 0; i < m_rawFinalScores.size(); i++)
        {
            double sc = m_rawFinalScores[i];
            indexScores.append({i, std::isnan(sc) ? sc : sc - median});
        }

        // Sort highest first, NaN last
        std::stable_sort(indexScores.begin(), indexScores.end(),
                         [](const QPair<int, double> &a, const QPair<int, double> &b)
                         {
                             bool aN = std::isnan(a.second), bN = std::isnan(b.second);
                             if (aN && bN)
                                 return false;
                             if (aN)
                                 return false;
                             if (bN)
                                 return true;
                             return a.second > b.second;
                         });
        m_cachedRatedOrder = indexScores;
        m_ratingsValid = true;
    }

    if (!m_solutionLines.isEmpty())
    {
        if (m_autoScrollPaused) {
            // User is browsing — flag it and animate the scroll button
            m_solveFinishedWhilePaused = true;
            // Animate: checkmark for 1.5s, then table icon
            // Animate text color from transparent to green (fade-in the checkmark content only)
            btnScrollToBottom->setText("✓");
            btnScrollToBottom->setToolTip("Done!");
            {
                // Use a QTimer to step the text color alpha from 0 to full
                auto *stepTimer = new QTimer(btnScrollToBottom);
                stepTimer->setInterval(16);
                auto *step = new int(0);
                auto applyCheckStyle = [this](int alpha) {
                    btnScrollToBottom->setStyleSheet(QString(
                        "QPushButton#btnScrollToBottom {"
                        "  background: #1a5c1a; border: 1px solid #2ecc40; border-radius: 16px;"
                        "  color: rgba(46,204,64,%1); font-size: 18px; font-weight: bold;"
                        "  padding: 0px; margin: 0px; text-align: center; }"
                        "QPushButton#btnScrollToBottom:hover { background: #236b23; }")
                        .arg(alpha));
                };
                applyCheckStyle(0);
                connect(stepTimer, &QTimer::timeout, this, [this, stepTimer, step, applyCheckStyle]() mutable {
                    *step += 1;
                    int alpha = qMin((int)(*step * 255 / 22), 255); // ~350ms at 16ms steps
                    applyCheckStyle(alpha);
                    if (*step >= 22) { stepTimer->stop(); delete step; }
                });
                stepTimer->start();
            }

            // After 1.5s: fade out checkmark text, swap to table icon, fade in
            QTimer::singleShot(1500, this, [this] {
                if (!m_solveFinishedWhilePaused) return;
                auto *stepOut = new QTimer(btnScrollToBottom);
                stepOut->setInterval(16);
                auto *stepO = new int(0);
                auto applyCheckFade = [this](int alpha) {
                    btnScrollToBottom->setStyleSheet(QString(
                        "QPushButton#btnScrollToBottom {"
                        "  background: #1a5c1a; border: 1px solid #2ecc40; border-radius: 16px;"
                        "  color: rgba(46,204,64,%1); font-size: 18px; font-weight: bold;"
                        "  padding: 0px; margin: 0px; text-align: center; }"
                        "QPushButton#btnScrollToBottom:hover { background: #236b23; }")
                        .arg(alpha));
                };
                connect(stepOut, &QTimer::timeout, this, [this, stepOut, stepO, applyCheckFade]() mutable {
                    *stepO += 1;
                    int alpha = qMax(255 - (int)(*stepO * 255 / 14), 0); // ~220ms
                    applyCheckFade(alpha);
                    if (*stepO >= 14) {
                        stepOut->stop(); delete stepO;
                        // Now swap to table icon and fade in
                        btnScrollToBottom->setText("⊞");
                        btnScrollToBottom->setToolTip("Go to table view");
                        auto *stepIn = new QTimer(btnScrollToBottom);
                        stepIn->setInterval(16);
                        auto *stepI = new int(0);
                        auto applyTableStyle = [this](int alpha) {
                            btnScrollToBottom->setStyleSheet(QString(
                                "QPushButton#btnScrollToBottom {"
                                "  background: #2a2a4a; border: 1px solid #7a6aaa; border-radius: 16px;"
                                "  color: rgba(204,170,255,%1); font-size: 18px; font-weight: bold;"
                                "  padding: 0px; margin: 0px; text-align: center; }"
                                "QPushButton#btnScrollToBottom:hover { background: #3a3a6a; }")
                                .arg(alpha));
                        };
                        applyTableStyle(0);
                        connect(stepIn, &QTimer::timeout, this, [this, stepIn, stepI, applyTableStyle]() mutable {
                            *stepI += 1;
                            int alpha = qMin((int)(*stepI * 255 / 22), 255);
                            applyTableStyle(alpha);
                            if (*stepI >= 22) { stepIn->stop(); delete stepI; }
                        });
                        stepIn->start();
                    }
                });
                stepOut->start();
            });
        } else {
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

void MainWindow::appendStatusLine(const QString &msg)
{
    QString col = Theme::textTerminal(m_lightTheme);
    int savedScroll = txtOutput->verticalScrollBar()->value();
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
    // Move cursor to end without triggering scroll — use a temp cursor
    QTextCursor endCur = txtOutput->textCursor();
    endCur.movePosition(QTextCursor::End);
    txtOutput->setTextCursor(endCur);
    if (m_autoScrollPaused)
        txtOutput->verticalScrollBar()->setValue(savedScroll);
    else
        txtOutput->verticalScrollBar()->setValue(
            txtOutput->verticalScrollBar()->maximum());
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
        moves = 0;
        slices = 0;
        int lb = line.lastIndexOf('[');
        int rb = line.lastIndexOf(']');
        if (lb < 0 || rb < 0)
            return;
        QString bracket = line.mid(lb + 1, rb - lb - 1);
        QStringList parts = bracket.split('|');
        if (parts.size() >= 2)
        {
            slices = parts[0].trimmed().toInt();
            moves = parts[1].trimmed().toInt();
        }
    };

    auto stripBracket = [](const QString &line) -> QString
    {
        int lb = line.lastIndexOf('[');
        return lb > 0 ? line.left(lb).trimmed() : line.trimmed();
    };

    struct Row
    {
        QString alg;
        int moves;
        int slices;
        double ergo;
    };
    QVector<Row> rows;

    bool useKarn = chkKarnotation->isChecked();
    const QStringList &displayLines = useKarn ? m_karnSolutionLines : m_solutionLines;

    if (showErgo && m_ratingsValid)
    {
        // Use the pre-computed cache — no rateAlg calls here.
        // m_cachedRatedOrder is already sorted highest-first.
        for (auto &[idx, score] : m_cachedRatedOrder)
        {
            if (idx < 0 || idx >= displayLines.size())
                continue;
            const QString &dline = displayLines[idx];
            int mv, sl;
            parseCounts(dline, mv, sl);
            int lb = dline.lastIndexOf('[');
            QString alg = lb > 0 ? dline.left(lb).trimmed() : dline.trimmed();
            rows.append({alg, mv, sl, score});
        }
    }
    else
    {
        // No ergo rating — build rows from display lines in arrival order
        for (const QString &line : std::as_const(displayLines))
        {
            int mv, sl;
            parseCounts(line, mv, sl);
            int lb = line.lastIndexOf('[');
            QString alg = lb > 0 ? line.left(lb).trimmed() : line.trimmed();
            rows.append({alg, mv, sl, 0.0});
        }
    }

    // Sort: ergo rank already baked into row order from cache; for non-ergo sort by moves/slices
    if (!(ergo && showErgo && m_ratingsValid))
        std::stable_sort(rows.begin(), rows.end(), [](const Row &a, const Row &b)
                         {
            if (a.slices != b.slices) return a.slices < b.slices;
            return a.moves < b.moves; });

    const QColor rowA = QColor(Theme::rowAltDark(m_lightTheme));
    const QColor rowB = m_lightTheme ? rowA : QColor(Theme::rowAltLight(m_lightTheme));
    const QColor textCol = QColor(Theme::textSolution(m_lightTheme));
    const QColor metaCol = QColor(Theme::textSecondary(m_lightTheme));
    const int rowH = m_expanded ? 36 : 24;
    const int fontSize = m_expanded ? 15 : 12;

    m_solutionTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); i++)
    {
        const Row &r = rows[i];
        QColor bg = (i % 2 == 0) ? rowA : rowB;

        auto cell = [&](int col, const QString &txt, bool isMeta = false)
        {
            QTableWidgetItem *item = new QTableWidgetItem(txt);
            item->setBackground(bg);
            item->setForeground(isMeta ? metaCol : textCol);
            item->setFlags(Qt::ItemIsEnabled); // not selectable
            if (m_expanded)
            {
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
        QString algDisplay = (m_abidNotation && !m_abidFontFamily.isEmpty())
                             ? abidifyDisplay(r.alg) : r.alg;
        QLabel *algLabel = new QLabel(algDisplay);
        algLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
        algLabel->setContextMenuPolicy(Qt::CustomContextMenu);
        algLabel->setProperty("cleanAlg", r.alg);
        algLabel->setCursor(Qt::ArrowCursor);
        algLabel->setContentsMargins(4, 0, 4, 0);
        if (m_abidNotation && !m_abidFontFamily.isEmpty()) {
            // Font family must live inside the stylesheet — setFont() is overridden by it.
            // Bump point size +2 to match the kompact font's smaller cap-height.
            int abidPt = fontSize + 2;
            algLabel->setStyleSheet(QString(
                "QLabel { background: %1; color: %2; font-family: '%3'; font-size: %4pt; }")
                .arg(bg.name(), textCol.name(), m_abidFontFamily, QString::number(abidPt)));
        } else {
            algLabel->setStyleSheet(QString("QLabel { background: %1; color: %2; %3 }")
                                        .arg(bg.name(),
                                             textCol.name(),
                                             m_expanded ? QString("font-size: %1pt;").arg(fontSize) : QString()));
        }
        // Install event filter to show IBeam only when hovering over the text itself
        algLabel->installEventFilter(this);
        m_solutionTable->setCellWidget(i, 1, algLabel);

        cell(2, QString::number(r.moves), true);
        cell(3, QString::number(r.slices), true);
        if (showErgo)
        {
            if (std::isnan(r.ergo))
            {
                // Rating failed for this alg — show a red warning icon instead of a score
                auto *warn = new QLabel("⚠");
                warn->setAlignment(Qt::AlignCenter);
                warn->setStyleSheet(QString(
                                        "QLabel { color: #cc2020; background: %1; font-size: 14px; }")
                                        .arg(bg.name()));
                m_solutionTable->setCellWidget(i, 4, warn);
            }
            else
            {
                cell(4, QString::number(r.ergo, 'f', 1), true);
            }
        }
        m_solutionTable->setRowHeight(i, rowH);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    applyZoom();
}

void MainWindow::applyZoom()
{
    if (!m_zoomProxy || !m_zoomScene || !m_zoomView) return;
    QTransform t;
    t.scale(m_zoomScale, m_zoomScale);
    m_zoomView->setTransform(t);
    QSizeF inner(width() / m_zoomScale, height() / m_zoomScale);
    m_zoomProxy->resize(inner);
    m_zoomScene->setSceneRect(0, 0, inner.width(), inner.height());

    // Treat logical width exactly like a browser viewport:
    // zoom in = smaller logical width = smaller left panel
    // zoom out = larger logical width = larger left panel
    int logicalW = static_cast<int>(inner.width());
    int leftW = qBound(260, 260 + (logicalW - 760) / 5, 420);
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
    if (!m_ratingsValid)
        return;
    lblStatus->setText("Rating algorithms…");

    const bool useKarn = chkKarnotation->isChecked();
    const QStringList &displaySols = useKarn ? m_karnSolutionLines : m_solutionLines;

    txtOutput->clear();
    QTextCursor cur(txtOutput->document());
    bool firstBlock = true;

    // insertLine: plain text, no abid transformation (used for status / non-sol lines)
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

    // insertSolLine: solution line with optional abid font on the alg portion
    auto insertSolLine = [&](const QString &algPart, const QString &suffix,
                             const QString &color, bool bold, int ptSize, int lineH)
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
        if (m_abidNotation && !m_abidFontFamily.isEmpty()) {
            QTextCharFormat abidFmt = fmt;
            abidFmt.setFontFamily(m_abidFontFamily);
            abidFmt.setFontPointSize(ptSize + 2);
            cur.insertText(abidifyDisplay(algPart), abidFmt);
            if (!suffix.isEmpty())
                cur.insertText(suffix, fmt);
        } else {
            cur.insertText(algPart + suffix, fmt);
        }
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

    // Rated solution lines — read from cache (already sorted, no re-rating)
    int solIdx = 0;
    for (auto &[idx, score] : m_cachedRatedOrder)
    {
        if (idx < 0 || idx >= displaySols.size())
        {
            solIdx++;
            continue;
        }
        const QString &dline = displaySols[idx];
        int lb = dline.lastIndexOf('[');
        QString displayAlg  = lb > 0 ? dline.left(lb).trimmed() : dline.trimmed();
        QString bracketPart = lb > 0 ? dline.mid(lb).trimmed() : QString();

        bool isAlt = (solIdx % 2 == 1);
        QString col = m_lightTheme
                          ? (isAlt ? Theme::solutionAltLight(true) : Theme::solutionPrimary(true))
                          : (isAlt ? Theme::solutionAltLight(false) : Theme::textSolution(false));

        if (std::isnan(score))
        {
            QString suffix = (bracketPart.isEmpty() ? QString() : "  " + bracketPart) + "  (⚠)";
            insertSolLine(displayAlg, suffix, "#cc2020", m_expanded, m_expanded ? 13 : 10, m_expanded ? 180 : 120);
        }
        else
        {
            QString suffix = (bracketPart.isEmpty() ? QString() : "  " + bracketPart)
                             + QString("  (%1)").arg(score, 0, 'f', 2);
            insertSolLine(displayAlg, suffix, col, m_expanded, m_expanded ? 13 : 10, m_expanded ? 180 : 120);
        }
        solIdx++;
    }
    appendStatusLine(QString("Ranked %1 algs by ergonomics.").arg((int)m_cachedRatedOrder.size()));
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

// -------------------------------------------------------
// abidifyDisplay
// Converts an alg-only string for display with the Kompact font.
// WCA format  (contains commas): parses a,b pairs and maps digits to
//   the custom codepoints, using the two-sided bar illusion when both
//   values are negative.
// Karn / mixed format (no commas): replaces digits with normal custom
//   codepoints and applies singleBar to negative digit runs.
// The bracket part "[x|y]" must NOT be passed in — strip it first.
// -------------------------------------------------------
QString MainWindow::abidifyDisplay(const QString& algOnly) const
{
    if (m_abidFontFamily.isEmpty() || algOnly.isEmpty())
        return algOnly;

    // Codepoint helpers (values 0-6 only; square-1 never exceeds 6)
    auto normalCp  = [](int d) -> QChar { return QChar(0xe000 + d); };
    auto singleBar = [](int d) -> QChar { return QChar(0xe006 + d); }; // 1→E007…5→E00B
    auto barRight  = [](int d) -> QChar { return QChar(0xe00b + d); }; // 1→E00C…5→E010
    auto barLeft   = [](int d) -> QChar { return QChar(0xe010 + d); }; // 1→E011…5→E015

    auto mapDigits = [&](int absVal, std::function<QChar(int)> mapper) -> QString {
        QString s;
        for (QChar c : QString::number(absVal))
            if (c.isDigit()) s += mapper(c.digitValue());
        return s;
    };

    if (algOnly.contains(',')) {
        // ── WCA format: find every a,b token ─────────────────────────────────
        static const QRegularExpression pairRe(R"((-?\d+),(-?\d+))");
        QString result;
        int last = 0;
        auto it = pairRe.globalMatch(algOnly);
        while (it.hasNext()) {
            auto m = it.next();
            // Pass through non-numeric content (slashes, slice indicators, spaces)
            result += algOnly.mid(last, m.capturedStart() - last);
            int a = m.captured(1).toInt();
            int b = m.captured(2).toInt();
            if (a < 0 && b < 0) {
                result += mapDigits(qAbs(a), barRight);
                result += mapDigits(qAbs(b), barLeft);
            } else {
                result += (a < 0) ? mapDigits(qAbs(a), singleBar)
                                  : mapDigits(qAbs(a), normalCp);
                result += (b < 0) ? mapDigits(qAbs(b), singleBar)
                                  : mapDigits(qAbs(b), normalCp);
            }
            last = m.capturedEnd();
        }
        result += algOnly.mid(last);
        return result;
    } else {
        // ── Karn / stripped-comma format ──────────────────────────────────────
        // Commas were stripped, so (-2,-3) → "-2-3" and (-5,0) → "-50".
        // Sq1 values are always single digits (-6..6), so a '-' always governs
        // exactly ONE following digit. Consume exactly one digit per negative token.
        //
        // Both-negative pair detection: if '-'digit is immediately followed by
        // another '-'digit (no space), apply barRight + barLeft so the bars connect.
        QString result;
        int i = 0;
        while (i < algOnly.size()) {
            QChar c = algOnly[i];
            if (c == '-' && i + 1 < algOnly.size() && algOnly[i + 1].isDigit()) {
                // Peek: is there a second '-'digit immediately after this one?
                bool bothNeg = (i + 2 < algOnly.size() && algOnly[i + 2] == '-' &&
                                i + 3 < algOnly.size() && algOnly[i + 3].isDigit());
                if (bothNeg) {
                    result += barRight(algOnly[i + 1].digitValue()); // first  → barRight
                    i += 2;
                    result += barLeft(algOnly[i + 1].digitValue());  // second → barLeft
                    i += 2;
                } else {
                    result += singleBar(algOnly[i + 1].digitValue());
                    i += 2;
                }
            } else if (c.isDigit()) {
                result += normalCp(c.digitValue());
                ++i;
            } else {
                result += c;
                ++i;
            }
        }
        return result;
    }
}

QString MainWindow::convertLine(const QString &rawLine)
{
    int lb = rawLine.lastIndexOf('[');
    int rb = rawLine.lastIndexOf(']');
    if (lb < 0 || rb < 0)
        return rawLine;

    QString algPart = rawLine.left(lb).trimmed();
    QString bracketPart = rawLine.mid(lb).trimmed();

    std::string converted;
    if (m_smartKarn && !m_cubeshapeWasActive)
    {
        converted = karnifycs(algPart.toStdString(), m_posHex.toStdString(), chkGenerator->isChecked());
    }
    else
    {
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

    // Update command line error label; core txtCommand color is handled by QSS.
    if (m_inputBarOuter)
        m_inputBarOuter->setStyleSheet("");
    cubeWidget->setLightTheme(m_lightTheme);
    FadingTooltip::setLightTheme(m_lightTheme, this);
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
    chkLight->setToolTip("Switch between dark (default) and light theme.\n"
                         "The change applies immediately.");
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
    connect(chkSmart, &QCheckBox::toggled, this, [this](bool checked)
            {
        m_smartKarn = checked;
        if (!m_rawLines.isEmpty()) {
            // Rebuild karn cache with new mode.
            // m_rawLines[i] is the injected raw display line for solutions, or the plain
            // status text for non-solutions. We need the CLEAN line for karnification;
            // those are in m_solutionLinesForRating (parallel to m_solutionLines).
            m_karnLines.clear();
            m_karnSolutionLines.clear();
            int solIdx = 0;
            for (int i = 0; i < m_rawLines.size(); i++) {
                bool isSol = m_rawLines[i].contains('[') && m_rawLines[i].contains(']');
                QString karnLine = m_rawLines[i]; // default for non-solutions
                if (isSol && solIdx < m_solutionLinesForRating.size()) {
                    // Karnify from the clean (uninjected) line, then re-inject the stored indicator
                    QString cleanLine = m_solutionLinesForRating[solIdx];
                    karnLine = convertLine(cleanLine);
                    if (solIdx < m_sliceIndicators.size())
                        karnLine = injectSliceIndicatorDisplay(karnLine, m_sliceIndicators[solIdx]);
                    m_karnSolutionLines.append(karnLine);
                    solIdx++;
                }
                m_karnLines.append(karnLine);
            }
            if (chkKarnotation->isChecked()) {
                if (m_tableVisible) rebuildTable();
                else if (chkRankErgo->isChecked()) onRankErgoToggled(true);
                else rebuildTerminalView();
            }
        } });
    lay->addWidget(chkSmart);

    // ── Abid's Notation ───────────────────────────────────────────────────────
    QCheckBox *chkAbid = new QCheckBox("Abid's notation");
    chkAbid->setChecked(m_abidNotation && !m_abidFontFamily.isEmpty());
    chkAbid->setEnabled(!m_abidFontFamily.isEmpty());
    chkAbid->setToolTip("Display negative numbers as barred digits using the Kompact font\n"
                        "(e.g. -3 → 3̄) for a cleaner look. Displayed only — copies\n"
                        "always use standard notation with minus signs.");
    if (m_abidFontFamily.isEmpty())
        chkAbid->setToolTip(chkAbid->toolTip() +
                            "\n\n⚠ kompact-font.ttf not found — visual effect unavailable.");
    chkAbid->setStyleSheet(QString("color:%1;background:transparent;font-size:13px;").arg(textPrimary));
    connect(chkAbid, &QCheckBox::toggled, this, [this](bool checked) {
        m_abidNotation = checked;
        if (!m_rawLines.isEmpty()) {
            if (m_tableVisible) rebuildTable();
            else if (chkRankErgo->isChecked()) onRankErgoToggled(true);
            else rebuildTerminalView();
        }
    });
    lay->addWidget(chkAbid);

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
    if (event->type() == QEvent::MouseMove || event->type() == QEvent::HoverMove)
    {
        QWidget *under = QApplication::widgetAt(QCursor::pos());
        if (QCheckBox *cb = qobject_cast<QCheckBox *>(under))
        {
            QPoint localPos = cb->mapFromGlobal(QCursor::pos());
            QStyleOptionButton opt;
            opt.initFrom(cb);
            QRect indRect = cb->style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, cb);
            QFontMetrics fm(cb->font());
            QRect textRect(indRect.right() + 6, 0, fm.horizontalAdvance(cb->text()), cb->height());
            QRect activeRect = indRect.united(textRect);
            if (activeRect.contains(localPos))
            {
                if (cb != chkDepths)
                    cb->setCursor(Qt::PointingHandCursor);
                if (!cb->toolTip().isEmpty())
                    FadingTooltip::arm(cb->toolTip(), QCursor::pos(), this);
            }
            else
            {
                cb->setCursor(Qt::ArrowCursor);
                FadingTooltip::dismiss(this);
            }
        }
        else
        {
            FadingTooltip::dismiss(this);
        }
    }

    // ── Right-click copy on table-cell labels (always copies clean notation) ──
    if (event->type() == QEvent::ContextMenu)
    {
        if (QLabel *lbl = qobject_cast<QLabel *>(watched)) {
            QVariant v = lbl->property("cleanAlg");
            if (v.isValid()) {
                QMenu menu;
                QAction *copyAct = menu.addAction("Copy");
                QAction *chosen = menu.exec(
                    static_cast<QContextMenuEvent *>(event)->globalPos());
                if (chosen == copyAct)
                    QApplication::clipboard()->setText(v.toString());
                return true;
            }
        }
    }

    // ── IBeam cursor only over actual label text ──────────────────────────────
    if (event->type() == QEvent::MouseMove)
    {
        if (QLabel *lbl = qobject_cast<QLabel *>(watched))
        {
            if (lbl->parent() && qobject_cast<QTableWidget *>(lbl->parent()->parent()))
            {
                QMouseEvent *me = static_cast<QMouseEvent *>(event);
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
        int h = m_outputWrapper->height();
        int margin = 6;
        int bw = 22;
        btnExpand->move(w - margin - bw, margin);
        btnTableMode->move(w - margin - bw * 2 - 4, margin);
        btnCopyTerminal->move(w - margin - bw * 3 - 8, margin);
        btnScrollToBottom->move(w - margin - 28 - 16, h - margin - 32 - margin);
        return false;
    }

    // ── Scroll-up on terminal viewport pauses auto-scroll ────────────────────
    if (event->type() == QEvent::Wheel && txtOutput && watched == txtOutput->viewport())
    {
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
        bool scrollingUp = we->angleDelta().y() > 0;
        if (scrollingUp && !m_autoScrollPaused && (worker && worker->isRunning()))
        {
            m_autoScrollPaused = true;
            btnScrollToBottom->setGraphicsEffect(nullptr);
            btnScrollToBottom->setText("⌄");
            btnScrollToBottom->setStyleSheet(""); // revert to QSS default
            int w = m_outputWrapper->width();
            int h = m_outputWrapper->height();
            btnScrollToBottom->move(w - 6 - 28 - 16, h - 6 - 32 - 6);
            btnScrollToBottom->raise();
            btnScrollToBottom->setVisible(true);
        }
        return false; // let the scroll happen normally
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
        // If Ctrl+C fires on a table-cell label while Abid's notation is on,
        // copy the clean (minus-sign) text instead of the PUA glyphs.
        if (m_abidNotation && !m_abidFontFamily.isEmpty()) {
            if (QLabel *lbl = qobject_cast<QLabel *>(watched)) {
                QVariant v = lbl->property("cleanAlg");
                if (v.isValid()) {
                    QApplication::clipboard()->setText(v.toString());
                    return true;
                }
            }
        }
        return QMainWindow::eventFilter(watched, event);
    }


    // ── Ctrl+= / Ctrl+- — zoom in / out ─────────────────────────────────────
    if (ke->modifiers() == Qt::ControlModifier)
    {
        if (ke->key() == Qt::Key_Equal || ke->key() == Qt::Key_Plus)
        {
            m_zoomScale = qMin(m_zoomScale + 0.1, 2.0);
            applyZoom(); return true;
        }
        if (ke->key() == Qt::Key_Minus)
        {
            m_zoomScale = qMax(m_zoomScale - 0.1, 0.5);
            applyZoom(); return true;
        }
        if (ke->key() == Qt::Key_0)
        {
            m_zoomScale = 1.0; applyZoom(); return true;
        }
    }

    // ── Ctrl+Z / Ctrl+Y — undo / redo (work from any widget, including inputs) ─
    if (ke->modifiers() == Qt::ControlModifier)
    {
        if (ke->key() == Qt::Key_Z)
        {
            if (!m_undoStack.isEmpty())
                btnUndo->click();
            return true;
        }
        if (ke->key() == Qt::Key_Y)
        {
            if (!m_redoStack.isEmpty())
                btnRedo->click();
            return true;
        }
    }

    // ── Events already targeting cubeWidget: let cubeWidget handle them. ─────
    // (sendEvent below will re-enter here with watched == cubeWidget.)
    if (watched == cubeWidget)
        return QMainWindow::eventFilter(watched, event);

    // ── Esc from m_mainInput — reset cube without stealing focus ─────────────
    if ((watched == m_mainInput || QApplication::focusWidget() == m_mainInput) && ke->key() == Qt::Key_Escape && ke->modifiers() == Qt::NoModifier)
    {
        m_redoStack.clear();
        btnUndo->setEnabled(false);
        btnRedo->setEnabled(false);
        QKeyEvent escEv(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(cubeWidget, &escEv);
        return true;
    }

    // ── Enter / Shift+Enter in m_mainInput ───────────────────────────────────
    // Enter        = apply alg/scramble from current cube state
    // Shift+Enter  = apply alg/scramble from solved state (resets first)
    if ((watched == m_mainInput || QApplication::focusWidget() == m_mainInput) && ke->key() == Qt::Key_Return)
    {
        if (ke->modifiers() & Qt::ShiftModifier)
        {
            // Shift+Enter: apply from solved state
            m_applyFromSolved = true;
            btnApply->click();
            m_applyFromSolved = false;
        }
        else
        {
            // plain Enter: apply on top of current cube state
            m_applyFromSolved = false;
            btnApply->click();
        }
        return true;
    }

    // ── Text inputs get all remaining keys — never steal from them ────────────
    {
        QWidget *fw = QApplication::focusWidget();
        if (fw == txtCommand || fw == txtDepths || fw == m_mainInput ||
            watched == txtCommand || watched == txtDepths || watched == m_mainInput ||
            m_mainInput->hasFocus() || txtCommand->hasFocus() || txtDepths->hasFocus())
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
