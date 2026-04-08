#include "mainwindow.h"
#include "sq1widget.h"
#include "karnotation.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QLineEdit>
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
#include <QDialog>
#include <QShortcut>
#include <QDateTime>
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

// ============================================================
// Ergonomics Rating — pure C++ translation of alg_rater.html
// Uses KARNOTATION from karnotation.h for unkarnify.
// ============================================================

static const std::map<int,int> CLOSEST_MAP = {
    {-5,-6},{-4,-3},{-3,-3},{-2,-3},{-1,0},{0,0},
    { 1, 0},{ 2, 3},{ 3, 3},{ 4, 3},{5, 6},{6, 6}
};

// MOVE_VALUES table from alg_rater.html.
// Key format: "<A|a><slash|backslash><top>,<bot>"  or  "<slash|backslash><top>,<bot>"
// A=aligned top (top%3==0), a=not. slash=upslice(odd), backslash=downslice(even).
static const std::map<std::string,int> MOVE_VALUES = {
    {"A/0,3",16},  {"a/0,3",5},   {"A\\0,3",17},  {"a\\0,3",5},
    {"A/0,6",1},   {"a/0,6",5},   {"A\\0,6",1},   {"a\\0,6",5},
    {"A/0,-3",18}, {"a/0,-3",12}, {"A\\0,-3",8},  {"a\\0,-3",5},
    {"A/3,0",16},  {"a/3,0",17},  {"A\\3,0",6},   {"a\\3,0",16},
    {"A/3,3",12},  {"a/3,3",10},  {"A\\3,3",14},  {"a\\3,3",11},
    {"A/3,6",0},   {"a/3,6",5},   {"A\\3,6",1},   {"a\\3,6",4},
    {"A/3,-3",13}, {"a/3,-3",7},  {"A\\3,-3",12}, {"a\\3,-3",6},
    {"A/6,0",12},  {"a/6,0",4},   {"A\\6,0",14},  {"a\\6,0",4},
    {"A/6,3",11},  {"a/6,3",2},   {"A\\6,3",11},  {"a\\6,3",2},
    {"A/6,6",2},   {"a/6,6",0},   {"A\\6,6",5},   {"a\\6,6",0},
    {"A/6,-3",12}, {"a/6,-3",3},  {"A\\6,-3",8},  {"a\\6,-3",1},
    {"A/-3,0",9},  {"a/-3,0",18}, {"A\\-3,0",11}, {"a\\-3,0",15},
    {"A/-3,3",13}, {"a/-3,3",12}, {"A\\-3,3",14}, {"a\\-3,3",10},
    {"A/-3,6",4},  {"a/-3,6",7},  {"A\\-3,6",6},  {"a\\-3,6",2},
    {"A/-3,-3",12},{"a/-3,-3",11},{"A\\-3,-3",9}, {"a\\-3,-3",5},
    {"/1,-2",4},   {"\\1,-2",17}, {"/-1,2",15},   {"\\-1,2",14},
    {"/1,-5",3},   {"\\1,-5",1},  {"/-1,5",8},    {"\\-1,5",3},
    {"/1,4",7},    {"\\1,4",14},  {"/-1,-4",12},  {"\\-1,-4",9},
    {"/1,1",11},   {"\\1,1",20},  {"/-1,-1",20},  {"\\-1,-1",10},
    {"/2,-1",20},  {"\\2,-1",12}, {"/-2,1",14},   {"\\-2,1",18},
    {"/2,2",12},   {"\\2,2",13},  {"/-2,-2",14},  {"\\-2,-2",8},
    {"/2,5",5},    {"\\2,5",3},   {"/-2,-5",4},   {"\\-2,-5",3},
    {"/2,-4",14},  {"\\2,-4",6},  {"/-2,4",13},   {"\\-2,4",13},
    {"/4,4",5},    {"\\4,4",12},  {"/-4,-4",12},  {"\\-4,-4",4},
    {"/4,1",6},    {"\\4,1",13},  {"/-4,-1",16},  {"\\-4,-1",6},
    {"/4,-2",12},  {"\\4,-2",9},  {"/-4,2",16},   {"\\-4,2",13},
    {"/4,-5",2},   {"\\4,-5",5},  {"/-4,5",13},   {"\\-4,5",3},
    {"/5,5",1},    {"\\5,5",4},   {"/-5,-5",2},   {"\\-5,-5",0},
    {"/5,2",6},    {"\\5,2",10},  {"/-5,-2",12},  {"\\-5,-2",13},
    {"/5,-1",11},  {"\\5,-1",7},  {"/-5,1",14},   {"\\-5,1",15},
    {"/5,-4",2},   {"\\5,-4",2},  {"/-5,4",12},   {"\\-5,4",14}
};

static int getMoveValue(bool startA, bool upslice, const std::string& move) {
    std::string key;
    auto comma = move.find(',');
    int topVal = std::stoi(move.substr(0, comma));
    if (topVal % 3 == 0) {
        key = (startA ? "A" : "a");
        key += (upslice ? "/" : "\\");
        key += move;
    } else {
        key = (upslice ? "/" : "\\");
        key += move;
    }
    auto it = MOVE_VALUES.find(key);
    return (it != MOVE_VALUES.end()) ? it->second : 5;
}

static std::vector<std::string> splitStr(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == delim) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

static std::string addCommasToMove(const std::string& move) {
    if (move.empty()) return move;
    for (char c : move)
        if (c != '-' && !std::isdigit((unsigned char)c)) return move;
    switch (move.size()) {
        case 1: return move + ",0";
        case 2: return move[0] == '-' ? move + ",0"
                                      : std::string(1, move[0]) + "," + std::string(1, move[1]);
        case 3: return move[0] == '-' ? move.substr(0,2) + "," + std::string(1, move[2])
                                      : std::string(1, move[0]) + "," + move.substr(1);
        case 4: return move.substr(0,2) + "," + move.substr(2);
        default: return move;
    }
}

// ---------------------------------------------------------------------------
// getAlignment — returns "00","10","0-1","1-1" suffix for shorthand lookup
// ---------------------------------------------------------------------------
static std::string getAlignment(bool topA, bool bottomA) {
    return (topA ? "1" : "0") + std::string(bottomA ? "-1" : "0");
}

// ---------------------------------------------------------------------------
// unkarnifyHelp — apply KARN_TO_WCA dict, collapse consecutive slashes
// ---------------------------------------------------------------------------
static std::string unkarnifyHelp(const std::string& scramble) {
    std::string s = dictReplace(" " + scramble + " ", KARN_TO_WCA);
    // trim
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    s = s.substr(a, b - a + 1);
    // collapse consecutive slashes and spaces-around-slashes
    // replace runs of "/ /" or " /" etc. → single "/"
    // simple approach: replace multiple-slash runs
    std::string out;
    for (size_t i = 0; i < s.size(); ) {
        bool isSlashOrSpace = (s[i] == '/' || s[i] == ' ');
        if (isSlashOrSpace) {
            // scan ahead for the whole run
            size_t j = i;
            bool sawSlash = false;
            while (j < s.size() && (s[j] == '/' || s[j] == ' ')) {
                if (s[j] == '/') sawSlash = true;
                j++;
            }
            if (sawSlash) out += '/';
            else out += ' ';
            i = j;
        } else {
            out += s[i++];
        }
    }
    // replace remaining spaces with '/'
    for (char& c : out) if (c == ' ') c = '/';
    // collapse double slashes
    while (out.find("//") != std::string::npos)
        out = replaceAll(out, "//", "/");
    return out;
}

// ---------------------------------------------------------------------------
// replaceShorthands — resolve shorthand names (bjj, fv, kk, …) tracking
// alignment state to pick the correct alignment-suffixed key.
// ---------------------------------------------------------------------------
static std::string replaceShorthands(const std::string& scrambleIn) {
    // Fast path: if no alpha chars outside of numeric/slash context, skip
    bool hasAlpha = false;
    for (char c : scrambleIn)
        if (std::isalpha((unsigned char)c)) { hasAlpha = true; break; }
    if (!hasAlpha) return unkarnifyHelp(scrambleIn);

    std::vector<std::string> moves = splitStr(scrambleIn, '/');

    bool topA = false, bottomA = false;
    std::string result = scrambleIn;

    for (const auto& move : moves) {
        std::string m = trimStr(move);
        if (m.empty()) continue;

        if (m.find(',') != std::string::npos) {
            // Numeric turn — update alignment
            auto c = m.find(',');
            int t = 0;
            try { t = std::stoi(m.substr(0, c)); } catch (...) {}
            int d = 0;
            try { d = std::stoi(m.substr(c+1)); } catch (...) {}
            if (t % 3 != 0) topA    = !topA;
            if (d % 3 != 0) bottomA = !bottomA;
        } else {
            // Shorthand token
            std::string mLow = m;
            for (char& ch : mLow) ch = std::tolower((unsigned char)ch);

            std::string key;
            if (SHORTHAND_ALIGN_INDEPENDENT.count(mLow))
                key = mLow;
            else
                key = mLow + getAlignment(topA, bottomA);

            auto it = SHORTHAND_TO_KARN.find(key);
            if (it == SHORTHAND_TO_KARN.end()) {
                // Unknown shorthand — return as-is (runtime-safe)
                return scrambleIn;
            }
            std::string repl = it->second;
            result = replaceAll(result, m, repl);

            // Update alignment based on replacement expansion
            std::string expanded = repl;
            if (!expanded.empty() && expanded.front() == '/') expanded = expanded.substr(1);
            if (!expanded.empty() && expanded.back()  == '/') expanded.pop_back();
            for (const auto& sub : splitStr(unkarnifyHelp(expanded), '/')) {
                if (sub.empty()) continue;
                auto c2 = sub.find(',');
                if (c2 == std::string::npos) continue;
                int t = 0, d = 0;
                try { t = std::stoi(sub.substr(0, c2)); } catch (...) {}
                try { d = std::stoi(sub.substr(c2+1));  } catch (...) {}
                if (t % 3 != 0) topA    = !topA;
                if (d % 3 != 0) bottomA = !bottomA;
            }
        }
    }

    // Collapse double-slashes introduced by replacements
    result = replaceAll(result, " / ", "/");
    result = replaceAll(result, "  ", "/");
    while (result.find("//") != std::string::npos)
        result = replaceAll(result, "//", "/");
    return unkarnifyHelp(result);
}

