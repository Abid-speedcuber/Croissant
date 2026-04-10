#include "mainwindow.h"
#include "sq1widget.h"
#include "karnotation.h"
#include "theme.h"
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
#include <QPropertyAnimation>
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
    auto updateLogo = [this, logoLabel]() {
        QString primary = m_lightTheme ? Theme::LIGHT_TEXT_PRIMARY : Theme::TEXT_PRIMARY;
        QString muted   = m_lightTheme ? Theme::LIGHT_TEXT_MUTED   : Theme::TEXT_MUTED;
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
        QWidget* cubeWrapper = new QWidget();
        cubeWrapper->setFixedSize(cubeWidget->width(), cubeWidget->height());
        cubeWidget->setParent(cubeWrapper);
        cubeWidget->move(0, 0);

        QWidget* cubeWithReset = new QWidget();
        cubeWithReset->setFixedSize(cubeWrapper->width(), cubeWrapper->height());
        cubeWrapper->setParent(cubeWithReset);
        cubeWrapper->move(0, 0);

        btnReset = new QPushButton("Reset", cubeWithReset);
        btnReset->setObjectName("btnReset");
        btnReset->setFixedSize(52, 52);
        btnReset->setToolTip("Reset  [Esc]");
        btnReset->move(cubeWithReset->width() - 52 - 6, 6);
        btnReset->raise();

        QHBoxLayout* centerRow = new QHBoxLayout();
        centerRow->setContentsMargins(0,0,0,0);
        centerRow->addStretch();
        centerRow->addWidget(cubeWithReset);
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
    btnSlice->setText("Slice [I/K]");
    btnSlice->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    moveGrid->addWidget(btnUP,    0, 0);
    moveGrid->addWidget(btnSlice, 0, 1, 2, 1); // rowspan=2
    moveGrid->addWidget(btnU,     0, 2);
    moveGrid->addWidget(btnD,     1, 0);
    moveGrid->addWidget(btnDP,    1, 2);

    moveGrid->setColumnStretch(0, 1);
    moveGrid->setColumnStretch(1, 1);
    moveGrid->setColumnStretch(2, 1);
    moveGrid->setRowStretch(0, 1);
    moveGrid->setRowStretch(1, 1);

    leftCol->addLayout(moveGrid);

    // Row 3: Undo | Redo
    QHBoxLayout* undoResetRedoRow = new QHBoxLayout();
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

    // Stub out old scramble widgets so references don't break
    btnScrambleMode = new QPushButton(); btnScrambleMode->setVisible(false);
    txtScramble = new QLineEdit();       txtScramble->setVisible(false);
    btnApplyScramble = new QPushButton(); btnApplyScramble->setVisible(false);
    lblScrambleError = new QLabel("");
    lblScrambleError->setObjectName("lblScrambleError");
    lblScrambleError->setWordWrap(true);
    lblScrambleError->setVisible(false);
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
    });
    auto applyInputMode = [this](int idx) {
        m_inputModeIndex = idx;
        static const char* labels[] = {"SCRAMBLE", "ALG", "POSITION"};
        static const char* placeholders[] = {
            "1,0 / 3,3 / 0,-3 / ...  (supports karn)",
            "1,0 / 3,3 / 0,-3 / ...  (supports karn)",
            "ABCDEFGH12345678-"
        };
        m_inputMode->setText(labels[idx]);
        m_mainInput->setPlaceholderText(placeholders[idx]);
        m_mainInput->clear();
        lblScrambleError->setVisible(false);
    };

    // Mode toggle button: cycles SCRAMBLE → ALG → POSITION
    connect(m_inputMode, &QPushButton::clicked, this, [this, applyInputMode]{
        applyInputMode((m_inputModeIndex + 1) % 3);
    });

    // Arrow button: opens dropdown menu
    connect(m_inputModeArrow, &QPushButton::clicked, this, [this, applyInputMode]{
        QMenu* menu = new QMenu(this);
        menu->setStyleSheet(QString(
            "QMenu { background: #1a1a2e; border: 1px solid #3a3a5e; border-radius: 6px; padding: 4px; color: #e0e0e0; font-size: 12px; }"
            "QMenu::item { padding: 6px 20px; border-radius: 4px; }"
            "QMenu::item:selected { background: #3a3a5e; }"
            "QMenu::item:checked { color: #2db570; font-weight: bold; }"
        ));
        QAction* aScram = menu->addAction("Scramble");
        QAction* aAlg   = menu->addAction("Alg");
        QAction* aPos   = menu->addAction("Position");
        aScram->setCheckable(true); aScram->setChecked(m_inputModeIndex == 0);
        aAlg->setCheckable(true);   aAlg->setChecked(m_inputModeIndex == 1);
        aPos->setCheckable(true);   aPos->setChecked(m_inputModeIndex == 2);
        connect(aScram, &QAction::triggered, this, [applyInputMode]{ applyInputMode(0); });
        connect(aAlg,   &QAction::triggered, this, [applyInputMode]{ applyInputMode(1); });
        connect(aPos,   &QAction::triggered, this, [applyInputMode]{ applyInputMode(2); });
        // Show menu below the arrow button
        menu->exec(m_inputModeArrow->mapToGlobal(QPoint(0, m_inputModeArrow->height())));
    });

    connect(m_mainInput, &QLineEdit::textChanged, this, [this](const QString& text){
        lblScrambleError->setVisible(false);
        if (m_inputModeIndex == 2) {
            // Position mode: live update cube
            if (text.trimmed().isEmpty()) return;
            bool ok = cubeWidget->setPositionFromString(text.trimmed());
            if (ok) updateCommand();
        } else {
            // Scramble/Alg mode: live apply on each keystroke
            if (text.trimmed().isEmpty()) {
                cubeWidget->reset();
                updateCommand();
                return;
            }
            // Temporarily set mode flags and reuse onApplyScramble logic
            m_scrambleIsAlg = (m_inputModeIndex == 1);
            txtScramble->setText(text);
            onApplyScramble();
            // Restore undo stack – live updates shouldn't pollute undo
            if (!m_undoStack.isEmpty()) m_undoStack.removeLast();
            btnUndo->setEnabled(!m_undoStack.isEmpty());
        }
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
    chkSlice = new QCheckBox("Slice metric");
    chkSlice->setToolTip("If selected, only slices count as \"moves\", else layer turns count too.");

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

    chkSlice->setChecked(true);
    chkKarnotation->setChecked(true);

    // ── Grid layout ──────────────────────────────────────────────────────────
    int row = 0;
    grid->addWidget(chkSlice,      row++, 0, 1, 2);
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

    connect(chkSlice,      &QCheckBox::toggled, this, upd);
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

    // Hidden command line (does its job under the hood)
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

    btnSolve = new QPushButton("▶ Solve");
    btnSolve->setObjectName("btnSolve");
    btnSolve->setFixedHeight(32);
    btnSolve->setFixedWidth(90);

    // Unified input bar: mode toggle + dropdown arrow + input + solve button
    m_inputMode = new QPushButton("SCRAMBLE");
    m_inputMode->setObjectName("btnInputMode");
    m_inputMode->setFixedHeight(32);
    m_inputMode->setFixedWidth(82);

    m_inputModeArrow = new QPushButton("▾");
    m_inputModeArrow->setObjectName("btnInputModeArrow");
    m_inputModeArrow->setFixedHeight(32);
    m_inputModeArrow->setFixedWidth(20);

    m_mainInput = new QLineEdit();
    m_mainInput->setObjectName("txtMainInput");
    m_mainInput->setFixedHeight(32);
    m_mainInput->setPlaceholderText("1,0 / 3,3 / 0,-3 / ...  (supports karn)");

    cmdSolveLayout->addWidget(m_inputMode);
    cmdSolveLayout->addWidget(m_inputModeArrow);
    cmdSolveLayout->addWidget(m_mainInput, 1);
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
    txtOutput->setStyleSheet("QTextEdit { background: #000000; }");
    txtOutput->document()->setDefaultStyleSheet("div, span { background: transparent !important; }");

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
    QTextCursor cur(txtOutput->document());
    int solIdx = 0;
    for (const QString& line : std::as_const(m_rawLines)) {
        bool isSol = line.contains('[') && line.contains(']');
        if (!cur.atStart()) cur.insertBlock();
        QTextCharFormat fmt;
        if (isSol) {
            bool isAlt = (solIdx % 2 == 1);
            QString col = m_lightTheme
                ? (isAlt ? "#2a6a2a" : "#1a4a8a")
                : (isAlt ? "#cbcbcb" : Theme::TEXT_SOLUTION);
            fmt.setForeground(QColor(col));
            fmt.setFontWeight(m_expanded ? QFont::Bold : QFont::Normal);
            fmt.setFontPointSize(m_expanded ? 13 : 10);
            QTextBlockFormat blkFmt;
            blkFmt.setLineHeight(m_expanded ? 180 : 120, QTextBlockFormat::ProportionalHeight);
            cur.setBlockFormat(blkFmt);
            solIdx++;
        } else {
            QString col = m_lightTheme ? Theme::LIGHT_TEXT_MUTED : Theme::TEXT_MUTED;
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
        txtDepths->setStyleSheet(QString(
            "QLineEdit { color: %1; background: %2; border-color: %3; }")
            .arg(m_lightTheme ? "#aaa" : "#666",
                 m_lightTheme ? Theme::LIGHT_DISABLED_BG : "#1e1e30",
                 m_lightTheme ? Theme::LIGHT_BORDER_DARK : "#3a3a4e"));
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
    setStyleSheet(buildStyleSheet());
}

QString MainWindow::buildStyleSheet() {
    bool L = m_lightTheme;
    auto b = [&](const char* dark, const char* light) -> QString {
        return L ? QString(light) : QString(dark);
    };

    QString PRIMARY_BG      = b(Theme::PRIMARY_BG,       Theme::LIGHT_PRIMARY_BG);
    QString SECONDARY_BG    = b(Theme::SECONDARY_BG,     Theme::LIGHT_TABLE_BG);
    QString TERTIARY_BG     = b(Theme::TERTIARY_BG,      Theme::LIGHT_TERTIARY_BG);
    QString DARK_BG         = b(Theme::DARK_BG,          Theme::LIGHT_DARK_BG);
    QString DISABLED_BG     = b(Theme::DISABLED_BG,      Theme::LIGHT_DISABLED_BG);
    QString HOVER_BG        = b(Theme::HOVER_BG,          Theme::LIGHT_HOVER_BG);
    QString PRESSED_BG      = b(Theme::PRESSED_BG,        Theme::LIGHT_PRESSED_BG);
    QString TEXT_PRIMARY    = b(Theme::TEXT_PRIMARY,      Theme::LIGHT_TEXT_PRIMARY);
    QString TEXT_SECONDARY  = b(Theme::TEXT_SECONDARY,    Theme::LIGHT_TEXT_SECONDARY);
    QString TEXT_MUTED      = b(Theme::TEXT_MUTED,        Theme::LIGHT_TEXT_MUTED);
    QString TEXT_DISABLED   = b(Theme::TEXT_DISABLED,     Theme::LIGHT_TEXT_DISABLED);
    QString TEXT_ERROR      = b(Theme::TEXT_ERROR,        Theme::LIGHT_TEXT_ERROR);
    QString TEXT_CYAN       = b(Theme::TEXT_CYAN,         Theme::LIGHT_TEXT_CYAN);
    QString TEXT_TERMINAL   = b(Theme::TEXT_TERMINAL,     Theme::LIGHT_TEXT_TERMINAL);
    QString TEXT_SOLUTION   = b(Theme::TEXT_SOLUTION,     Theme::LIGHT_TEXT_SOLUTION);
    QString BORDER_LIGHT    = b(Theme::BORDER_LIGHT,      Theme::LIGHT_BORDER_LIGHT);
    QString BORDER_DARK     = b(Theme::BORDER_DARK,       Theme::LIGHT_BORDER_DARK);
    QString BORDER_GROUP    = b(Theme::BORDER_GROUP,      Theme::LIGHT_BORDER_GROUP);
    QString BORDER_BOTTOM   = b(Theme::BORDER_BOTTOM,     Theme::LIGHT_BORDER_BOTTOM);
    QString CHECKBOX_BG     = b(Theme::CHECKBOX_BG,       Theme::LIGHT_CHECKBOX_BG);
    QString CHECKBOX_BORDER = b(Theme::CHECKBOX_BORDER,   Theme::LIGHT_BORDER_LIGHT);
    QString BUTTON_BG       = b(Theme::BUTTON_BG,         Theme::LIGHT_BUTTON_BG);
    QString BUTTON_BORDER   = b(Theme::BUTTON_BORDER,     Theme::LIGHT_BUTTON_BORDER);
    QString BUTTON_TEXT     = b(Theme::BUTTON_TEXT,       Theme::LIGHT_BUTTON_TEXT);
    QString BUTTON_SEC_TEXT = b(Theme::BUTTON_SECONDARY_TEXT, Theme::LIGHT_TEXT_SECONDARY);
    QString SCROLLBAR_BG    = b(Theme::SCROLLBAR_BG,      Theme::LIGHT_SCROLLBAR_BG);
    QString SCROLLBAR_HANDLE= b(Theme::SCROLLBAR_HANDLE,  Theme::LIGHT_SCROLLBAR_HANDLE);
    QString SCROLLBAR_HOVER = b(Theme::SCROLLBAR_HOVER,   Theme::LIGHT_SCROLLBAR_HOVER);
    QString TABLE_BG        = b(Theme::TABLE_BG,          Theme::LIGHT_TABLE_BG);
    QString TABLE_BORDER    = b(Theme::TABLE_BORDER,      Theme::LIGHT_TABLE_BORDER);
    QString TABLE_HEADER_BG = b(Theme::TABLE_HEADER_BG,   Theme::LIGHT_TABLE_HEADER_BG);
    QString TABLE_HEADER_TEXT= b(Theme::TABLE_HEADER_TEXT, Theme::LIGHT_TABLE_HEADER_TEXT);
    QString TABLE_SEL_BG    = b(Theme::TABLE_SELECTED_BG, Theme::LIGHT_TABLE_SELECTED_BG);
    QString TABLE_SEL_TEXT  = b(Theme::TABLE_SELECTED_TEXT, Theme::LIGHT_TABLE_SELECTED_TEXT);
    QString TOOLTIP_BG      = b(Theme::TOOLTIP_BG,        Theme::LIGHT_TOOLTIP_BG);
    QString TOOLTIP_TEXT    = b(Theme::TOOLTIP_TEXT,      Theme::LIGHT_TOOLTIP_TEXT);
    QString TOOLTIP_BORDER  = b(Theme::TOOLTIP_BORDER,    Theme::LIGHT_TOOLTIP_BORDER);
    QString PROGRESS_BG     = b(Theme::PROGRESS_BG,       Theme::LIGHT_PROGRESS_BG);
    QString PROGRESS_FILL   = b(Theme::PROGRESS_FILL,     Theme::LIGHT_PROGRESS_FILL);
    QString UTIL_BG         = b(Theme::BUTTON_UTIL_BG,    Theme::LIGHT_BUTTON_BG);
    QString UTIL_BORDER     = b(Theme::BUTTON_UTIL_BORDER, Theme::LIGHT_BUTTON_BORDER);
    QString UTIL_TEXT       = b(Theme::BUTTON_UTIL_TEXT,  Theme::LIGHT_TEXT_SECONDARY);
    QString UTIL_HOVER_BG   = b(Theme::BUTTON_UTIL_HOVER_BG, Theme::LIGHT_HOVER_BG);
    QString UTIL_HOVER_BORDER= b(Theme::BUTTON_UTIL_HOVER_BORDER, Theme::LIGHT_BORDER_LIGHT);
    QString UTIL_HOVER_TEXT = b(Theme::BUTTON_UTIL_HOVER_TEXT, Theme::LIGHT_TEXT_PRIMARY);
    QString ABOUT_BG        = b(Theme::BUTTON_ABOUT_BG,   Theme::LIGHT_BUTTON_BG);
    QString ABOUT_BORDER    = b(Theme::BUTTON_ABOUT_BORDER, Theme::LIGHT_BUTTON_BORDER);
    QString ABOUT_TEXT      = b(Theme::BUTTON_ABOUT_TEXT, Theme::LIGHT_TEXT_SECONDARY);
    QString ABOUT_HOVER_BG  = b(Theme::BUTTON_ABOUT_HOVER_BG, Theme::LIGHT_HOVER_BG);
    QString ABOUT_HOVER_BORDER = b(Theme::BUTTON_ABOUT_HOVER_BORDER, Theme::LIGHT_BORDER_LIGHT);
    QString ABOUT_HOVER_TEXT= b(Theme::BUTTON_ABOUT_HOVER_TEXT, Theme::LIGHT_TEXT_PRIMARY);
    QString RESET_BG        = b(Theme::BUTTON_RESET_BG,   Theme::LIGHT_BUTTON_BG);
    QString RESET_BORDER    = b(Theme::BUTTON_RESET_BORDER, Theme::LIGHT_BUTTON_BORDER);
    QString RESET_HOVER     = b(Theme::BUTTON_RESET_HOVER, Theme::LIGHT_HOVER_BG);
    QString UNDO_REDO_DISABLED_BG = b("#1e1e2a", Theme::LIGHT_DISABLED_BG);
    QString UNDO_REDO_DISABLED_BORDER = b("#333", Theme::LIGHT_BORDER_DARK);
    QString UNDO_REDO_DISABLED_TEXT   = b("#444", Theme::LIGHT_TEXT_DISABLED);
    QString SPINBOX_ARROW_BG  = b("#3a3a5a", Theme::LIGHT_HOVER_BG);
    QString SPINBOX_ARROW_HOVER = b("#5a5a8a", Theme::LIGHT_BORDER_LIGHT);
    QString INPUT_TEXT = b("#fff", Theme::LIGHT_TEXT_PRIMARY);

    return QString(R"(
        QMainWindow, QWidget { background: %1; color: %2; font-family: 'Segoe UI', Arial; font-size: 13px; }
        QGroupBox { border: 1px solid %3; border-radius: 6px; margin-top: 8px; padding-top: 8px; color: %4; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QCheckBox { spacing: 6px; }
        QCheckBox::indicator { width:14px; height:14px; border-radius:3px; border:1px solid %5; background:%6; }
        QCheckBox::indicator:checked { background: %7; border-color: %7; }
        QCheckBox#chkRankErgo::indicator:checked { background: %8; border-color: %8; }
        QCheckBox:disabled { color: #4a4a5a; }
        QCheckBox::indicator:disabled { border-color: %9; background: %10; }
        QLineEdit { background: %11; border: 1px solid %12; border-radius: 4px; padding: 3px 6px; color: %60; }
        QLineEdit:disabled { color: %13; background: %14; border-color: %15; }
        QLineEdit#txtCommand { font-family: monospace; color: %16; font-size: 12px; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; border-right: none; }
        QSpinBox {
            background: %11; border: 1px solid %12; border-radius: 4px;
            padding: 2px 4px 2px 6px; color: %60;
            min-width: 48px;
        }
        QSpinBox:disabled { color: %13; background: %14; border-color: %15; }
        QSpinBox::up-button {
            subcontrol-origin: border; subcontrol-position: top right;
            width: 16px; border-left: 1px solid %12;
            border-top-right-radius: 4px;
            background: %61;
        }
        QSpinBox::down-button {
            subcontrol-origin: border; subcontrol-position: bottom right;
            width: 16px; border-left: 1px solid %12; border-top: 1px solid %12;
            border-bottom-right-radius: 4px;
            background: %61;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: %62; }
        QSpinBox::up-arrow   { width: 6px; height: 6px;
                               border-left: 3px solid transparent;
                               border-right: 3px solid transparent;
                               border-bottom: 4px solid %2; }
        QSpinBox::down-arrow { width: 6px; height: 6px;
                               border-left: 3px solid transparent;
                               border-right: 3px solid transparent;
                               border-top: 4px solid %2; }
        QTextEdit#txtOutput { background: %17; border: 1px solid %3; border-radius: 4px;
                              font-family: monospace; font-size: 12px; color: %18; padding: 4px; }
        QTextEdit#txtOutput > QWidget { background: %17; }
        QPushButton { background: %19; border: 1px solid %20; border-radius: 5px; padding: 5px 12px; color: %21; }
        QPushButton:hover { background: %22; border-color: %20; }
        QPushButton:pressed { background: %23; }
        QPushButton#btnSolve { background: %24; border-color: %25; color: #fff; font-size: 13px; font-weight: bold; }
        QPushButton#btnSolve:hover { background: %26; }
        QPushButton#btnSolve:disabled { background: #333; border-color: #444; color: #666; }
        QPushButton#btnCopy { background: %11; border: 1px solid %12; border-left: none; border-radius: 4px; border-top-left-radius: 0px; border-bottom-left-radius: 0px; color: %4; font-size: 14px; padding: 0; margin-right: 5px;}
        QPushButton#btnCopy:hover { background: %22; color: %21; }
        QPushButton#btnReset {
            background: %27; border: 1px solid %28; border-radius: 26px;
            color: %4; font-size: 11px; font-weight: bold; padding: 0;
            letter-spacing: -0.5px;
        }
        QPushButton#btnReset:hover { background: %29; border-color: %20; color: %21; }
        QPushButton#btnUndo, QPushButton#btnRedo {
            background: %19; border-color: %20; color: %21;
        }
        QPushButton#btnUndo:hover, QPushButton#btnRedo:hover {
            background: %22; border-color: %20;
        }
        QPushButton#btnUndo:pressed, QPushButton#btnRedo:pressed {
            background: %23;
        }
        QPushButton#btnUndo:disabled, QPushButton#btnRedo:disabled {
            background: %63; border-color: %64; color: %65;
        }
        QPushButton#btnApplyScramble {
            background: %11; border: 1px solid %12;
            border-radius: 4px; color: %4; padding: 4px 8px;
        }
        QPushButton#btnApplyScramble:hover { background: %22; border-color: %20; }
        QPushButton#btnScrambleMode {
            background: #333350; border: 1px solid %12;
            border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px;
            border-right: none; color: #fff; padding: 0 6px; font-size: 11px;
        }
        QPushButton#btnScrambleMode:checked { color: #fff; background: #333350; }
        QPushButton#btnScrambleMode:hover { background: %22; }
        QLineEdit#txtScramble {
            border-top-left-radius: 0; border-bottom-left-radius: 0;
        }
        QPushButton#btnInputMode {
            background: #1a6b3c; border: 1px solid #2db570;
            border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px;
            border-right: none; color: #fff; padding: 0 8px; font-size: 11px; font-weight: bold;
        }
        QPushButton#btnInputMode:hover { background: #227a47; }
        QPushButton#btnInputModeArrow {
            background: #1a6b3c; border: 1px solid #2db570;
            border-radius: 0; border-left: 1px solid #2db570;
            border-right: none; color: #fff; padding: 0; font-size: 11px;
        }
        QPushButton#btnInputModeArrow:hover { background: #227a47; }
        QLineEdit#txtMainInput {
            border-top-left-radius: 0; border-bottom-left-radius: 0;
            border-right: none; border-radius: 0;
            font-family: monospace; font-size: 12px;
        }
        QPushButton#btnExpand, QPushButton#btnCopyTerminal, QPushButton#btnTableMode {
            background: %30; border: 1px solid %31; border-radius: 4px;
            color: %32; font-size: 13px; padding: 0;
        }
        QPushButton#btnExpand:hover, QPushButton#btnCopyTerminal:hover, QPushButton#btnTableMode:hover {
            background: %33; border-color: %34; color: %35;
        }
        QPushButton#btnExpand:pressed, QPushButton#btnCopyTerminal:pressed, QPushButton#btnTableMode:pressed {
            background: %23;
        }
        QProgressBar { border: none; background: %36; border-radius: 3px; }
        QProgressBar::chunk { background: %37; border-radius: 3px; }
        QLabel#lblStatus { color: %38; font-size: 11px; }
        QLabel#lblScrambleError { color: %39; font-size: 11px; padding: 2px 2px; }
        QLabel#lblCommandError  { color: %39; font-size: 11px; padding: 2px 2px; }
        QScrollBar:vertical { background: %40; width: 8px; border-radius: 4px; margin: 0; }
        QScrollBar::handle:vertical { background: %41; border-radius: 4px; min-height: 24px; }
        QScrollBar::handle:vertical:hover { background: %42; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; border: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
        QScrollBar:horizontal { background: %40; height: 8px; border-radius: 4px; margin: 0; }
        QScrollBar::handle:horizontal { background: %41; border-radius: 4px; min-width: 24px; }
        QScrollBar::handle:horizontal:hover { background: %42; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; border: 0; }
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }
        QTableWidget#m_solutionTable {
            background: %43; border: 1px solid %44;
            border-radius: 4px; gridline-color: %45;
            font-family: monospace; font-size: 12px; color: %2;
        }
        QTableWidget#m_solutionTable QHeaderView::section {
            background: %46; color: %47; border: none;
            border-bottom: 1px solid %44; padding: 4px;
            font-size: 11px; font-weight: bold;
        }
        QTableWidget#m_solutionTable::item { }
        QTableWidget#m_solutionTable::item:selected {
            background: %48; color: %49;
        }
        QWidget#topBar {
            background: %50;
            border-bottom: 2px solid %51;
            min-height: 52px;
            max-height: 52px;
        }
        QLabel#logoLabel {
            background: transparent;
            padding: 0;
        }
        QPushButton#btnAbout, QPushButton#btnHamburger {
            background: %52;
            border: 1px solid %53;
            border-radius: 15px;
            color: %54;
            font-size: 15px;
            font-weight: bold;
            padding: 0;
            min-width: 30px;
            max-width: 30px;
            min-height: 30px;
            max-height: 30px;
        }
        QPushButton#btnAbout:hover, QPushButton#btnHamburger:hover {
            background: %55;
            border-color: %56;
            color: %57;
        }
        QToolTip {
            background: %58;
            color: %59;
            border: 1px solid %58;
            border-radius: 5px;
            padding: 6px 10px;
            font-size: 12px;
            opacity: 230;
        }
    )")
        .arg(PRIMARY_BG)          // %1  main bg
        .arg(TEXT_PRIMARY)        // %2  main text
        .arg(BORDER_GROUP)        // %3  group border
        .arg(TEXT_SECONDARY)      // %4  secondary text
        .arg(CHECKBOX_BORDER)     // %5
        .arg(CHECKBOX_BG)         // %6
        .arg(Theme::CHECKBOX_CHECK_BLUE)  // %7
        .arg(Theme::CHECKBOX_CHECK_ORANGE)// %8
        .arg(BORDER_DARK)         // %9
        .arg(DISABLED_BG)         // %10
        .arg(TERTIARY_BG)         // %11 input/button bg
        .arg(BORDER_LIGHT)        // %12 input border
        .arg(TEXT_DISABLED)       // %13
        .arg(DISABLED_BG)         // %14
        .arg(BORDER_DARK)         // %15
        .arg(TEXT_CYAN)           // %16 command line text
        .arg(SECONDARY_BG)        // %17 terminal bg
        .arg(TEXT_TERMINAL)       // %18
        .arg(BUTTON_BG)           // %19
        .arg(BUTTON_BORDER)       // %20
        .arg(BUTTON_TEXT)         // %21
        .arg(HOVER_BG)            // %22
        .arg(PRESSED_BG)          // %23
        .arg(Theme::BUTTON_SOLVE_BG)     // %24
        .arg(Theme::BUTTON_SOLVE_BORDER) // %25
        .arg(Theme::BUTTON_SOLVE_HOVER)  // %26
        .arg(RESET_BG)            // %27
        .arg(RESET_BORDER)        // %28
        .arg(RESET_HOVER)         // %29
        .arg(UTIL_BG)             // %30 floating btns bg
        .arg(UTIL_BORDER)         // %31
        .arg(UTIL_TEXT)           // %32
        .arg(UTIL_HOVER_BG)       // %33
        .arg(UTIL_HOVER_BORDER)   // %34
        .arg(UTIL_HOVER_TEXT)     // %35
        .arg(PROGRESS_BG)         // %36
        .arg(PROGRESS_FILL)       // %37
        .arg(TEXT_MUTED)          // %38 status text
        .arg(TEXT_ERROR)          // %39
        .arg(SCROLLBAR_BG)        // %40
        .arg(SCROLLBAR_HANDLE)    // %41
        .arg(SCROLLBAR_HOVER)     // %42
        .arg(TABLE_BG)            // %43
        .arg(TABLE_BORDER)        // %44
        .arg(TABLE_BORDER)        // %45 gridline
        .arg(TABLE_HEADER_BG)     // %46
        .arg(TABLE_HEADER_TEXT)   // %47
        .arg(TABLE_SEL_BG)        // %48
        .arg(TABLE_SEL_TEXT)      // %49
        .arg(DARK_BG)             // %50 topbar bg
        .arg(BORDER_BOTTOM)       // %51
        .arg(ABOUT_BG)            // %52
        .arg(ABOUT_BORDER)        // %53
        .arg(ABOUT_TEXT)          // %54
        .arg(ABOUT_HOVER_BG)      // %55
        .arg(ABOUT_HOVER_BORDER)  // %56
        .arg(ABOUT_HOVER_TEXT)    // %57
        .arg(TOOLTIP_BG)          // %58
        .arg(TOOLTIP_TEXT)        // %59
        .arg(INPUT_TEXT)          // %60 input text color
        .arg(SPINBOX_ARROW_BG)    // %61 spinbox button bg
        .arg(SPINBOX_ARROW_HOVER) // %62 spinbox button hover
        .arg(UNDO_REDO_DISABLED_BG)    // %63
        .arg(UNDO_REDO_DISABLED_BORDER)// %64
        .arg(UNDO_REDO_DISABLED_TEXT); // %65
}