// ---------------------------------------------------------------------------
// unkarnify — master Karnotation → numeric WCA conversion
// Matches karn.js unkarnify() pipeline.
// ---------------------------------------------------------------------------
static std::string unkarnify(const std::string& algIn) {
    std::string s = algIn;

    // Easter egg passthrough
    if (s.find("meow") != std::string::npos) return s;

    // Legacy single-char substitutions (compact notation)
    s = replaceAll(s, "&", "-1");
    s = replaceAll(s, "^", "-2");
    s = replaceAll(s, "9", "-3");
    s = replaceAll(s, "8", "-4");
    s = replaceAll(s, "7", "-5");

    // Detect leading/trailing slice
    bool firstSlice = (!s.empty() && (s[0] == '/' || s[0] == '\\'));
    if (!firstSlice) {
        // Check if first token is a karn name that maps to something starting with /
        std::istringstream iss(s);
        std::string tok;
        if (iss >> tok) {
            auto it = KARN_TO_WCA.find(" " + tok + " ");
            if (it != KARN_TO_WCA.end()) firstSlice = true;
        }
    }
    bool lastSlice = false;
    {
        std::istringstream iss(s);
        std::string last, tok;
        while (iss >> tok) last = tok;
        if (!last.empty()) {
            auto it = KARN_TO_WCA.find(" " + last + " ");
            if (it != KARN_TO_WCA.end()) lastSlice = true;
        }
    }

    // Normalise separators
    for (char& c : s) if (c == '\\' || c == '/') c = ' ';
    // collapse parens
    s = replaceAll(s, "(", "");
    s = replaceAll(s, ")", "");
    // collapse multiple spaces
    {
        std::string tmp;
        bool sp = false;
        for (char c : s) {
            if (c == ' ') { if (!sp) { tmp += ' '; sp = true; } }
            else { tmp += c; sp = false; }
        }
        s = trimStr(tmp);
    }

    // addCommas to each space-separated token
    {
        std::vector<std::string> tokens;
        std::istringstream iss(s);
        std::string tok;
        while (iss >> tok) tokens.push_back(tok);
        s.clear();
        for (size_t i = 0; i < tokens.size(); i++) {
            if (i) s += ' ';
            s += addCommasToMove(tokens[i]);
        }
    }

    // replaceShorthands then full dict-replace
    std::string final_ = replaceShorthands(unkarnifyHelp(s));

    // Re-attach leading/trailing slices
    if (firstSlice && (final_.empty() || final_[0] != '/'))
        final_ = "/" + final_;
    if (lastSlice && (final_.empty() || final_.back() != '/'))
        final_ = final_ + "/";
    // collapse double slashes
    while (final_.find("//") != std::string::npos)
        final_ = replaceAll(final_, "//", "/");

    // addCommas pass on each slash-segment
    auto parts = splitStr(final_, '/');
    final_.clear();
    for (const auto& part : parts) {
        if (!final_.empty()) final_ += "/";
        final_ += addCommasToMove(part);
    }

    return final_;
}

static std::pair<int,int> getOverwork(const std::vector<std::string>& moves) {
    std::vector<int> top, bot;
    for (auto& m : moves) {
        auto c = m.find(',');
        if (c == std::string::npos) { top.push_back(0); bot.push_back(0); continue; }
        try { top.push_back(std::stoi(m.substr(0,c))); } catch(...) { top.push_back(0); }
        try { bot.push_back(std::stoi(m.substr(c+1))); } catch(...) { bot.push_back(0); }
    }

    int movement = 0, bonus = 0;
    int streak = 0, closestMovement = 0, buffer = 0;
    for (int t : top) {
        bool isLeft = (t == 6 || t < 0);
        if (isLeft) {
            streak++;
            auto it = CLOSEST_MAP.find(t);
            closestMovement += std::abs(it != CLOSEST_MAP.end() ? it->second : 0);
            buffer += std::abs(t);
            if (streak > 1 && closestMovement > 3) { movement += buffer; buffer = 0; }
        } else { streak = 0; closestMovement = 0; buffer = 0; }
    }
    streak = 0; closestMovement = 0; buffer = 0;
    for (int b : bot) {
        bool isLeft = (b > 0);
        if (isLeft) {
            streak++;
            auto it = CLOSEST_MAP.find(b);
            closestMovement += std::abs(it != CLOSEST_MAP.end() ? it->second : 0);
            buffer += std::abs(b);
            if (streak > 1 && closestMovement > 3) { movement += buffer; buffer = 0; }
        } else { streak = 0; closestMovement = 0; buffer = 0; }
    }
    for (size_t i = 0; i + 1 < top.size(); i++) {
        if (top[i] + top[i+1] != 0) bonus++;
        if (bot[i] + bot[i+1] != 0) bonus++;
    }
    return {movement, bonus};
}

struct AlgRating {
    double FINAL;
    std::string sliceStart;
};

static AlgRating rateAlg(const std::string& algRaw, bool initial_top_A,
                         double W1, double W2, double W3, double W4, double W5)
{
    std::string a = algRaw;
    { size_t lb = a.find('['); if (lb != std::string::npos) a = a.substr(0, lb); }
    a = trimStr(a);
    bool isKarnAlg = false;
    for (char ch : a) if (std::isalpha(ch)) { isKarnAlg = true; break; }
    std::string numeric = isKarnAlg ? unkarnify(a) : replaceAll(a, " ", "");
    auto rawParts = splitStr(numeric, '/');
    std::vector<std::string> r;
    for (size_t i = 0; i < rawParts.size(); i++) {
        std::string pt = trimStr(rawParts[i]);
        if (i == 0 || !pt.empty()) r.push_back(pt);
    }
    if (r.size() < 2) return {W4, ""};

    int sliceCount = (int)r.size() - 1;
    if (sliceCount <= 0) return {W4, ""};

    double ergo_up = 0, ergo_down = 0;
    bool is_top_A = false, odd_slice = true;
    for (int i = 0; i < (int)r.size() - 1; i++) {
        if (i == 0) {
            auto c = r[i].find(',');
            int t = 0;
            if (c != std::string::npos) try { t = std::stoi(r[i].substr(0,c)); } catch(...) {}
            is_top_A = (initial_top_A != (t % 3 != 0));
            odd_slice = true;
            continue;
        }
        int vu = getMoveValue(is_top_A,  odd_slice, r[i]);
        int vd = getMoveValue(is_top_A, !odd_slice, r[i]);
        ergo_up   += vu;
        ergo_down += vd;
        auto c = r[i].find(',');
        int t = 0;
        if (c != std::string::npos) try { t = std::stoi(r[i].substr(0,c)); } catch(...) {}
        is_top_A  = (is_top_A != (t % 3 != 0));
        odd_slice = !odd_slice;
    }
    double PHASE1 = W1 * std::max(ergo_up, ergo_down) / sliceCount;
    std::string sliceStart;
    if ((std::abs(ergo_up - ergo_down) / sliceCount) > 2) {
        sliceStart = (ergo_up > ergo_down) ? "/" : "\\";
    } else sliceStart = " ";

    double PHASE2 = W2 * sliceCount;
    auto moves = std::vector<std::string>(r.begin() + 1, r.end() - 1);
    auto [movement, bonus] = getOverwork(moves);
    double PHASE3 = W3 * movement / sliceCount;
    double PHASE4 = bonus * W5 / sliceCount;

    double FINAL = PHASE1 - PHASE2 - PHASE3 + PHASE4 + W4;
    return {FINAL, sliceStart};
}

static std::vector<std::pair<QString, double>>
rateAndSort(const QStringList& solutionLines, const QString& posHex, bool useKarnotation) {
    Q_UNUSED(useKarnotation);
    bool initial_top_A = false;
    if (!posHex.isEmpty()) {
        QChar first = posHex[0];
        initial_top_A = first.isDigit() ||
                        first == 'X' || first == 'Y' || first == 'Z';
    }

    const double W1=34, W2=100, W3=38, W4=500, W5=10;
    std::vector<std::pair<QString, double>> results;

    for (const QString& lineIn : solutionLines) {
        QString line = lineIn;
        std::string algStr = line.toStdString();
        auto bracket = algStr.find('[');
        std::string algOnly = bracket != std::string::npos
                                  ? trimStr(algStr.substr(0, bracket))
                                  : trimStr(algStr);
        double score = W4;
        AlgRating rating;
        bool rated = false;
        try {
            // Always convert to numeric for rating
            std::string numericAlg = algOnly;
            bool isKarn = false;
            for (char ch : algOnly) if (std::isalpha((unsigned char)ch)) { isKarn = true; break; }
            if (isKarn) numericAlg = unkarnify(algOnly);

            rating = rateAlg(numericAlg, initial_top_A, W1, W2, W3, W4, W5);
            score = rating.FINAL;
            rated = true;
        } catch (...) {}

        // Inject slice start indicator into display line
        if (rated) {
            QString sliceStr = QString::fromStdString(rating.sliceStart);
            if (sliceStr == "/" || sliceStr == "\\") {
                // Find the first '/' in the line (can only be in the alg part)
                int slashPos = line.indexOf('/');
                if (slashPos >= 0)
                    line = line.left(slashPos) + sliceStr + line.mid(slashPos + 1);
                else {
                    int spacePos = line.indexOf(' ');
                    if (spacePos >= 0)
                        line = line.left(spacePos) + sliceStr + line.mid(spacePos + 1);
                }
            }
        }

        results.push_back({line, score});
    }
    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });
    return results;
}

// ============================================================
// FastTipStyle — QProxyStyle that makes tooltips appear instantly.
// Also extends the fall-asleep delay so tooltips linger naturally.
// ============================================================
class FastTipStyle : public QProxyStyle {
public:
    using QProxyStyle::QProxyStyle;
    int styleHint(StyleHint hint, const QStyleOption* opt = nullptr,
                  const QWidget* widget = nullptr,
                  QStyleHintReturn* ret = nullptr) const override {
        if (hint == QStyle::SH_ToolTip_WakeUpDelay)    return 0;     // instant
        if (hint == QStyle::SH_ToolTip_FallAsleepDelay) return 8000; // linger 8 s
        return QProxyStyle::styleHint(hint, opt, widget, ret);
    }
};

// -------------------------------------------------------
// SolverWorker
// -------------------------------------------------------
void SolverWorker::requestStop() {
    // Safe to call from any thread: m_proc is atomic.
    QProcess* p = m_proc.load();
    if (p) p->kill();
}

void SolverWorker::run() {
    QString exePath = QCoreApplication::applicationDirPath() + "/sq1opt";
#ifdef Q_OS_WIN
    exePath += ".exe";
#endif
    QProcess proc;
    // Publish pointer so requestStop() can kill it from the main thread.
    m_proc.store(&proc);

    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.setWorkingDirectory(QCoreApplication::applicationDirPath());
    QStringList args;
    args << "-v5";
    args.append(flags);
    args << positionStr;
    proc.start(exePath, args);
    if (!proc.waitForStarted(3000)) {
        m_proc.store(nullptr);
        emit lineReady("ERROR: Could not start sq1opt. Make sure sq1opt is in the same folder.");
        emit finished(-1);
        return;
    }

    QByteArray buf;
    auto drainLines = [&]() {
        int nl;
        while ((nl = buf.indexOf('\n')) != -1) {
            QString line = QString::fromUtf8(buf.left(nl)).trimmed();
            buf.remove(0, nl + 1);
            if (!line.isEmpty()) emit lineReady(line);
        }
    };

    while (true) {
        bool gotData = proc.waitForReadyRead(200);
        if (gotData) buf += proc.readAll();
        drainLines();
        if (!gotData && proc.state() == QProcess::NotRunning) break;
    }
    buf += proc.readAll();
    drainLines();
    buf = buf.trimmed();
    if (!buf.isEmpty()) emit lineReady(QString::fromUtf8(buf));

    // Null the pointer BEFORE proc is destroyed so requestStop can't fire on a dead object.
    m_proc.store(nullptr);
    proc.waitForFinished(1000);
    emit finished(proc.exitCode());
}

// -------------------------------------------------------
// MainWindow
// -------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
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
    connect(m_sliceTimer, &QTimer::timeout, this, [this]{
        int saves = (m_sliceCount % 2 == 0) ? 2 : 1;
        m_undoStack.append(m_slicePending.first());
        if (saves == 2 && m_slicePending.size() >= 2)
            m_undoStack.append(m_slicePending.last());
        while (m_undoStack.size() > 64) m_undoStack.removeFirst();
        btnUndo->setEnabled(true);
        m_redoStack.clear();
        btnRedo->setEnabled(false);
        m_sliceCount = 0;
        m_slicePending.clear();
    });
}

void MainWindow::buildUI() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QWidget* outerWidget = new QWidget(this);
    QVBoxLayout* outerLayout = new QVBoxLayout(outerWidget);
    outerLayout->setContentsMargins(0,0,0,0);
    outerLayout->setSpacing(0);
    setCentralWidget(outerWidget);

    // ── Top bar ───────────────────────────────────────────────────────────────
    QWidget* topBar = new QWidget();
    topBar->setObjectName("topBar");
    topBar->setFixedHeight(52);
    QHBoxLayout* topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(16, 0, 16, 0);

    QLabel* logoLabel = new QLabel();
    logoLabel->setText("<span style='font-size:17px;font-weight:bold;color:#e0e0e0;letter-spacing:1px;'>SOLVE-A-SQUAN</span>"
                       "<br><span style='font-size:10px;color:#888;'>by Abid and Matt</span>");
    logoLabel->setObjectName("logoLabel");

    QPushButton* btnAbout = new QPushButton("?");
    btnAbout->setObjectName("btnAbout");
    btnAbout->setFixedSize(30, 30);
    btnAbout->setToolTip("About");
    connect(btnAbout, &QPushButton::clicked, this, &MainWindow::showAboutModal);

    topBarLayout->addWidget(logoLabel);
    topBarLayout->addStretch();
    topBarLayout->addWidget(btnAbout);
    outerLayout->addWidget(topBar);

    QWidget* contentWidget = new QWidget();
    outerLayout->addWidget(contentWidget, 1);

    QHBoxLayout* root = new QHBoxLayout(contentWidget);
    root->setSpacing(12);
    root->setContentsMargins(12,12,12,12);

    // ---- LEFT: cube widget + move buttons ----
    QWidget* leftContainer = new QWidget();
    leftContainer->setMinimumWidth(316);
    QVBoxLayout* leftCol = new QVBoxLayout(leftContainer);
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(4);
    cubeWidget = new Sq1Widget(this);
    connect(cubeWidget, &Sq1Widget::positionChanged, this, &MainWindow::updateCommand);
    {
        QHBoxLayout* centerRow = new QHBoxLayout();
        centerRow->setContentsMargins(0,0,0,0);
        centerRow->addStretch();
        centerRow->addWidget(cubeWidget);
        centerRow->addStretch();
        leftCol->addLayout(centerRow);
    }

    // Grid: U' | Slice (rowspan 2) | U
    //        D |                   | D'
    QGridLayout* moveGrid = new QGridLayout();
    moveGrid->setSpacing(4);

    QPushButton* btnUP    = new QPushButton("U'");
    QPushButton* btnU     = new QPushButton("U");
    QPushButton* btnD     = new QPushButton("D");
    QPushButton* btnDP    = new QPushButton("D'");
    QPushButton* btnSlice = new QPushButton();
    btnSlice->setText("Slice\n[I/K]");
    btnSlice->setFixedWidth(70);
    btnSlice->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    moveGrid->addWidget(btnUP,    0, 0);
    moveGrid->addWidget(btnSlice, 0, 1, 2, 1); // rowspan=2
    moveGrid->addWidget(btnU,     0, 2);
    moveGrid->addWidget(btnD,     1, 0);
    moveGrid->addWidget(btnDP,    1, 2);

    moveGrid->setColumnStretch(0, 1);
    moveGrid->setColumnStretch(1, 0);
    moveGrid->setColumnStretch(2, 1);

    leftCol->addLayout(moveGrid);

    // Row 3: Undo | Reset | Redo  (all on same line)
    QHBoxLayout* undoResetRedoRow = new QHBoxLayout();
    undoResetRedoRow->setSpacing(4);
    btnUndo = new QPushButton("⟲");
    btnUndo->setObjectName("btnUndo");
    btnUndo->setEnabled(false);
    btnUndo->setToolTip("Undo  [Z]");
    btnReset = new QPushButton("Reset  [Esc]");
    btnReset->setObjectName("btnReset");
    btnRedo = new QPushButton("⟳");
    btnRedo->setObjectName("btnRedo");
    btnRedo->setEnabled(false);
    btnRedo->setToolTip("Redo  [Y]");
    undoResetRedoRow->addWidget(btnUndo, 1);
    undoResetRedoRow->addWidget(btnRedo, 1);
    undoResetRedoRow->addWidget(btnReset, 1);
    leftCol->addLayout(undoResetRedoRow);

    QGroupBox* grpScramble = new QGroupBox("Scramble / Alg input");
    QVBoxLayout* scrambleLay = new QVBoxLayout(grpScramble);
    scrambleLay->setSpacing(4);
    scrambleLay->setContentsMargins(6,8,6,6);
    QHBoxLayout* scrambleInputRow = new QHBoxLayout();
    scrambleInputRow->setSpacing(0);
    scrambleInputRow->setContentsMargins(0,0,0,0);

    btnScrambleMode = new QPushButton("scram");
    btnScrambleMode->setObjectName("btnScrambleMode");
    btnScrambleMode->setCheckable(true);
    btnScrambleMode->setChecked(false);
    btnScrambleMode->setFixedWidth(48);
    btnScrambleMode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    btnScrambleMode->setToolTip("Toggle between Scramble and Algorithm mode.\n"
                                "Algorithm mode inverts the sequence before applying.");

    txtScramble = new QLineEdit();
    txtScramble->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)");
    txtScramble->setToolTip("Enter a move sequence in (x,y)/ format to be applied on the current cube");
    txtScramble->setObjectName("txtScramble");

    scrambleInputRow->addWidget(btnScrambleMode);
    scrambleInputRow->addWidget(txtScramble);
    scrambleLay->addLayout(scrambleInputRow);

    lblScrambleError = new QLabel("");
    lblScrambleError->setObjectName("lblScrambleError");
    lblScrambleError->setWordWrap(true);
    lblScrambleError->setVisible(false);
    scrambleLay->addWidget(lblScrambleError);

    btnApplyScramble = new QPushButton("Apply");
    btnApplyScramble->setObjectName("btnApplyScramble");
    scrambleLay->addWidget(btnApplyScramble);
    leftCol->addWidget(grpScramble);
    leftCol->addStretch();

    leftScroll = new QScrollArea();
    m_leftPanel = leftScroll;
    leftScroll->setWidget(leftContainer);
    leftScroll->setWidgetResizable(false);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setMinimumWidth(320);
    leftScroll->setMaximumWidth(380);

    connect(btnU,     &QPushButton::clicked, cubeWidget, [this]{ pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_J,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnUP,    &QPushButton::clicked, cubeWidget, [this]{ pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_F,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnSlice, &QPushButton::clicked, cubeWidget, [this]{
        cubeWidget->setFocus();
        m_sliceCount++;

        // Capture snapshot before this slice
        m_slicePending.append({cubeWidget->getPositionString()});
        QKeyEvent e(QEvent::KeyPress, Qt::Key_I, Qt::NoModifier);
        QApplication::sendEvent(cubeWidget, &e);
        m_sliceTimer->start(600);
    });
    connect(btnD,     &QPushButton::clicked, cubeWidget, [this]{ pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_S,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnDP,    &QPushButton::clicked, cubeWidget, [this]{ pushUndoState(); cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_L,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnReset, &QPushButton::clicked, this, [this]{
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
        onReset();
    });
    connect(btnUndo, &QPushButton::clicked, this, [this]{
        if (m_undoStack.isEmpty()) return;
        m_redoStack.append({cubeWidget->getPositionString()});
        if (m_redoStack.size() > 64) m_redoStack.removeFirst();
        btnRedo->setEnabled(true);
        CubeSnapshot snap = m_undoStack.takeLast();
        cubeWidget->setPositionFromString(snap.posStr);
        updateCommand();
        btnUndo->setEnabled(!m_undoStack.isEmpty());
        appendStatusLine("Undone.");
    });
    connect(btnRedo, &QPushButton::clicked, this, [this]{
        if (m_redoStack.isEmpty()) return;
        m_undoStack.append({cubeWidget->getPositionString()});
        if (m_undoStack.size() > 64) m_undoStack.removeFirst();
        btnUndo->setEnabled(true);
        CubeSnapshot snap = m_redoStack.takeLast();
        cubeWidget->setPositionFromString(snap.posStr);
        updateCommand();
        btnRedo->setEnabled(!m_redoStack.isEmpty());
        appendStatusLine("Redone.");
    });
    connect(btnApplyScramble, &QPushButton::clicked, this, &MainWindow::onApplyScramble);
    connect(txtScramble, &QLineEdit::returnPressed, this, &MainWindow::onApplyScramble);
    connect(txtScramble, &QLineEdit::textEdited, this, [this]{
        lblScrambleError->setVisible(false);
        txtScramble->setStyleSheet("");
    });
    connect(btnScrambleMode, &QPushButton::toggled, this, [this](bool checked){
        m_scrambleIsAlg = checked;
        btnScrambleMode->setText(checked ? "Alg" : "Scram");
    });

    root->addWidget(leftScroll);

    // ---- RIGHT: options + output ----
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);

    QGroupBox* grpOptions = new QGroupBox("Options");
    grpOptions->setMinimumHeight(200);

    QWidget* optionsInner = new QWidget();
    QGridLayout* grid = new QGridLayout(optionsInner);
    grid->setVerticalSpacing(2);

    // ── Widgets ──────────────────────────────────────────────────────────────
    chkTwist = new QCheckBox("Twist metric");
    chkTwist->setToolTip("If selected, only slices count as \"moves\", else layer turns count too.");

    chkAllOptimal = new QCheckBox("All optimal");
    chkAllOptimal->setToolTip("Find all the optimal solutions, not just the first one.");

    spnSuboptimal = new QSpinBox();
    spnSuboptimal->setRange(0, 9);
    spnSuboptimal->setValue(0);
    spnSuboptimal->setFixedWidth(48);
    spnSuboptimal->setFixedHeight(26);
    spnSuboptimal->setToolTip("Extra moves beyond optimal to *also* find (0 = optimal only).");

    // Container for the All-optimal row
    QWidget* allOptRow = new QWidget();
    allOptRow->setFixedHeight(28);
    QHBoxLayout* allOptLayout = new QHBoxLayout(allOptRow);
    allOptLayout->setContentsMargins(0, 0, 0, 0);
    allOptLayout->setSpacing(4);
    allOptLayout->addWidget(chkAllOptimal);
    allOptLayout->addStretch(1);
    QLabel* lblSuboptLabel = new QLabel("+suboptimal:");
    lblSuboptLabel->setObjectName("lblSuboptLabel");
    allOptLayout->addWidget(lblSuboptLabel);
    allOptLayout->addWidget(spnSuboptimal);

    chkDepths = new QCheckBox("Specific depths:");
    chkDepths->setToolTip("Search only the listed move depths instead of starting from 0 and going up.\n"
                          "Comma-separated, e.g.\"8,9\"");

    txtDepths = new QLineEdit();
    txtDepths->setFixedWidth(80);
    txtDepths->setFixedHeight(26);
    txtDepths->setPlaceholderText("e.g. 8,9");
    // Only digits and commas allowed; letters are eaten by the global event filter anyway.
    txtDepths->setValidator(new QRegularExpressionValidator(
        QRegularExpression("[0-9,]*"), txtDepths));
    txtDepths->setToolTip("Comma-separated list of depths to search, e.g. \"8,9\"");

    chkGenerator = new QCheckBox("Generator alg");
    chkGenerator->setToolTip("If selected, generated algs will set up to the case from a solved cube,\n"
                            "else the algs will solve the case.");

    chk2gen = new QCheckBox("2Gen  (top layer + slices only)");
    chk2gen->setToolTip("Restrict to 2-gen moves: top-layer turns and slices only.\n"
                        "Requires the bottom left pieces to already be solved.\n"
                        "You cannot demand both 2-gen and stay-in-cubeshape.");

    chkPseudo2gen = new QCheckBox("Pseudo 2Gen  (bottom: ±1 only)");
    chkPseudo2gen->setToolTip("Restrict bottom-layer turns to ±1 only (2-gen with bottom 1 moves).\n");

    chkCubeshape = new QCheckBox("Stay in cubeshape");
    chkCubeshape->setToolTip("Only generate algs that keep the puzzle in cubeshape throughout.");

    chkIgnoreMid = new QCheckBox("Ignore middle layer");
    chkIgnoreMid->setToolTip("Ignore bar states. Equivalent to clicking on the bar until it is gray.");

    chkKarnotation = new QCheckBox("Karnotation output");
    chkKarnotation->setToolTip("Display solutions in karnotation instead of WCA notation.");

    chkSpecificAngle = new QCheckBox("Generate alg from this specific angle");
    chkSpecificAngle->setObjectName("chkSpecificAngle");
    chkSpecificAngle->setToolTip("Generate algs from this angle and this angle only.\n"
                                "Essentially restricting the move before the first slice to 1 moves only.");

    chkMaxX = new QCheckBox("Max top turn:");
    chkMaxX->setToolTip("Limit the maximum top-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"4\", that means algs can do -4 to 4 on top.");
    spnMaxX = new QSpinBox();
    spnMaxX->setRange(0, 6);
    spnMaxX->setValue(3);
    spnMaxX->setFixedWidth(48);
    spnMaxX->setFixedHeight(26);
    spnMaxX->setToolTip("Maximum top-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"4\", that means algs can do -4 to 4 on top.");

    chkMaxY = new QCheckBox("Max bottom turn:");
    chkMaxY->setToolTip("Limit the maximum bottom-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"3\", that means algs can do -3 to 3 on bottom.");
    spnMaxY = new QSpinBox();
    spnMaxY->setRange(0, 6);
    spnMaxY->setValue(3);
    spnMaxY->setFixedWidth(48);
    spnMaxY->setFixedHeight(26);
    spnMaxY->setToolTip("Maximum bottom-layer turn in either direction (0–6).\n"
                        "e.g. if you put \"3\", that means algs can do -3 to 3 on bottom.");

    chkMaxTotal = new QCheckBox("Max total turn:");
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

    chkTwist->setChecked(true);
    chkKarnotation->setChecked(true);

    // ── Grid layout ──────────────────────────────────────────────────────────
    int row = 0;
    grid->addWidget(chkTwist,      row++, 0, 1, 2);
    grid->addWidget(allOptRow,     row++, 0, 1, 2);
    grid->addWidget(chkDepths,     row,   0);
    grid->addWidget(txtDepths,     row++, 1);
    grid->addWidget(chkGenerator,  row++, 0, 1, 2);
    grid->addWidget(chk2gen,       row++, 0, 1, 2);
    grid->addWidget(chkPseudo2gen, row++, 0, 1, 2);
    grid->addWidget(chkCubeshape,  row++, 0, 1, 2);
    grid->addWidget(chkIgnoreMid,  row++, 0, 1, 2);
    grid->addWidget(chkKarnotation,row++, 0, 1, 2);
    grid->addWidget(chkSpecificAngle, row++, 0,1,2);
    grid->addWidget(chkMaxX,       row,   0); grid->addWidget(spnMaxX,    row++, 1);
    grid->addWidget(chkMaxY,       row,   0); grid->addWidget(spnMaxY,    row++, 1);
    grid->addWidget(chkMaxTotal,   row,   0); grid->addWidget(spnMaxTotal,row++, 1);

    // Uniform row height — set after all rows are populated.
    for (int r = 0; r < row; r++) grid->setRowMinimumHeight(r, 28);

    QScrollArea* optionsScroll = new QScrollArea();
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
    auto upd = [this]{ updateConstraints(); updateCommand(); };

    connect(chkTwist,      &QCheckBox::toggled, this, upd);
    connect(chkAllOptimal, &QCheckBox::toggled, this, upd);
    connect(spnSuboptimal, QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkDepths,     &QCheckBox::toggled, this, upd);
    connect(txtDepths,     &QLineEdit::textChanged, this, upd);
    connect(chkGenerator,  &QCheckBox::toggled, this, upd);
    connect(chk2gen,       &QCheckBox::toggled, this, upd);
    connect(chkPseudo2gen, &QCheckBox::toggled, this, upd);
    connect(chkCubeshape,  &QCheckBox::toggled, this, upd);
    connect(chkIgnoreMid,  &QCheckBox::toggled, this, upd);
    connect(chkKarnotation,&QCheckBox::toggled, this, upd);
    connect(chkSpecificAngle,&QCheckBox::toggled,this,upd);
    connect(chkMaxX,       &QCheckBox::toggled, this, upd);
    connect(spnMaxX,       QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxY,       &QCheckBox::toggled, this, upd);
    connect(spnMaxY,       QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxTotal,   &QCheckBox::toggled, this, upd);
    connect(spnMaxTotal,   QOverload<int>::of(&QSpinBox::valueChanged), this, upd);

    // fix #2: propagate tooltip to the whole allOptRow so right-side hover works
    allOptRow->setToolTip(chkAllOptimal->toolTip());

    // ── Pack options/command/solve/progress into one hideable wrapper ─────────
    m_topSection = new QWidget();
    QVBoxLayout* topLay = new QVBoxLayout(m_topSection);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->setSpacing(6);
    topLay->addWidget(grpOptions);

    QWidget* cmdSolveRow = new QWidget();
    QHBoxLayout* cmdSolveLayout = new QHBoxLayout(cmdSolveRow);
    cmdSolveLayout->setContentsMargins(0, 0, 0, 0);
    cmdSolveLayout->setSpacing(0);

    txtCommand = new QLineEdit();
    txtCommand->setReadOnly(false);
    txtCommand->setObjectName("txtCommand");

    btnCopy = new QPushButton("⎘");
    btnCopy->setObjectName("btnCopy");
    btnCopy->setFixedWidth(32);
    btnCopy->setFixedHeight(24);
    btnCopy->setToolTip("Copy command");

    btnSolve = new QPushButton("▶ Solve");
    btnSolve->setObjectName("btnSolve");
    btnSolve->setFixedHeight(24);
    btnSolve->setFixedWidth(90);

    cmdSolveLayout->addWidget(txtCommand, 1);
    cmdSolveLayout->addWidget(btnCopy);
    cmdSolveLayout->addWidget(btnSolve);
    topLay->addWidget(cmdSolveRow);

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
    QWidget* outputWrapper = new QWidget();
    outputWrapper->setMinimumHeight(120);
    QVBoxLayout* outputWrapperLay = new QVBoxLayout(outputWrapper);
    outputWrapperLay->setContentsMargins(0,0,0,0);
    outputWrapperLay->setSpacing(0);

    txtOutput = new QTextEdit(outputWrapper);
    txtOutput->setReadOnly(true);
    txtOutput->setObjectName("txtOutput");
    txtOutput->setMinimumHeight(120);

    // Floating buttons — parented to outputWrapper so they overlay the terminal
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

    // Table view
    m_tableContainer = new QWidget();
    m_tableContainer->setVisible(false);
    m_tableContainer->setMinimumHeight(120);
    QVBoxLayout* tableLay = new QVBoxLayout(m_tableContainer);
    tableLay->setContentsMargins(0,0,0,0);
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
    m_solutionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_solutionTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_solutionTable->setShowGrid(false);
    m_solutionTable->setAlternatingRowColors(false); // we do it manually
    m_solutionTable->setTextElideMode(Qt::ElideNone);
    m_solutionTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_solutionTable, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos){
        int row = m_solutionTable->rowAt(pos.y());
        if (row < 0) return;
        QMenu menu(this);
        QAction* copyRow = menu.addAction("Copy row");
        QAction* copyAlg = menu.addAction("Copy algorithm");
        QAction* chosen = menu.exec(m_solutionTable->viewport()->mapToGlobal(pos));
        if (chosen == copyRow) {
            QStringList parts;
            for (int c = 0; c < m_solutionTable->columnCount(); c++) {
                QTableWidgetItem* it = m_solutionTable->item(row, c);
                if (it) parts << it->text();
            }
            QApplication::clipboard()->setText(parts.join("\t"));
            appendStatusLine("Row copied to clipboard.");
        } else if (chosen == copyAlg) {
            QTableWidgetItem* it = m_solutionTable->item(row, 1);
            if (it) {
                QApplication::clipboard()->setText(it->text());
                appendStatusLine("Algorithm copied to clipboard.");
            }
        }
    });
    tableLay->addWidget(m_solutionTable, 1);

    outputWrapperLay->addWidget(m_tableContainer);
    outputWrapperLay->addWidget(m_tableContainer);
    m_outputWrapper = outputWrapper;
    outputWrapper->installEventFilter(this);
    rightCol->addWidget(outputWrapper, 1);

    // Always visible; enabled only when eligible (cubeshape + solutions present).
    chkRankErgo = new QCheckBox("Roughly rank algs based on relative ergonomics");
    chkRankErgo->setEnabled(false);
    chkRankErgo->setObjectName("chkRankErgo");
    rightCol->addWidget(chkRankErgo);

    lblStatus = new QLabel("");
    lblStatus->setObjectName("lblStatus");
    lblStatus->setVisible(false);

    root->addLayout(rightCol, 1);

    // ── Button connections ────────────────────────────────────────────────────
    connect(btnSolve,        &QPushButton::clicked,  this, &MainWindow::onSolveButtonClicked);
    connect(btnCopy,         &QPushButton::clicked,  this, &MainWindow::onCopy);
    connect(btnExpand,       &QPushButton::clicked,  this, &MainWindow::toggleExpand);
    connect(btnCopyTerminal, &QPushButton::clicked,  this, [this]{
        QApplication::clipboard()->setText(txtOutput->toPlainText());
        appendStatusLine("Terminal copied to clipboard!");
    });
    connect(btnTableMode, &QPushButton::clicked, this, [this]{
        m_tableVisible = !m_tableVisible;
        txtOutput->setVisible(!m_tableVisible);
        m_tableContainer->setVisible(m_tableVisible);
        btnTableMode->setText(m_tableVisible ? "▤" : "⊞");
        btnTableMode->setToolTip(m_tableVisible ? "Switch to terminal view" : "Switch to table view");
        if (m_tableVisible) rebuildTable();
        else if (chkRankErgo->isChecked()) onRankErgoToggled(true);
        else rebuildTerminalView();
    });
    connect(chkRankErgo, &QCheckBox::toggled,    this, &MainWindow::onRankErgoToggled);
    connect(txtCommand, &QLineEdit::textEdited, this, [this](const QString& text){
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

        // Must not look like a flag
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

        // Validate characters before passing to setPositionFromString to prevent crashes.
        // Valid chars: A-H, 1-8, U, V, W, X, Y, Z, and optional trailing - or /
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

        // Safe to call setPositionFromString now
        bool applied = cubeWidget->setPositionFromString(pos);
        if (!applied) {
            showCmdError("Invalid position string — duplicate or unrecognised pieces.");
            return;
        }
        clearCmdError();
        syncFlagsFromCommand(text);
    });

    // Initial floating button positions (will be corrected on first resize)
    QTimer::singleShot(0, this, [this]{
        int w = m_outputWrapper->width();
        int margin = 6; int bw = 22;
        btnExpand->move(w - margin - bw, margin);
        btnTableMode->move(w - margin - bw*2 - 4, margin);
        btnCopyTerminal->move(w - margin - bw*3 - 8, margin);
        btnExpand->raise(); btnTableMode->raise(); btnCopyTerminal->raise();
    });

    updateConstraints();
}