// -------------------------------------------------------
// buildArgList
// -------------------------------------------------------
QStringList MainWindow::buildArgList() {
    QStringList args;

    if (chkSlice->isChecked()) args << "-w";

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

    block(chkSlice,        true); chkSlice->setChecked(has("-w"));        block(chkSlice,        false);
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
    QString cyan = m_lightTheme ? Theme::LIGHT_TEXT_CYAN : Theme::TEXT_CYAN;
    txtCommand->setStyleSheet(QString("QLineEdit#txtCommand { font-family: monospace; color: %1; font-size: 12px; border-right: none; border-radius: 0; border-top-left-radius: 4px; border-bottom-left-radius: 4px; }").arg(cyan));
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
    btnSolve->setStyleSheet(QString(
        "QPushButton#btnSolve {"
        "  background: %1; border: 1px solid %2; padding-top: 0px; padding-bottom: 0px;"
        "  color: %3; font-size: 12px; font-weight: bold; }"
        "QPushButton#btnSolve:hover { background: %4; }")
        .arg(Theme::BUTTON_STOP_BG, Theme::BUTTON_STOP_BORDER, Theme::BUTTON_STOP_TEXT, Theme::BUTTON_STOP_HOVER));

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
        updateRankErgoState();
        {
            bool isAlt = (m_solutionLines.size() % 2 == 0);
            QString col = m_lightTheme
                ? (isAlt ? "#2a6a2a" : "#1a4a8a")
                : (isAlt ? "#cbcbcb" : Theme::TEXT_SOLUTION);
            QTextCursor cur = txtOutput->textCursor();
            cur.movePosition(QTextCursor::End);
            if (!txtOutput->document()->isEmpty()) cur.insertBlock();
            QTextBlockFormat blkFmt;
            blkFmt.setLineHeight(m_expanded ? 180 : 120, QTextBlockFormat::ProportionalHeight);
            cur.setBlockFormat(blkFmt);
            QTextCharFormat fmt;
            fmt.setForeground(QColor(col));
            fmt.setFontWeight(m_expanded ? QFont::Bold : QFont::Normal);
            fmt.setFontPointSize(m_expanded ? 13 : 10);
            cur.insertText(line, fmt);
            txtOutput->setTextCursor(cur);
        }
    } else {
        QString col = m_lightTheme ? Theme::LIGHT_TEXT_MUTED : Theme::TEXT_MUTED;
        QTextCursor cur = txtOutput->textCursor();
        cur.movePosition(QTextCursor::End);
        if (!txtOutput->document()->isEmpty()) cur.insertBlock();
        QTextBlockFormat blkFmt;
        blkFmt.setLineHeight(m_expanded ? 150 : 120, QTextBlockFormat::ProportionalHeight);
        cur.setBlockFormat(blkFmt);
        QTextCharFormat fmt;
        fmt.setForeground(QColor(col));
        fmt.setFontPointSize(m_expanded ? 11 : 10);
        cur.insertText(line, fmt);
        txtOutput->setTextCursor(cur);
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
    auto isSliceable = [&]() {
        return pos[0]!=pos[11] && pos[5]!=pos[6] &&
               pos[12]!=pos[23] && pos[17]!=pos[18];
    };
    auto doSlice = [&]() {
        if (!isSliceable()) return;
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
    QString col = m_lightTheme ? Theme::LIGHT_TEXT_TERMINAL : "#6a9ab8";
    QTextCursor cur = txtOutput->textCursor();
    cur.movePosition(QTextCursor::End);
    if (!txtOutput->document()->isEmpty()) cur.insertBlock();
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
    // rated[i].first is the full line with slice indicator injected (bracket still attached).
    // Build rows directly from rated so the display alg already has the indicator.
    for (auto& [line, score] : rated) {
        int mv, sl;
        parseCounts(line, mv, sl);
        QString displayAlg = stripBracket(line);
        rows.append({displayAlg, mv, sl, score});
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

    const QColor rowA = QColor(m_lightTheme ? Theme::LIGHT_TABLE_BG : Theme::ROW_ALT_DARK);
    const QColor rowB = m_lightTheme ? rowA : QColor(Theme::ROW_ALT_LIGHT);
    // Dark mode: row A gets blue text, row B gets white text (alternating)
    const QColor textCol    = m_lightTheme ? QColor(Theme::LIGHT_TEXT_SOLUTION) : QColor(Theme::TEXT_SOLUTION);
        const QColor textColAlt = m_lightTheme ? QColor(Theme::LIGHT_TEXT_SOLUTION) : QColor("#cbcbcb");
    const QColor metaCol = m_lightTheme ? QColor(Theme::LIGHT_TEXT_SECONDARY) : QColor(154, 172, 190);
    const QColor metaColAlt = m_lightTheme ? QColor(Theme::LIGHT_TEXT_SECONDARY) : QColor(150, 150, 150, 238);
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
            QColor c = isMeta ? (isAltRow ? metaColAlt : metaCol)
                               : (isAltRow ? textColAlt : textCol);
            item->setForeground(c);
            if (m_expanded) {
                QFont f = item->font();
                f.setPointSize(fontSize);
                item->setFont(f);
            }
            item->setTextAlignment(Qt::AlignCenter);
            m_solutionTable->setItem(i, col, item);
        };

        QTableWidgetItem* numItem = new QTableWidgetItem(QString::number(i+1));
        numItem->setBackground(bg);
        numItem->setForeground(isAltRow ? metaColAlt : metaCol);
        numItem->setTextAlignment(Qt::AlignCenter);
        {
            QFont f = numItem->font();
            f.setPointSize(m_expanded ? fontSize - 2 : 10);
            f.setItalic(true);
            numItem->setFont(f);
        }
        m_solutionTable->setItem(i, 0, numItem);
        // Solution column: left-aligned
        QTableWidgetItem* algItem = new QTableWidgetItem(r.alg);
        algItem->setBackground(bg);
        algItem->setForeground(isAltRow ? textColAlt : textCol);
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
    QTextCursor cur(txtOutput->document());
    bool firstBlock = true;
    auto insertLine = [&](const QString& text, const QString& color, bool bold, int ptSize, int lineH) {
        if (!firstBlock) cur.insertBlock();
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
    for (const QString& line : std::as_const(m_rawLines)) {
        bool isSol = line.contains('[') && line.contains(']');
        if (!isSol) {
            QString col = m_lightTheme ? Theme::LIGHT_TEXT_MUTED : Theme::TEXT_MUTED;
            insertLine(line, col, false, m_expanded ? 11 : 10, m_expanded ? 150 : 120);
        }
    }
    int solIdx = 0;
    for (auto& [line, score] : rated) {
        bool isAlt = (solIdx % 2 == 1);
        QString col = m_lightTheme
            ? (isAlt ? "#2a6a2a" : "#1a4a8a")
            : (isAlt ? "#cbcbcb" : Theme::TEXT_SOLUTION);
        QString display = QString("%1  (%2)").arg(line).arg(score, 0, 'f', 2);
        insertLine(display, col, m_expanded, m_expanded ? 13 : 10, m_expanded ? 180 : 120);
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
    const bool cubeshapeActive = m_solutionLines.isEmpty() ? chkCubeshape->isChecked() : m_cubeshapeWasActive;
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

    bool L = m_lightTheme;
    QString modalBg     = L ? Theme::LIGHT_PRIMARY_BG  : Theme::MODAL_BG;
    QString modalBorder = L ? Theme::LIGHT_BORDER_GROUP : Theme::MODAL_BORDER;
    QWidget* card = new QWidget(overlay);
    card->setObjectName("aboutCard");
    card->setFixedWidth(480);
    card->setStyleSheet(QString(
        "QWidget#aboutCard { background:%1; border:1px solid %2; border-radius:10px; }"
    ).arg(modalBg, modalBorder));

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(10);

    QString textPrimary = L ? Theme::LIGHT_TEXT_PRIMARY   : "#e0e0e0";
    QString textBody    = L ? Theme::LIGHT_TEXT_SECONDARY : "#b0b0c8";
    QLabel* title = new QLabel("About Solve-A-Squan");
    title->setStyleSheet(QString("font-size:16px;font-weight:bold;color:%1;background:transparent;").arg(textPrimary));

    QLabel* body = new QLabel();
    body->setWordWrap(true);
    body->setOpenExternalLinks(true);
    body->setTextFormat(Qt::RichText);
    body->setStyleSheet("background:transparent;");
    body->setText(QString(
        "<span style='color:%1;font-size:12px;line-height:1.7;'>").arg(textBody) +
        QString(
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
    ));

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

void MainWindow::applyTheme() {
    setStyleSheet(buildStyleSheet());
    if (m_updateLogo) m_updateLogo();
    updateConstraints();
    QString termBg = m_lightTheme ? Theme::LIGHT_SECONDARY_BG : "#000000";
    txtOutput->setStyleSheet(QString("QTextEdit { background: %1; }").arg(termBg));
    txtOutput->document()->setDefaultStyleSheet("div, span { background: transparent !important; }");
    if (!m_rawLines.isEmpty()) {
        if (chkRankErgo->isChecked()) onRankErgoToggled(true);
        else rebuildTerminalView();
    }
    if (m_tableVisible) rebuildTable();
    // Repaint the cube widget with the right canvas bg
    QString canvasBg = m_lightTheme ? Theme::LIGHT_CANVAS_BG : Theme::PRIMARY_BG;
    cubeWidget->setStyleSheet(QString("background: %1;").arg(canvasBg));
    // Update command line color
    QString cyan = m_lightTheme ? Theme::LIGHT_TEXT_CYAN : Theme::TEXT_CYAN;
    txtCommand->setStyleSheet(QString(
        "QLineEdit#txtCommand { font-family: monospace; color: %1; font-size: 12px;"
        " border-right: none; border-radius: 0; border-top-left-radius: 4px;"
        " border-bottom-left-radius: 4px; }").arg(cyan));
}

void MainWindow::openSidebar() {
    if (m_sidebarOpen) return;
    m_sidebarOpen = true;

    QWidget* central = this->centralWidget();

    m_sidebarOverlay = new QWidget(central);
    m_sidebarOverlay->setGeometry(central->rect());
    m_sidebarOverlay->setStyleSheet("background: rgba(0,0,0,120);");
    m_sidebarOverlay->show();
    m_sidebarOverlay->raise();

    bool L = m_lightTheme;
    QString sidebarBg     = L ? Theme::LIGHT_SIDEBAR_BG     : Theme::SIDEBAR_BG;
    QString sidebarBorder = L ? Theme::LIGHT_SIDEBAR_BORDER  : Theme::SIDEBAR_BORDER;
    QString textPrimary   = L ? Theme::LIGHT_TEXT_PRIMARY    : Theme::TEXT_PRIMARY;
    QString textMuted     = L ? Theme::LIGHT_TEXT_MUTED      : Theme::TEXT_MUTED;
    QString hoverBg       = L ? Theme::LIGHT_HOVER_BG        : Theme::HOVER_BG;
    QString btnBg         = L ? Theme::LIGHT_BUTTON_BG       : Theme::BUTTON_BG;
    QString btnBorder     = L ? Theme::LIGHT_BUTTON_BORDER   : Theme::BUTTON_BORDER;

    m_sidebar = new QWidget(m_sidebarOverlay);
    m_sidebar->setFixedWidth(220);
    m_sidebar->setStyleSheet(QString(
        "QWidget { background: %1; border-right: 2px solid %2; }"
    ).arg(sidebarBg, sidebarBorder));
    m_sidebar->setGeometry(-220, 0, 220, central->height());

    QVBoxLayout* slay = new QVBoxLayout(m_sidebar);
    slay->setContentsMargins(0, 0, 0, 0);
    slay->setSpacing(0);

    // Header
    QWidget* sHeader = new QWidget();
    sHeader->setFixedHeight(52);
    sHeader->setStyleSheet(QString("QWidget { background: %1; border-bottom: 1px solid %2; border-right: none; }").arg(
        L ? Theme::LIGHT_DARK_BG : Theme::DARK_BG, sidebarBorder));
    QHBoxLayout* shLay = new QHBoxLayout(sHeader);
    shLay->setContentsMargins(16, 0, 12, 0);
    QLabel* sTitle = new QLabel("Menu");
    sTitle->setStyleSheet(QString("font-size:15px;font-weight:bold;color:%1;background:transparent;border:none;").arg(textPrimary));
    QPushButton* closeBtn = new QPushButton("✕");
    closeBtn->setFixedSize(26, 26);
    closeBtn->setStyleSheet(QString(
        "QPushButton { background:%1; border:1px solid %2; border-radius:13px; color:%3; font-size:12px; padding:0; }"
        "QPushButton:hover { background:%4; }").arg(btnBg, btnBorder, textMuted, hoverBg));
    connect(closeBtn, &QPushButton::clicked, this, &MainWindow::closeSidebar);
    shLay->addWidget(sTitle);
    shLay->addStretch();
    shLay->addWidget(closeBtn);
    slay->addWidget(sHeader);

    // Menu items
    auto makeItem = [&](const QString& icon, const QString& label) -> QPushButton* {
        QPushButton* btn = new QPushButton(QString("  %1  %2").arg(icon, label));
        btn->setFixedHeight(48);
        btn->setStyleSheet(QString(
            "QPushButton { background: transparent; border: none; border-bottom: 1px solid %1;"
            " color: %2; font-size: 13px; text-align: left; padding-left: 8px; border-radius: 0; }"
            "QPushButton:hover { background: %3; }"
            "QPushButton:pressed { background: %4; }"
        ).arg(sidebarBorder, textPrimary, hoverBg, btnBg));
        return btn;
    };

    QPushButton* itemSettings   = makeItem("⚙", "Settings");
    QPushButton* itemHowToUse   = makeItem("?", "How to Use");
    QPushButton* itemAbout      = makeItem("ℹ", "About");

    connect(itemSettings, &QPushButton::clicked, this, [this]{ closeSidebar(); showSettingsModal(); });
    connect(itemHowToUse, &QPushButton::clicked, this, [this]{ closeSidebar(); showHowToUseModal(); });
    connect(itemAbout,    &QPushButton::clicked, this, [this]{ closeSidebar(); showAboutModal(); });

    slay->addWidget(itemSettings);
    slay->addWidget(itemHowToUse);
    slay->addWidget(itemAbout);
    slay->addStretch();

    m_sidebar->show();
    m_sidebar->raise();

    // Animate slide-in
    QPropertyAnimation* anim = new QPropertyAnimation(m_sidebar, "geometry");
    anim->setDuration(180);
    anim->setStartValue(QRect(-220, 0, 220, central->height()));
    anim->setEndValue(QRect(0, 0, 220, central->height()));
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->start(QAbstractAnimation::DeleteWhenStopped);

    // Close on overlay click
    struct SidebarFilter : public QObject {
        MainWindow* mw; QWidget* overlay; QWidget* sidebar;
        SidebarFilter(MainWindow* m, QWidget* o, QWidget* s)
            : QObject(o), mw(m), overlay(o), sidebar(s) {}
        bool eventFilter(QObject* watched, QEvent* e) override {
            if (e->type() == QEvent::MouseButtonPress && watched == overlay) {
                QMouseEvent* me = static_cast<QMouseEvent*>(e);
                if (!sidebar->geometry().contains(me->pos()))
                    mw->closeSidebar();
                return true;
            }
            if (e->type() == QEvent::Resize && watched == overlay->parentWidget()) {
                overlay->setGeometry(overlay->parentWidget()->rect());
                sidebar->setFixedHeight(overlay->height());
                return false;
            }
            return false;
        }
    };
    SidebarFilter* sf = new SidebarFilter(this, m_sidebarOverlay, m_sidebar);
    central->installEventFilter(sf);
    m_sidebarOverlay->installEventFilter(sf);
}

void MainWindow::closeSidebar() {
    if (!m_sidebarOpen || !m_sidebar) return;
    QWidget* sb = m_sidebar;
    QWidget* ov = m_sidebarOverlay;
    QPropertyAnimation* anim = new QPropertyAnimation(sb, "geometry");
    anim->setDuration(150);
    anim->setStartValue(sb->geometry());
    anim->setEndValue(QRect(-220, 0, 220, sb->height()));
    anim->setEasingCurve(QEasingCurve::InCubic);
    connect(anim, &QPropertyAnimation::finished, this, [ov, this]{
        if (ov) ov->deleteLater();
        m_sidebar = nullptr;
        m_sidebarOverlay = nullptr;
        m_sidebarOpen = false;
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::showSettingsModal() {
    QWidget* central = this->centralWidget();
    bool L = m_lightTheme;
    QString modalBg     = L ? Theme::LIGHT_PRIMARY_BG   : Theme::MODAL_BG;
    QString modalBorder = L ? Theme::LIGHT_BORDER_GROUP  : Theme::MODAL_BORDER;
    QString textPrimary = L ? Theme::LIGHT_TEXT_PRIMARY  : Theme::TEXT_PRIMARY;
    QString textMuted   = L ? Theme::LIGHT_TEXT_MUTED    : Theme::TEXT_MUTED;

    QWidget* overlay = new QWidget(central);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background: rgba(0,0,0,160);");
    overlay->show(); overlay->raise();

    QWidget* card = new QWidget(overlay);
    card->setObjectName("settingsCard");
    card->setFixedWidth(380);
    card->setStyleSheet(QString(
        "QWidget#settingsCard { background:%1; border:1px solid %2; border-radius:10px; }"
    ).arg(modalBg, modalBorder));

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(28, 24, 28, 24);
    lay->setSpacing(14);

    QLabel* title = new QLabel("Settings");
    title->setStyleSheet(QString("font-size:16px;font-weight:bold;color:%1;background:transparent;").arg(textPrimary));
    lay->addWidget(title);

    // Theme toggle
    QCheckBox* chkLight = new QCheckBox("Light theme");
    chkLight->setChecked(m_lightTheme);
    chkLight->setStyleSheet(QString("color:%1;background:transparent;font-size:13px;").arg(textPrimary));
    connect(chkLight, &QCheckBox::toggled, this, [this, overlay](bool checked){
        m_lightTheme = checked;
        applyTheme();
        // Rebuild overlay style so it doesn't look stale
        overlay->setStyleSheet("background: rgba(0,0,0,160);");
    });
    lay->addWidget(chkLight);

    QLabel* hint = new QLabel("More settings coming soon.");
    hint->setStyleSheet(QString("color:%1;font-size:11px;background:transparent;").arg(textMuted));
    lay->addWidget(hint);

    card->show(); card->adjustSize();

    auto center = [overlay, card](){
        overlay->setGeometry(overlay->parentWidget()->rect());
        card->move((overlay->width()-card->width())/2, (overlay->height()-card->height())/2);
    };
    center(); card->raise();

    struct F : public QObject {
        QWidget* overlay; QWidget* card; std::function<void()> fn;
        F(QWidget* o, QWidget* c, std::function<void()> f): QObject(o),overlay(o),card(c),fn(f){}
        bool eventFilter(QObject* w, QEvent* e) override {
            if (e->type()==QEvent::Resize && w==overlay->parentWidget()){ fn(); return false; }
            if (e->type()==QEvent::MouseButtonPress && w==overlay){
                if (!card->geometry().contains(static_cast<QMouseEvent*>(e)->pos()))
                    overlay->deleteLater();
                return true;
            }
            return false;
        }
    };
    F* f = new F(overlay, card, center);
    central->installEventFilter(f);
    overlay->installEventFilter(f);
}

void MainWindow::showHowToUseModal() {
    QWidget* central = this->centralWidget();
    bool L = m_lightTheme;
    QString modalBg     = L ? Theme::LIGHT_PRIMARY_BG   : Theme::MODAL_BG;
    QString modalBorder = L ? Theme::LIGHT_BORDER_GROUP  : Theme::MODAL_BORDER;
    QString textPrimary = L ? Theme::LIGHT_TEXT_PRIMARY  : Theme::TEXT_PRIMARY;
    QString textBody    = L ? Theme::LIGHT_TEXT_SECONDARY: "#b0b0c8";
    QString textCyan    = L ? Theme::LIGHT_TEXT_CYAN     : "#7abfe8";
    QString scrollBg    = L ? Theme::LIGHT_SCROLLBAR_BG  : Theme::SCROLLBAR_BG;
    QString scrollHandle= L ? Theme::LIGHT_SCROLLBAR_HANDLE : Theme::SCROLLBAR_HANDLE;

    QWidget* overlay = new QWidget(central);
    overlay->setGeometry(central->rect());
    overlay->setStyleSheet("background: rgba(0,0,0,160);");
    overlay->show(); overlay->raise();

    QWidget* card = new QWidget(overlay);
    card->setObjectName("howToCard");
    int cardW = qMin(560, central->width() - 60);
    int cardH = qMin(520, central->height() - 80);
    card->setFixedSize(cardW, cardH);
    card->setStyleSheet(QString(
        "QWidget#howToCard { background:%1; border:1px solid %2; border-radius:10px; }"
    ).arg(modalBg, modalBorder));

    QVBoxLayout* lay = new QVBoxLayout(card);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);

    // Title bar
    QWidget* titleBar = new QWidget();
    titleBar->setFixedHeight(48);
    titleBar->setStyleSheet(QString(
        "QWidget { background: %1; border-bottom: 1px solid %2;"
        " border-top-left-radius: 10px; border-top-right-radius: 10px; border-bottom-left-radius:0; border-bottom-right-radius:0; }"
    ).arg(modalBg, modalBorder));
    QHBoxLayout* tbLay = new QHBoxLayout(titleBar);
    tbLay->setContentsMargins(20, 0, 16, 0);
    QLabel* titleLbl = new QLabel("How to Use");
    titleLbl->setStyleSheet(QString("font-size:15px;font-weight:bold;color:%1;background:transparent;").arg(textPrimary));
    tbLay->addWidget(titleLbl);
    tbLay->addStretch();
    lay->addWidget(titleBar);

    // Scrollable content
    QScrollArea* sa = new QScrollArea();
    sa->setWidgetResizable(true);
    sa->setFrameShape(QFrame::NoFrame);
    sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sa->setStyleSheet(QString(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { background: %1; width: 6px; border-radius: 3px; }"
        "QScrollBar::handle:vertical { background: %2; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height:0; border:0; }"
    ).arg(scrollBg, scrollHandle));

    QLabel* body = new QLabel();
    body->setWordWrap(true);
    body->setOpenExternalLinks(false);
    body->setTextFormat(Qt::RichText);
    body->setContentsMargins(20, 16, 20, 16);
    body->setStyleSheet(QString("background: transparent; color: %1; font-size: 12px; line-height: 1.7;").arg(textBody));
    body->setText(QString(
        "<b style='color:%1;font-size:13px;'>Cube Controls</b><br>"
        "<b style='color:%1;'>Keyboard shortcuts:</b><br>"
        "• <b style='color:%2;'>J</b> = U (top clockwise) &nbsp; <b style='color:%2;'>F</b> = U' (top counter-clockwise)<br>"
        "• <b style='color:%2;'>S</b> = D (bottom clockwise) &nbsp; <b style='color:%2;'>L</b> = D' (bottom counter-clockwise)<br>"
        "• <b style='color:%2;'>I</b> or <b style='color:%2;'>K</b> = Slice<br>"
        "• <b style='color:%2;'>H</b> = UU &nbsp; <b style='color:%2;'>G</b> = U'U' &nbsp; <b style='color:%2;'>W</b> = DD &nbsp; <b style='color:%2;'>O</b> = D'D'<br>"
        "• <b style='color:%2;'>Z</b> = Undo &nbsp; <b style='color:%2;'>Y</b> = Redo &nbsp; <b style='color:%2;'>Esc</b> = Reset<br><br>"
        "<b style='color:%1;font-size:13px;'>Scramble / Alg Input</b><br>"
        "Type a move sequence in <b style='color:%2;'>(x,y)/</b> format and click <b>Apply</b>.<br>"
        "Toggle the mode button to switch between <b>Scram</b> (applies as-is) and <b>Alg</b> (inverts before applying).<br>"
        "Karnotation names like <b style='color:%2;'>U</b>, <b style='color:%2;'>E</b>, <b style='color:%2;'>bjj</b> etc. are supported.<br><br>"
        "<b style='color:%1;font-size:13px;'>Options</b><br>"
        "• <b>Slice metric</b>: count only slices as moves (instead of layer turns).<br>"
        "• <b>All optimal</b>: find all solutions at the optimal length, not just the first.<br>"
        "• <b>+suboptimal</b>: also find solutions up to N moves longer than optimal.<br>"
        "• <b>Specific depths</b>: search only these move counts (comma-separated).<br>"
        "• <b>Generator alg</b>: output sets up the case; otherwise it solves it.<br>"
        "• <b>Stay in cubeshape</b>: restricts to algs that stay in cubeshape throughout.<br>"
        "• <b>Karnotation output</b>: display solutions using karnotation names.<br>"
        "• <b>Max X / Y / Total</b>: limit how large layer turns can be.<br><br>"
        "<b style='color:%1;font-size:13px;'>Output</b><br>"
        "Solutions appear in the terminal. Use <b>⊞</b> to switch to table view.<br>"
        "Use <b>⤢</b> to expand the terminal to full screen.<br>"
        "Right-click a row in table view to copy the algorithm or the whole row.<br>"
        "If <b>Stay in cubeshape</b> was active, you can enable <b>Roughly rank algs</b> to sort by ergonomics.<br>"
    ).arg(textPrimary, textCyan));

    sa->setWidget(body);
    lay->addWidget(sa, 1);

    card->show();

    auto center = [overlay, card, central](){
        overlay->setGeometry(overlay->parentWidget()->rect());
        int cw = qMin(560, central->width()-60);
        int ch = qMin(520, central->height()-80);
        card->setFixedSize(cw, ch);
        card->move((overlay->width()-card->width())/2, (overlay->height()-card->height())/2);
    };
    center(); card->raise();

    struct F : public QObject {
        QWidget* overlay; QWidget* card; std::function<void()> fn;
        F(QWidget* o, QWidget* c, std::function<void()> f): QObject(o),overlay(o),card(c),fn(f){}
        bool eventFilter(QObject* w, QEvent* e) override {
            if (e->type()==QEvent::Resize && w==overlay->parentWidget()){ fn(); return false; }
            if (e->type()==QEvent::MouseButtonPress && w==overlay){
                if (!card->geometry().contains(static_cast<QMouseEvent*>(e)->pos()))
                    overlay->deleteLater();
                return true;
            }
            return false;
        }
    };
    F* f = new F(overlay, card, center);
    central->installEventFilter(f);
    overlay->installEventFilter(f);
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