// -------------------------------------------------------
// toggleExpand — expand / shrink the output terminal
// -------------------------------------------------------
void MainWindow::toggleExpand() {
    m_expanded = !m_expanded;
    m_topSection->setVisible(!m_expanded);
    m_leftPanel->setVisible(!m_expanded);
    if (m_expanded) {
        btnExpand->setText("⤡");
        btnExpand->setToolTip("Shrink terminal");
    } else {
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

void MainWindow::rebuildTerminalView() {
    txtOutput->clear();
    int solIdx = 0;
    for (const QString& line : std::as_const(m_rawLines)) {
        bool isSol = line.contains('[') && line.contains(']');
        if (isSol) {
            const char* bg  = (solIdx % 2 == 0) ? "#0d1117" : "#131c28";
            QString color   = m_expanded
                ? (solIdx % 2 == 0 ? "#7abfe8" : "#cbcbcb")
                : "#7abfe8";
            QString fsStyle = m_expanded
                ? "font-size:15px;line-height:1.9;"
                : "";
            QString padding = m_expanded ? "padding:5px 8px;" : "padding:1px 4px;";
            txtOutput->append(QString(
                "<div style='background:%1;color:%2;font-weight:bold;"
                "margin:0;%3%4'>%5</div>")
                .arg(bg, color, fsStyle, padding, line.toHtmlEscaped()));
            solIdx++;
        } else {
            QString fsStyle = m_expanded ? "font-size:13px;line-height:1.6;" : "";
            txtOutput->append(QString("<span style='color:#888;%1'>%2</span>")
                .arg(fsStyle, line.toHtmlEscaped()));
        }
    }
    txtOutput->verticalScrollBar()->setValue(0);
}

// -------------------------------------------------------
// updateConstraints
// -------------------------------------------------------
void MainWindow::updateConstraints() {
    const bool is2gen    = chk2gen->isChecked();
    const bool isPseudo  = chkPseudo2gen->isChecked();
    const bool isAllOpt  = chkAllOptimal->isChecked();
    const bool isDepths  = chkDepths->isChecked();

    // Auto-deselect "Specific depths" when the input field is cleared.
    if (isDepths && txtDepths->text().trimmed().isEmpty()) {
        chkDepths->blockSignals(true);
        chkDepths->setChecked(false);
        chkDepths->blockSignals(false);
    }
    const bool isDepthsNow = chkDepths->isChecked(); // re-read after possible uncheck

    auto disableCheck = [](QCheckBox* cb) {
        cb->setEnabled(false);
        if (cb->isChecked()) {
            cb->blockSignals(true);
            cb->setChecked(false);
            cb->blockSignals(false);
        }
    };

    if (is2gen)   disableCheck(chkCubeshape);
    else          chkCubeshape->setEnabled(true);

    if (chkCubeshape->isChecked()) disableCheck(chk2gen);
    else if (!is2gen)              chk2gen->setEnabled(true);

    if (is2gen)  disableCheck(chkPseudo2gen);
    else         chkPseudo2gen->setEnabled(true);

    if (isPseudo) disableCheck(chk2gen);
    else if (!chkCubeshape->isChecked()) chk2gen->setEnabled(true);

    spnSuboptimal->setVisible(isAllOpt && !isDepthsNow);
    if (QLabel* lbl = findChild<QLabel*>("lblSuboptLabel"))
        lbl->setVisible(isAllOpt && !isDepthsNow);

    // txtDepths is always enabled so the user can click into it and activate the option.
    // Style it to look inactive when the checkbox is off.
    if (isDepthsNow) {
        txtDepths->setStyleSheet("");  // revert to global style
    } else {
        txtDepths->setStyleSheet(
            "QLineEdit { color: #666; background: #1e1e30; border-color: #3a3a4e; }");
    }

    spnMaxX->setEnabled(chkMaxX->isChecked());
    spnMaxY->setEnabled(chkMaxY->isChecked());
    spnMaxTotal->setEnabled(chkMaxTotal->isChecked());

    updateRankErgoState();
}

// -------------------------------------------------------
// buildStyles
// -------------------------------------------------------
void MainWindow::buildStyles() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #1a1a2e; color: #e0e0e0; font-family: 'Segoe UI', Arial; font-size: 13px; }
        QGroupBox { border: 1px solid #444; border-radius: 6px; margin-top: 8px; padding-top: 8px; color: #aaa; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QCheckBox { spacing: 6px; }
        QCheckBox::indicator { width:14px; height:14px; border-radius:3px; border:1px solid #666; background:#2a2a3e; }
        QCheckBox::indicator:checked { background: #4a90d9; border-color: #4a90d9; }
        QCheckBox#chkRankErgo::indicator:checked { background: #d97a4a; border-color: #d97a4a; }
        QCheckBox:disabled { color: #4a4a5a; }
        QCheckBox::indicator:disabled { border-color: #3a3a4e; background: #1e1e30; }
        QLineEdit { background: #2a2a3e; border: 1px solid #555; border-radius: 4px; padding: 3px 6px; color: #fff; }
        QLineEdit:disabled { color: #555; background: #1e1e30; border-color: #3a3a4e; }
        QLineEdit#txtCommand { font-family: monospace; color: #7fdbff; font-size: 12px; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; border-right: none; }
        QSpinBox { background: #2a2a3e; border: 1px solid #555; border-radius: 4px;
                   padding: 2px 8px 2px 4px; color: #fff; }
        QSpinBox:disabled { color: #555; background: #1e1e30; border-color: #3a3a4e; }
        QSpinBox::up-button, QSpinBox::down-button { width: 0; height: 0; border: 0; }
        QSpinBox::up-arrow,  QSpinBox::down-arrow  { width: 0; height: 0; image: none; }
        QTextEdit#txtOutput { background: #0d1117; border: 1px solid #444; border-radius: 4px;
                              font-family: monospace; font-size: 12px; color: #7ec8e3; }
        QPushButton { background: #2a2a3e; border: 1px solid #555; border-radius: 5px; padding: 5px 12px; color: #ddd; }
        QPushButton:hover { background: #3a3a5e; border-color: #777; }
        QPushButton:pressed { background: #1a1a2e; }
        QPushButton#btnSolve { background: #1a6b3c; border-color: #2db570; color: #fff; font-size: 13px; font-weight: bold; }
        QPushButton#btnSolve:hover { background: #227a47; }
        QPushButton#btnSolve:disabled { background: #333; border-color: #444; color: #666; }
        QPushButton#btnCopy { background: #2a2a3e; border: 1px solid #555; border-left: none; border-radius: 4px; border-top-left-radius: 0px; border-bottom-left-radius: 0px; color: #aaa; font-size: 14px; padding: 0; margin-right: 5px;}
        QPushButton#btnCopy:hover { background: #3a3a5e; color: #ddd; }
        QPushButton#btnReset { background: #6b1a1a; border-color: #b52d2d; color: #fdd; }
        QPushButton#btnUndo, QPushButton#btnRedo { background: #2a2a3e; border-color: #555; color: #aaa; }
        QPushButton#btnUndo:disabled, QPushButton#btnRedo:disabled { background: #1e1e2a; border-color: #333; color: #444; }
        QPushButton#btnApplyScramble {
            background: #2a2a3e; border: 1px solid #555;
            border-radius: 4px; color: #ddd; padding: 4px 8px;
        }
        QPushButton#btnApplyScramble:hover { background: #3a3a5e; border-color: #777; }
        QPushButton#btnScrambleMode {
            background: #2a2a3e; border: 1px solid #555;
            border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px;
            border-right: none; color: #aaa; padding: 0 6px; font-size: 11px;
        }
        QPushButton#btnScrambleMode:checked { color: #fff; background: #333350; }
        QPushButton#btnScrambleMode:hover { background: #3a3a5e; }
        QLineEdit#txtScramble {
            border-top-left-radius: 0; border-bottom-left-radius: 0;
        }
        QPushButton#btnExpand, QPushButton#btnCopyTerminal {
            background: #1e1e30; border: 1px solid #3a3a5e; border-radius: 4px;
            color: #7a7aaa; font-size: 13px; padding: 0;
        }
        QPushButton#btnExpand:hover, QPushButton#btnCopyTerminal:hover { background: #2a2a4a; border-color: #5a5a8a; color: #b0b0dd; }
        QProgressBar { border: none; background: #2a2a3e; border-radius: 3px; }
        QProgressBar::chunk { background: #4a90d9; border-radius: 3px; }
        QLabel#lblStatus { color: #888; font-size: 11px; }
        QLabel#lblScrambleError { color: #ff5555; font-size: 11px; padding: 2px 2px; }
        QLabel#lblCommandError  { color: #ff5555; font-size: 11px; padding: 2px 2px; }
        QScrollBar:vertical { background: #0d1117; width: 12px; border-radius: 6px; margin: 0; }
        QScrollBar::handle:vertical { background: #4a4a6e; border-radius: 6px; min-height: 24px; }
        QScrollBar::handle:vertical:hover { background: #6a6aae; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; border: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        QTableWidget#m_solutionTable {
            background: #0d1117; border: 1px solid #444;
            border-radius: 4px; gridline-color: #0d1117;
            font-family: monospace; font-size: 12px;
        }
        QTableWidget#m_solutionTable QHeaderView::section {
            background: #1a1a2e; color: #7a9ab8; border: none;
            border-bottom: 1px solid #2a2a4a; padding: 4px;
            font-size: 11px; font-weight: bold;
        }
        QTableWidget#m_solutionTable::item:selected {
            background: #1e3a5a; color: #ffffff;
        }
        QPushButton#btnTableMode {
            background: #1e1e30; border: 1px solid #3a3a5e; border-radius: 4px;
            color: #7a7aaa; font-size: 13px; padding: 0;
        }
        QPushButton#btnTableMode:hover { background: #2a2a4a; border-color: #5a5a8a; color: #b0b0dd; }
        QWidget#topBar {
            background: #0e0e1c;
            border-bottom: 2px solid #2a2a4a;
            min-height: 52px;
            max-height: 52px;
        }
        QLabel#logoLabel {
            background: transparent;
            padding: 0;
        }
        QPushButton#btnAbout {
            background: #2a2a3e;
            border: 1px solid #4a4a6a;
            border-radius: 15px;
            color: #9090bb;
            font-size: 15px;
            font-weight: bold;
            padding: 0;
            min-width: 30px;
            max-width: 30px;
            min-height: 30px;
            max-height: 30px;
        }
        QPushButton#btnAbout:hover {
            background: #3a3a5e;
            border-color: #7a7aaa;
            color: #e0e0ff;
        }
        QToolTip {
            background: #252540;
            color: #d8d8f0;
            border: 1px solid #5a5a8a;
            border-radius: 5px;
            padding: 6px 10px;
            font-size: 12px;
            opacity: 230;
        }
    )");
}

// -------------------------------------------------------
// buildArgList
// -------------------------------------------------------
QStringList MainWindow::buildArgList() {
    QStringList args;

    if (chkTwist->isChecked()) args << "-w";

    if (chkAllOptimal->isChecked()) {
        const bool useNumber = !chkDepths->isChecked() && spnSuboptimal->value() > 0;
        args << (useNumber ? QString("-a%1").arg(spnSuboptimal->value()) : QString("-a"));
    }

    if (chkDepths->isChecked()) {
        QString dv = txtDepths->text().trimmed().remove(' ');
        if (!dv.isEmpty()) args << QString("-d%1").arg(dv);
    }

    if (chkGenerator->isChecked())  args << "-g";
    if (chk2gen->isChecked())       args << "-2";
    if (chkPseudo2gen->isChecked()) args << "-p";
    if (chkCubeshape->isChecked())  args << "-c";
    if (chkIgnoreMid->isChecked())  args << "-m";
    if (chkKarnotation->isChecked())args << "-k";
    if (chkSpecificAngle->isChecked())args<<"-n";

    if (chkMaxX->isChecked())     args << QString("-X%1").arg(spnMaxX->value());
    if (chkMaxY->isChecked())     args << QString("-Y%1").arg(spnMaxY->value());
    if (chkMaxTotal->isChecked()) args << QString("-Z%1").arg(spnMaxTotal->value());

    return args;
}

void MainWindow::syncFlagsFromCommand(const QString& text) {
    QStringList parts = text.trimmed().split(' ', Qt::SkipEmptyParts);
    // Remove the executable name and position string (first and last tokens)
    if (parts.size() >= 1 && parts[0] == "sq1opt") parts.removeFirst();
    if (!parts.isEmpty() && !parts.last().startsWith('-')) parts.removeLast();

    auto has = [&](const QString& flag) {
        return parts.contains(flag);
    };
    auto hasPrefix = [&](const QString& prefix) -> QString {
        for (const QString& p : std::as_const(parts))
            if (p.startsWith(prefix) && p.length() > prefix.length())
                return p.mid(prefix.length());
        return QString();
    };

    // Block all signals while we sync so updateCommand isn't re-triggered
    auto block = [](QObject* o, bool b){ o->blockSignals(b); };

    block(chkTwist,        true); chkTwist->setChecked(has("-w"));        block(chkTwist,        false);
    block(chkGenerator,    true); chkGenerator->setChecked(has("-g"));    block(chkGenerator,    false);
    block(chk2gen,         true); chk2gen->setChecked(has("-2"));         block(chk2gen,         false);
    block(chkPseudo2gen,   true); chkPseudo2gen->setChecked(has("-p"));   block(chkPseudo2gen,   false);
    block(chkCubeshape,    true); chkCubeshape->setChecked(has("-c"));    block(chkCubeshape,    false);
    block(chkIgnoreMid,    true); chkIgnoreMid->setChecked(has("-m"));    block(chkIgnoreMid,    false);
    block(chkKarnotation,  true); chkKarnotation->setChecked(has("-k"));  block(chkKarnotation,  false);
    block(chkSpecificAngle,true); chkSpecificAngle->setChecked(has("-n"));block(chkSpecificAngle,false);

    // -a / -a<n>
    bool hasA = false;
    int subopt = 0;
    for (const QString& p : std::as_const(parts)) {
        if (p == "-a") { hasA = true; subopt = 0; break; }
        if (p.startsWith("-a") && p.length() > 2) {
            bool ok; int v = p.sliced(2).toInt(&ok);
            if (ok) { hasA = true; subopt = v; break; }
        }
    }
    block(chkAllOptimal,  true); chkAllOptimal->setChecked(hasA);  block(chkAllOptimal,  false);
    block(spnSuboptimal,  true); spnSuboptimal->setValue(subopt);  block(spnSuboptimal,  false);

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
    block(chkMaxX,    true); block(spnMaxX,    true);
    block(chkMaxY,    true); block(spnMaxY,    true);
    block(chkMaxTotal,true); block(spnMaxTotal,true);
    chkMaxX->setChecked(!xv.isEmpty());
    if (!xv.isEmpty()) { bool ok; int v = xv.toInt(&ok); if (ok) spnMaxX->setValue(v); }
    chkMaxY->setChecked(!yv.isEmpty());
    if (!yv.isEmpty()) { bool ok; int v = yv.toInt(&ok); if (ok) spnMaxY->setValue(v); }
    chkMaxTotal->setChecked(!zv.isEmpty());
    if (!zv.isEmpty()) { bool ok; int v = zv.toInt(&ok); if (ok) spnMaxTotal->setValue(v); }
    block(chkMaxX,    false); block(spnMaxX,    false);
    block(chkMaxY,    false); block(spnMaxY,    false);
    block(chkMaxTotal,false); block(spnMaxTotal,false);

    updateConstraints();
}

void MainWindow::updateCommand() {
    QString pos = cubeWidget->getPositionString();
    QStringList args = buildArgList();
    txtCommand->setText("sq1opt " + args.join(" ") + " " + pos);
    lblCommandError->setVisible(false);
    txtCommand->setStyleSheet("QLineEdit#txtCommand { font-family: monospace; color: #7fdbff; font-size: 12px; border-right: none; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; }");
}

// -------------------------------------------------------
// onSolveButtonClicked — single entry-point for the Solve/Stop button
// -------------------------------------------------------
void MainWindow::onSolveButtonClicked() {
    if (worker && worker->isRunning())
        stopSolver();
    else
        onSolve();
}

// -------------------------------------------------------
// onSolve
// -------------------------------------------------------
void MainWindow::onSolve() {
    if (worker && worker->isRunning()) return;

    m_stopped = false;
    txtOutput->clear();
    m_rawLines.clear();
    m_solutionLines.clear();
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

    appendStatusLine("Solving…");

    // Swap Solve → Stop appearance (muted dark red, not alarming).
    btnSolve->setText("■  Stop");
    btnSolve->setStyleSheet(
        "QPushButton#btnSolve {"
        "  background: #3d1616; border: 1px solid #7a2e2e; padding-top: 0px; padding-bottom: 0px;"
        "  color: #c89898; font-size: 12px; font-weight: bold; }"
        "QPushButton#btnSolve:hover { background: #4d1e1e; }");

    progressBar->setVisible(true);

    worker = new SolverWorker();
    worker->positionStr = cubeWidget->getPositionString();
    m_posHex = worker->positionStr;
    worker->flags = buildArgList();
    m_cubeshapeWasActive = chkCubeshape->isChecked();
    connect(worker, &SolverWorker::lineReady, this, &MainWindow::onSolverLine, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished,  this, &MainWindow::onSolverDone, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished,  worker, &QObject::deleteLater);
    m_solveStartMs = QDateTime::currentMSecsSinceEpoch();
    m_firstSolutionMs = 0;
    m_hadFirstSolution = false;
    worker->start();
}

// -------------------------------------------------------
// stopSolver — kill the running process and flag m_stopped
// -------------------------------------------------------
void MainWindow::stopSolver() {
    m_stopped = true;
    if (worker && worker->isRunning())
        worker->requestStop();
}

// -------------------------------------------------------
// onSolverLine
// -------------------------------------------------------
void MainWindow::onSolverLine(QString line) {
    bool isSolution = line.contains('[') && line.contains(']');
    if (isSolution) {
        int bracketPos = line.indexOf('[');
        QString algKey = (bracketPos >= 0) ? line.left(bracketPos).trimmed() : line.trimmed();
        if (m_seenSolutions.contains(algKey)) return;
        m_seenSolutions.insert(algKey);
    }
    m_rawLines.append(line);
    if (isSolution) {
        m_solutionLines.append(line);
        if (!m_hadFirstSolution) {
            m_hadFirstSolution = true;
            m_firstSolutionMs = QDateTime::currentMSecsSinceEpoch();
        }
        btnExpand->setVisible(true);
        btnCopyTerminal->setVisible(true);
        btnTableMode->setVisible(true);
        // Alternate row backgrounds: near-black vs subtle dark-blue, text always light blue
        const char* bg = (m_solutionLines.size() % 2 == 1) ? "#0d1117" : "#131c28";
        QString solColor = m_expanded
            ? (m_solutionLines.size() % 2 == 1 ? "#7abfe8" : "#cbcbcb")
            : "#7abfe8";
        QString solFsStyle = m_expanded ? "font-size:15px;line-height:1.9;" : "";
        QString solPadding = m_expanded ? "padding:5px 8px;" : "padding:1px 4px;";
        txtOutput->append(QString(
            "<div style='background:%1;color:%2;font-weight:bold;"
            "margin:0;%3%4'>%5</div>")
            .arg(bg, solColor, solFsStyle, solPadding, line.toHtmlEscaped()));
        // Enable rank button as soon as the first solution arrives — even mid-solve —
        // so stopping early still allows ranking whatever was found.
        updateRankErgoState();
    } else {
        QString nonsolFs = m_expanded ? "font-size:13px;line-height:1.6;" : "";
        txtOutput->append(QString("<span style='color:#888;%1'>%2</span>")
            .arg(nonsolFs, line.toHtmlEscaped()));
    }
}

// -------------------------------------------------------
// onSolverDone
// -------------------------------------------------------
void MainWindow::onSolverDone(int code) {
    progressBar->setVisible(false);

    // Restore Solve button appearance.
    btnSolve->setText("▶  Solve");
    btnSolve->setStyleSheet(""); // revert to stylesheet-defined look

    const int n = m_solutionLines.size();
    double secs = (QDateTime::currentMSecsSinceEpoch() - m_solveStartMs) / 1000.0;
    QString secsStr = QString::number(secs, 'f', 2);

    if (m_stopped) {
        QString summary = QString("Stopped — %1 solution%2 found in %3s.")
                              .arg(n).arg(n == 1 ? "" : "s").arg(secsStr);
        appendStatusLine(summary);
    } else if (code == 0) {
        QString summary = QString("Done — %1 solution%2 found in %3s.")
                              .arg(n).arg(n == 1 ? "" : "s").arg(secsStr);
        appendStatusLine(summary);
    } else {
        appendStatusLine("Error (code " + QString::number(code) + ")");
    }

    updateRankErgoState();
    if (!m_solutionLines.isEmpty()) {
        qint64 elapsed = m_hadFirstSolution
            ? (QDateTime::currentMSecsSinceEpoch() - m_firstSolutionMs)
            : 3000;
        int delay = (elapsed < 3000) ? 400 : 0;
        QTimer::singleShot(delay, this, [this]{
            m_tableVisible = true;
            txtOutput->setVisible(false);
            m_tableContainer->setVisible(true);
            btnTableMode->setText("▤");
            btnTableMode->setToolTip("Switch to terminal view");
            rebuildTable();
        });
    }
}

// -------------------------------------------------------
// pushUndoState
// -------------------------------------------------------
void MainWindow::pushUndoState() {
    m_undoStack.append({cubeWidget->getPositionString()});
    if (m_undoStack.size() > 64) m_undoStack.removeFirst();
    btnUndo->setEnabled(true);
    m_redoStack.clear();
    btnRedo->setEnabled(false);
}

// -------------------------------------------------------
// onReset
// -------------------------------------------------------
void MainWindow::onReset() {
    txtOutput->clear();
    m_rawLines.clear();
    m_solutionLines.clear();
    m_seenSolutions.clear();
    chkRankErgo->blockSignals(true);
    chkRankErgo->setChecked(false);
    chkRankErgo->blockSignals(false);
    appendStatusLine("Reset.");
    btnExpand->setVisible(false);
    btnCopyTerminal->setVisible(false);
    btnTableMode->setVisible(false);
    m_tableVisible = false;
    txtOutput->setVisible(true);
    m_tableContainer->setVisible(false);
    if (m_expanded) toggleExpand();
    updateRankErgoState();
    updateCommand();
}

// -------------------------------------------------------
// keyPressEvent — the global eventFilter handles all routing;
// this is kept only as a fallback for events that slip through.
// -------------------------------------------------------
void MainWindow::onApplyScramble() {
    QString raw = txtScramble->text().trimmed();

    // Empty input = reset to solved
    if (raw.isEmpty()) {
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
        for (int i = 0; i < 16 && j < 24; i++) {
            int k = (unsigned char)s[i];
            if (k >= 'a' && k <= 'z') k += ('A' - 'a');
            if      (k >= 'A' && k <= 'H') k -= 'A';
            else if (k >= '1' && k <= '8') k -= ('1' - 8);
            else if (k == 'U') { k = nextPartialCorner; nextPartialCorner -= 3; }
            else if (k == 'V') { k = nextPartialCorner; nextPartialCorner -= 3; }
            else if (k == 'W') { k = nextPartialCorner; nextPartialCorner -= 3; }
            else if (k == 'X') { k = nextPartialEdge;   nextPartialEdge += 3; }
            else if (k == 'Y') { k = nextPartialEdge;   nextPartialEdge += 3; }
            else if (k == 'Z') { k = nextPartialEdge;   nextPartialEdge += 3; }
            pos[j++] = k;
            if (k >= 0 && k < 8) pos[j++] = k; // corner occupies two slots
        }
        if (s.size() >= 17)
            mid = (s[16] == '/') ? 1 : 0;
        else if (s.size() == 16)
            mid = (s.back() == '/') ? 1 : 0;
        // for position strings without trailing char, mid stays 0
        if (s.size() < 16) {
            // fallback: check last char of the 15/16 char string
            char last = s.back();
            mid = (last == '/') ? 1 : 0;
        }
    }

    auto doTop = [&](int m) {
        m = ((m % 12) + 12) % 12;
        for (int moves = 0; moves < m; moves++) {
            int c = pos[11];
            for (int i = 11; i > 0; i--) pos[i] = pos[i-1];
            pos[0] = c;
        }
    };
    auto doBot = [&](int m) {
        m = ((m % 12) + 12) % 12;
        for (int moves = 0; moves < m; moves++) {
            int c = pos[23];
            for (int i = 23; i > 12; i--) pos[i] = pos[i-1];
            pos[12] = c;
        }
    };
    auto isTwistable = [&]() {
        return pos[0]!=pos[11] && pos[5]!=pos[6] &&
               pos[12]!=pos[23] && pos[17]!=pos[18];
    };
    auto doSlice = [&]() {
        if (!isTwistable()) return;
        for (int i = 6; i < 12; i++) std::swap(pos[i], pos[i+6]);
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

    struct Move { bool isSlice; int x, y; };
    QVector<Move> moves;

    QString s = raw;
    int idx = 0;
    bool ok = true;

    // If the sequence starts with '/' it means a leading slice before any turn.
    // We'll handle this by treating the string as a sequence of:
    //   [optional leading /] (turn /) * [optional trailing turn]
    // We do a character-level parse.

    while (idx < s.size() && ok) {
        // Skip whitespace
        while (idx < s.size() && s[idx].isSpace()) idx++;
        if (idx >= s.size()) break;

        if (s[idx] == '/') {
            // Slash = slice
            moves.append({true, 0, 0});
            idx++;
        } else if (s[idx] == '(' || s[idx].isDigit() || s[idx] == '-') {
            // Turn: optional '(' x ',' y optional ')'
            if (s[idx] == '(') idx++;
            // parse x
            while (idx < s.size() && s[idx].isSpace()) idx++;
            int sign = 1;
            if (idx < s.size() && s[idx] == '-') { sign = -1; idx++; }
            int num = 0; bool hasDigit = false;
            while (idx < s.size() && s[idx].isDigit()) { num = num*10+(s[idx].toLatin1()-'0'); idx++; hasDigit = true; }
            if (!hasDigit) { ok = false; break; }
            int x = sign * num;
            while (idx < s.size() && s[idx].isSpace()) idx++;
            if (idx >= s.size() || s[idx] != ',') { ok = false; break; }
            idx++; // skip ','
            // parse y
            while (idx < s.size() && s[idx].isSpace()) idx++;
            sign = 1;
            if (idx < s.size() && s[idx] == '-') { sign = -1; idx++; }
            num = 0; hasDigit = false;
            while (idx < s.size() && s[idx].isDigit()) { num = num*10+(s[idx].toLatin1()-'0'); idx++; hasDigit = true; }
            if (!hasDigit) { ok = false; break; }
            int y = sign * num;
            while (idx < s.size() && s[idx].isSpace()) idx++;
            if (idx < s.size() && s[idx] == ')') idx++;
            moves.append({false, x, y});
        } else {
            ok = false; break;
        }
    }

    if (!ok) {
        int approxPos = idx;
        QString ctx = raw.mid(qMax(0, approxPos-6), 12).trimmed();
        QString msg = QString("Parse error near \"%1\" (col %2) — expected (x,y) or /.")
                          .arg(ctx).arg(approxPos);
        lblScrambleError->setText(msg);
        lblScrambleError->setVisible(true);
        txtScramble->setStyleSheet("QLineEdit { border-color: #ff5555; }");
        return;
    }

    // ── Optionally invert if "Input algorithm" mode ───────────────────────────
    if (m_scrambleIsAlg) {
        // Invert: reverse the move list and negate all turns
        QVector<Move> inv;
        for (int i = moves.size()-1; i >= 0; i--) {
            Move mv = moves[i];
            if (!mv.isSlice) { mv.x = -mv.x; mv.y = -mv.y; }
            inv.append(mv);
        }
        moves = inv;
    }

    // ── Apply moves ───────────────────────────────────────────────────────────
    for (const Move& mv : std::as_const(moves)) {
        if (mv.isSlice) doSlice();
        else { doTop(mv.x); doBot(mv.y); }
    }

    // Build position string
    const QString pieceChars = "ABCDEFGH12345678";
    QString posStr;
    for (int i = 0; i < 24; i++) {
        posStr += pieceChars[pos[i]];
        if (pos[i] < 8) i++;
    }
    posStr += (mid == 0 ? '-' : '/');

    bool applied = cubeWidget->setPositionFromString(posStr);
    if (!applied) {
        lblScrambleError->setText("Resulting position is invalid — check your move sequence.");
        lblScrambleError->setVisible(true);
        txtScramble->setStyleSheet("QLineEdit { border-color: #ff5555; }");
        return;
    }
    // Clear any previous error
    lblScrambleError->setVisible(false);
    txtScramble->setStyleSheet("");
    updateCommand();
    appendStatusLine(m_scrambleIsAlg ? "Algorithm applied (inverted)." : "Scramble applied.");
}

void MainWindow::appendStatusLine(const QString& msg) {
    txtOutput->append(QString("<div style='color:#6a9ab8;font-size:11px;padding:1px 4px;font-style:italic;'>%1</div>")
        .arg(msg.toHtmlEscaped()));
}

void MainWindow::rebuildTable() {
    const bool ergo = chkRankErgo->isChecked();
    const bool showErgo = m_cubeshapeWasActive;  // captured at solve time, not current checkbox
    m_solutionTable->setColumnCount(showErgo ? 5 : 4);
    m_solutionTable->setHorizontalHeaderLabels(
        showErgo ? QStringList{"#", "Solution", "Moves", "Slices", "Ergo"}
                 : QStringList{"#", "Solution", "Moves", "Slices"});
    m_solutionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_solutionTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    if (showErgo)
        m_solutionTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_solutionTable->setRowCount(0);
    if (m_solutionLines.isEmpty()) return;

    // Parse move count and slice count from bracket annotation e.g. "[7|14]"
    auto parseCounts = [](const QString& line, int& moves, int& slices){
        moves = 0; slices = 0;
        int lb = line.lastIndexOf('[');
        int rb = line.lastIndexOf(']');
        if (lb < 0 || rb < 0) return;
        QString bracket = line.mid(lb+1, rb-lb-1); // e.g. "7|14"
        QStringList parts = bracket.split('|');
        if (parts.size() >= 2) {
            slices = parts[0].trimmed().toInt();
            moves  = parts[1].trimmed().toInt();
        }
    };

    // Strip bracket annotation from display
    auto stripBracket = [](const QString& line) -> QString {
        int lb = line.lastIndexOf('[');
        return lb > 0 ? line.left(lb).trimmed() : line.trimmed();
    };

    struct Row { QString alg; int moves; int slices; double ergo; };
    QVector<Row> rows;

    bool useKarn = chkKarnotation->isChecked(); // showErgo already declared above
    auto rated = rateAndSort(m_solutionLines, m_posHex, useKarn);
    // Build a map from stripped alg -> ergo score
    QMap<QString, double> ergoMap;
    for (auto& [line, score] : rated)
        ergoMap[stripBracket(line)] = score;

    for (const QString& line : std::as_const(m_solutionLines)) {
        int mv, sl;
        parseCounts(line, mv, sl);
        QString alg = stripBracket(line);
        double eg = ergoMap.value(alg, 0.0);
        rows.append({alg, mv, sl, eg});
    }

    if (ergo && showErgo) {
        std::stable_sort(rows.begin(), rows.end(),
            [](const Row& a, const Row& b){ return a.ergo > b.ergo; });
    } else {
        std::stable_sort(rows.begin(), rows.end(),
            [](const Row& a, const Row& b){
                if (a.slices != b.slices) return a.slices < b.slices;
                return a.moves < b.moves;
            });
    }

    const QColor rowA(13, 17, 23);
    const QColor rowB(19, 28, 40);
    const QColor textCol(122, 191, 232);
    const QColor textColAlt = m_expanded ? QColor(203, 203, 203) : textCol;
    const QColor metaCol(154, 172, 190);
    const QColor metaColAlt = m_expanded ? QColor(150, 150, 150, 238) : metaCol;
    const int rowH = m_expanded ? 36 : 24;
    const int fontSize = m_expanded ? 15 : 12;

    m_solutionTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); i++) {
        const Row& r = rows[i];
        QColor bg = (i % 2 == 0) ? rowA : rowB;

        bool isAltRow = (i % 2 == 1);
        auto cell = [&](int col, const QString& txt, bool isMeta = false) {
            QTableWidgetItem* item = new QTableWidgetItem(txt);
            item->setBackground(bg);
            if (m_expanded) {
                QColor c = isMeta ? (isAltRow ? metaColAlt : metaCol)
                                  : (isAltRow ? textColAlt : textCol);
                item->setForeground(c);
                QFont f = item->font();
                f.setPointSize(fontSize);
                item->setFont(f);
            } else {
                item->setForeground(isMeta ? metaCol : textCol);
            }
            item->setTextAlignment(Qt::AlignCenter);
            m_solutionTable->setItem(i, col, item);
        };

        cell(0, QString::number(i+1), true);
        // Solution column: left-aligned
        QTableWidgetItem* algItem = new QTableWidgetItem(r.alg);
        algItem->setBackground(bg);
        algItem->setForeground(m_expanded && isAltRow ? textColAlt : textCol);
        algItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        if (m_expanded) {
            QFont f = algItem->font();
            f.setPointSize(fontSize);
            algItem->setFont(f);
        }
        m_solutionTable->setItem(i, 1, algItem);
        cell(2, QString::number(r.moves), true);
        cell(3, QString::number(r.slices), true);
        if (showErgo)
            cell(4, QString::number(r.ergo, 'f', 1), true);
        m_solutionTable->setRowHeight(i, rowH);
    }
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    int w = event->size().width();
    // Scale left panel: 320px base, grow a little above 1000px, cap at 400px
    int leftW = qBound(320, 320 + (w - 860) / 6, 400);
    leftScroll->setFixedWidth(leftW);
    if (auto* lc = leftScroll->widget())
        lc->setFixedWidth(leftW - 4);
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    QMainWindow::keyPressEvent(event);
}

// -------------------------------------------------------
// onCopy
// -------------------------------------------------------
void MainWindow::onCopy() {
    QApplication::clipboard()->setText(txtCommand->text());
    appendStatusLine("Copied to clipboard!");
}

// -------------------------------------------------------
// onRankErgoToggled
// -------------------------------------------------------
void MainWindow::onRankErgoToggled(bool checked) {
    if (!checked) {
        rebuildTerminalView();
        appendStatusLine("Done.");
        if (m_tableVisible) rebuildTable();
        return;
    }
    if (m_solutionLines.isEmpty()) return;
    lblStatus->setText("Rating algorithms…");

    auto rated = rateAndSort(m_solutionLines, m_posHex, true); // always use karn for rating

    txtOutput->clear();
    // First emit non-solution lines (status/info lines)
    for (const QString& line : std::as_const(m_rawLines)) {
        bool isSol = line.contains('[') && line.contains(']');
        if (!isSol) {
            QString fsStyle = m_expanded ? "font-size:13px;line-height:1.6;" : "";
            txtOutput->append(QString("<span style='color:#888;%1'>%2</span>")
                .arg(fsStyle, line.toHtmlEscaped()));
        }
    }

    int solIdx = 0;
    for (auto& [line, score] : rated) {
        const char* bg  = (solIdx % 2 == 0) ? "#0d1117" : "#131c28";
        bool isAltRow   = (solIdx % 2 == 1);
        QString color   = m_expanded ? (isAltRow ? "#cdcdcd" : "#7abfe8") : "#7abfe8";
        QString fsStyle = m_expanded ? "font-size:15px;line-height:1.9;" : "";
        QString padding = m_expanded ? "padding:5px 8px;" : "padding:1px 4px;";
        QString display = QString("%1  (%2)").arg(line).arg(score, 0, 'f', 2);
        txtOutput->append(QString(
            "<div style='background:%1;color:%2;font-weight:bold;"
            "margin:0;%3%4'>%5</div>")
            .arg(bg, color, fsStyle, padding, display.toHtmlEscaped()));
        solIdx++;
    }
    appendStatusLine(QString("Ranked %1 algs by ergonomics.").arg((int)rated.size()));
    txtOutput->verticalScrollBar()->setValue(0);
    if (m_tableVisible) rebuildTable();
}

// -------------------------------------------------------
// updateRankErgoState
// Decides whether chkRankErgo should be enabled, and sets
// a context-sensitive tooltip explaining why it's grayed out.
// -------------------------------------------------------
void MainWindow::updateRankErgoState() {
    const bool cubeshapeActive = chkCubeshape->isChecked();
    const bool hasSolutions    = !m_solutionLines.isEmpty();
    const bool solving         = worker && worker->isRunning();
const bool canRank         = cubeshapeActive && hasSolutions && !solving;

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
    if (!canRank && chkRankErgo->isChecked()) {
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
void MainWindow::showAboutModal() {
    QWidget* central = this->centralWidget();

    QWidget* overlay = new QWidget(central);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background:rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    QWidget* card = new QWidget(overlay);
    card->setObjectName("aboutCard");
    card->setFixedWidth(480);
    card->setStyleSheet(
        "QWidget#aboutCard {"
        "  background:#1e1e34;"
        "  border:1px solid #3a3a5e;"
        "  border-radius:10px;"
        "}"
    );

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(10);

    QLabel* title = new QLabel("About Solve-A-Squan");
    title->setStyleSheet("font-size:16px;font-weight:bold;color:#e0e0e0;background:transparent;");

    QLabel* body = new QLabel();
    body->setWordWrap(true);
    body->setOpenExternalLinks(true);
    body->setTextFormat(Qt::RichText);
    body->setStyleSheet("background:transparent;");
    body->setText(
        "<span style='color:#b0b0c8;font-size:12px;line-height:1.7;'>"
        "This program stemmed from the optimal Square-1 solver by "
        "<a href='https://www.jaapsch.net/puzzles/' style='color:#7abfe8;'>Jaap Scherphuis</a>."
        "<br><br>"
        "v2 was created by Michael Gottlieb "
        "(<a href='https://github.com/qqwref' style='color:#7abfe8;'>GitHub</a>, "
        "<a href='https://www.worldcubeassociation.org/persons/2006GOTT01' style='color:#7abfe8;'>WCA</a>), "
        "who rewrote the solver with significant improvements and optimisations."
        "<br><br>"
        "This is the official <b style='color:#e0e0e0;'>v3</b>. New in v3:"
        "<ul style='margin:4px 0 4px 16px;padding:0;color:#b0b0c8;'>"
        "<li>Actual graphical UI</li>"
        "<li>Ability to generate a solution from a specific angle</li>"
        "<li>Improved karnotation support</li>"
        "<li>Algorithm ergonomics rater</li>"
        "</ul>"
        "v3 is created by "
        "<a href='https://www.worldcubeassociation.org/persons/2024ASHR02' style='color:#7abfe8;'>Abid Ibn Ashraf</a>"
        " and "
        "<a href='https://www.worldcubeassociation.org/persons/2023MAOS01' style='color:#7abfe8;'>Matt Mao</a>."
        "</span>"
    );

    lay->addWidget(title);
    lay->addWidget(body);

    card->show();
    card->adjustSize();

    auto centerCard = [overlay, card]() {
        overlay->setGeometry(overlay->parentWidget()->rect());
        int cx = (overlay->width()  - card->width())  / 2;
        int cy = (overlay->height() - card->height()) / 2;
        card->move(cx, cy);
    };
    centerCard();
    card->raise();

    // Watch the centralWidget for resize events
    struct Filter : public QObject {
        QWidget* overlay; QWidget* card;
        std::function<void()> center;
        Filter(QWidget* o, QWidget* c, std::function<void()> fn)
            : QObject(o), overlay(o), card(c), center(fn) {}
        bool eventFilter(QObject* watched, QEvent* e) override {
            if (e->type() == QEvent::Resize && watched == overlay->parentWidget()) {
                center();
                return false;
            }
            if (e->type() == QEvent::MouseButtonPress && watched == overlay) {
                QMouseEvent* me = static_cast<QMouseEvent*>(e);
                if (!card->geometry().contains(me->pos())) {
                    overlay->deleteLater();
                    return true;
                }
            }
            return false;
        }
    };

    Filter* f = new Filter(overlay, card, centerCard);
    central->installEventFilter(f);   // watches parent for resize
    overlay->installEventFilter(f);   // watches overlay for click-outside
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    // ── (0) Per-line tooltips for the output box ──────────────────────────────
    // QTextEdit delivers QHelpEvent to its internal viewport, not to itself.
    if (event->type() == QEvent::ToolTip && txtOutput
            && watched == txtOutput->viewport())
        return true;

    if (event->type() == QEvent::Resize && watched == m_outputWrapper) {
        int w = m_outputWrapper->width();
        int margin = 6;
        int bw = 22;
        btnExpand->move(w - margin - bw, margin);
        btnTableMode->move(w - margin - bw*2 - 4, margin);
        btnCopyTerminal->move(w - margin - bw*3 - 8, margin);
        return false;
    }

    if (event->type() != QEvent::KeyPress)
        return QMainWindow::eventFilter(watched, event);

    QKeyEvent* ke = static_cast<QKeyEvent*>(event);

    // ── (1) Ctrl+C stops the solver ──────────────────────────────────────────
    if (ke->key() == Qt::Key_C && (ke->modifiers() & Qt::ControlModifier)) {
        if (worker && worker->isRunning()) {
            stopSolver();
            return true; // consume — don't copy
        }
        return QMainWindow::eventFilter(watched, event);
    }

    // ── Events already targeting cubeWidget: let cubeWidget handle them. ─────
    // (sendEvent below will re-enter here with watched == cubeWidget.)
    if (watched == cubeWidget)
        return QMainWindow::eventFilter(watched, event);

    // ── Text inputs get all keys — never steal from them ─────────────────────
    {
        QWidget* fw = QApplication::focusWidget();
        if (fw == txtCommand || fw == txtScramble || fw == txtDepths ||
            watched == txtCommand || watched == txtScramble || watched == txtDepths)
            return QMainWindow::eventFilter(watched, event);
    }

    // ── (2) Route cube shortcuts from any other widget ────────────────────────
    if (ke->modifiers() == Qt::NoModifier) {
        auto sendCube = [this](Qt::Key k) {
            QKeyEvent e(QEvent::KeyPress, k, Qt::NoModifier);
            QApplication::sendEvent(cubeWidget, &e);
        };
        bool handled = true;
        switch (ke->key()) {
        case Qt::Key_I: case Qt::Key_K: {
            m_sliceCount++;
            m_slicePending.append({cubeWidget->getPositionString()});
            sendCube(static_cast<Qt::Key>(ke->key()));
            m_sliceTimer->start(600);
            break;
        }
        case Qt::Key_J:                 pushUndoState(); sendCube(Qt::Key_J); break;
        case Qt::Key_F:                 pushUndoState(); sendCube(Qt::Key_F); break;
        case Qt::Key_S:                 pushUndoState(); sendCube(Qt::Key_S); break;
        case Qt::Key_L:                 pushUndoState(); sendCube(Qt::Key_L); break;
        case Qt::Key_Escape:            m_undoStack.clear(); m_redoStack.clear(); btnUndo->setEnabled(false); btnRedo->setEnabled(false); sendCube(Qt::Key_Escape); break;
        case Qt::Key_Z:                 if (!m_undoStack.isEmpty()) btnUndo->click(); break;
        case Qt::Key_Y:                 if (!m_redoStack.isEmpty()) btnRedo->click(); break;
        case Qt::Key_H: pushUndoState(); sendCube(Qt::Key_J); sendCube(Qt::Key_J); break; // UU
        case Qt::Key_G: pushUndoState(); sendCube(Qt::Key_F); sendCube(Qt::Key_F); break; // U'U'
        case Qt::Key_O: pushUndoState(); sendCube(Qt::Key_L); sendCube(Qt::Key_L); break; // D'D'
        case Qt::Key_W: pushUndoState(); sendCube(Qt::Key_S); sendCube(Qt::Key_S); break; // DD
        default: handled = false; break;
        }
        if (handled) return true; // consume — letter goes to cube, not to any text field
    }

    // ── (3) Auto-enable Specific depths when a digit is typed in txtDepths ────
    // txtDepths is kept enabled (never disabled) so it can receive focus and
    // clicks; the user enables the option implicitly by typing a number.
    if (txtDepths->hasFocus() && !chkDepths->isChecked()) {
        const QString text = ke->text();
        if (!text.isEmpty() && text[0].isDigit())
            chkDepths->setChecked(true); // fires updateConstraints → updateCommand
    }

    return QMainWindow::eventFilter(watched, event);
}
