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
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <string>
#include <map>

// ============================================================
// Ergonomics Rating — pure C++ translation of alg_rater.html
// Uses KARNOTATION from karnotation.h for unkarnify.
// ============================================================

// Split string by delimiter
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
    // check if top%3==0 (aligned)
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
    return (it != MOVE_VALUES.end()) ? it->second : 5; // default mediocre
}

// Split string by delimiter
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

// Trim whitespace
static std::string trimStr(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

// replaceAll helper
static std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.size(), to);
        pos += to.size();
    }
    return str;
}

// addCommasToMove: given a purely-numeric token (digits and minus signs only), insert a
// comma to produce a "top,bot" pair.  Tokens that already contain a comma or non-numeric
// characters are returned unchanged.  Mirrors the JavaScript addCommas() helper.
static std::string addCommasToMove(const std::string& move) {
    if (move.empty()) return move;
    for (char c : move)
        if (c != '-' && !std::isdigit((unsigned char)c)) return move; // already has comma or non-numeric
    switch (move.size()) {
        case 1: return move + ",0";
        case 2: return move[0] == '-' ? move + ",0"
                                      : std::string(1, move[0]) + "," + std::string(1, move[1]);
        case 3: return move[0] == '-' ? move.substr(0,2) + "," + std::string(1, move[2])
                                      : std::string(1, move[0]) + "," + move.substr(1);
        case 4: return move.substr(0,2) + "," + move.substr(2);
        default: return move; // unexpected length — pass through unchanged
    }
}

// unkarnify: convert a space-separated karnotation alg to slash-separated numeric tuples.
// Uses KARNOTATION from karnotation.h: for each space-separated token, find the matching
// KARNOTATION[i][0] (trimmed) and replace with KARNOTATION[i][1].
// Searches in reverse order so longer/more-specific entries (U3', U3, U2, U') match before U.
// After expansion, decodes sq1opt's single-char shorthands: &=-1, ^=-2, 9=-3, 8=-4, 7=-5.
static std::string unkarnify(const std::string& algIn) {
    // If the alg begins with '/' (leading slice, restored by sq1opt change #1), strip it
    // before tokenising so the fused "/token" (e.g. "/e") doesn't prevent KARNOTATION lookup.
    // The slash is reattached to the final result so rateAlg sees the correct structure.
    bool startsWithSlice = (!algIn.empty() && algIn[0] == '/');
    const std::string& algWork = startsWithSlice ? algIn.substr(1) : algIn;

    // Split karn alg into space-separated tokens
    std::vector<std::string> tokens;
    {
        std::istringstream iss(algWork);
        std::string t;
        while (iss >> t) tokens.push_back(t);
    }

    // For each token, look up in KARNOTATION in reverse order (avoids "U" eating "U'")
    std::vector<std::string> numericParts;
    for (const auto& tok : tokens) {
        bool found = false;
        for (int k = KARNOTATION_LEN - 1; k >= 0; k--) {
            const std::string& kname = KARNOTATION[k][0];
            const std::string& kval  = KARNOTATION[k][1];
            if (kname.empty()) continue;
            if (tok == trimStr(kname)) {
                numericParts.push_back(kval);
                found = true;
                break;
            }
        }
        if (!found) numericParts.push_back(tok); // already numeric or unknown
    }

    // Join parts: each part may be "3,0" or "3,0/9,0/" (multi-move expansion).
    // Strip any trailing "/" from individual parts before joining with "/".
    std::string result;
    for (auto part : numericParts) {
        while (!part.empty() && part.back() == '/') part.pop_back();
        if (!result.empty()) result += "/";
        result += part;
    }

    // Decode sq1opt single-char negative shorthands (applied after karnify in printsol):
    //   & = -1,  ^ = -2,  9 = -3,  8 = -4,  7 = -5
    result = replaceAll(result, "&", "-1");
    result = replaceAll(result, "^", "-2");
    result = replaceAll(result, "9", "-3");
    result = replaceAll(result, "8", "-4");
    result = replaceAll(result, "7", "-5");

    // Insert missing commas into any slash-separated part that is purely numeric
    // (e.g. "30" → "3,0", "-36" → "-3,6").  Parts already formatted as "top,bot" pass through.
    auto parts = splitStr(result, '/');
    result.clear();
    for (const auto& part : parts) {
        if (!result.empty()) result += "/";
        result += addCommasToMove(part);
    }

    // Reattach the leading slash that was stripped before tokenising.
    if (startsWithSlice) result = "/" + result;

    return result;
}

// get_overwork from alg_rater.html
static std::pair<int,int> getOverwork(const std::vector<std::string>& moves) {
    std::vector<int> top, bot;
    for (auto& m : moves) {
        auto c = m.find(',');
        if (c == std::string::npos) { top.push_back(0); bot.push_back(0); continue; }
        try { top.push_back(std::stoi(m.substr(0,c))); } catch(...) { top.push_back(0); }
        try { bot.push_back(std::stoi(m.substr(c+1))); } catch(...) { bot.push_back(0); }
    }

    int movement = 0, bonus = 0;
    // top: count left streaks (top < 0, or top == 6)
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
    // bot: count right streaks (bot > 0)
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
    // bonus: consecutive non-zero pairs
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

// initial_top_A: true when the top-layer seam slot holds an edge (digit) in the starting
// position hex — i.e. the layer is in the "A-aligned" state where an edge faces the seam.
// Determined by the caller from the position hex before solving.
static AlgRating rateAlg(const std::string& algRaw, bool initial_top_A,
                         double W1, double W2, double W3, double W4, double W5)
{
    // Parse alg directly: unkarnify if karnotation, then split by "/".
    // No legalMove canonicalization — rate the alg as-is.
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
        // Always keep r[0] even if empty: an empty first element means the alg begins with a
        // slice (AUF = 0,0).  Filtering it out shifts all subsequent indices by one, corrupting
        // alignment tracking through the rest of the alg.  Trailing empties are still dropped.
        if (i == 0 || !pt.empty()) r.push_back(pt);
    }
    if (r.size() < 2) return {W4, ""}; // degenerate

    // r has N elements where r[0] is the AUF, r[1..N-2] are real moves, r[N-1] is the sentinel.
    // In alg_rater.html: sliceCount = count of "/" = N-1, and r (after pop) has N-1 elements.
    // Our r still has the sentinel as last element, so: sliceCount = r.size() - 1.
    int sliceCount = (int)r.size() - 1;
    if (sliceCount <= 0) return {W4, ""};

    // Phase 1: ergonomics — iterate r[0..N-2] only (exclude sentinel r[N-1])
    double ergo_up = 0, ergo_down = 0;
    bool is_top_A = false, odd_slice = true;
    for (int i = 0; i < (int)r.size() - 1; i++) {
        if (i == 0) {
            // Seed is_top_A from the actual starting position, then XOR with the AUF's
            // top value — exactly the same logic used for subsequent moves.  This replaces
            // the old approach of deriving alignment solely from the AUF value, which was
            // wrong whenever the starting position had a corner (not an edge) at the seam.
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
    if ((std::abs(ergo_up - ergo_down) / sliceCount) > 5) {
        sliceStart = (ergo_up > ergo_down) ? "/" : "\\";
    } else sliceStart = " ";

    // Phase 2: penalize slice count
    double PHASE2 = W2 * sliceCount;

    // Phase 3: penalize overwork — exclude r[0] (AUF) and r[N-1] (sentinel)
    auto moves = std::vector<std::string>(r.begin() + 1, r.end() - 1);
    auto [movement, bonus] = getOverwork(moves);
    double PHASE3 = W3 * movement / sliceCount;
    double PHASE4 = bonus * W5 / sliceCount;

    double FINAL = PHASE1 - PHASE2 - PHASE3 + PHASE4 + W4;
    return {FINAL, sliceStart};
}

// Rate a list of solution lines and return them sorted highest→lowest.
// Returns pairs of {original_line, score}.
// posHex is the position string captured at solve time (e.g. "A1B2C3D45E6F7G8H-").
// The first character of posHex is the piece at the top-layer seam slot:
//   letter (A-H / U-W) = corner,  digit (1-8) / X-Z = edge.
// A digit (or partial-edge letter X-Z) means the top layer starts in the A-aligned state.
static std::vector<std::pair<QString, double>>
rateAndSort(const QStringList& solutionLines, const QString& posHex, bool useKarnotation) {
    // Determine initial top-layer alignment from the position hex.
    // Letters A-H (corners) or U/V/W (partial corners) = corner at seam  → is_top_A = false.
    // Digits 1-8 (edges)   or X/Y/Z  (partial edges)   = edge at seam   → is_top_A = true.
    bool initial_top_A = false;
    if (!posHex.isEmpty()) {
        QChar first = posHex[0];
        initial_top_A = first.isDigit() ||
                        first == 'X' || first == 'Y' || first == 'Z';
    }

    const double W1=34, W2=100, W3=38, W4=500, W5=10;
    std::vector<std::pair<QString, double>> results;
    for (QString line : solutionLines) {
        // The alg string is the part before the [...] annotation
        std::string algStr = line.toStdString();
        // strip the [tw|tu] part for rating but keep original line for display
        auto bracket = algStr.find('[');
        std::string algOnly = bracket != std::string::npos
                                  ? trimStr(algStr.substr(0, bracket))
                                  : trimStr(algStr);
        double score = W4;
        try {
            auto rating = rateAlg(algOnly, initial_top_A, W1, W2, W3, W4, W5);
            score = rating.FINAL;
            QString sliceStr = QString::fromStdString(rating.sliceStart);
            if (!useKarnotation) {
                // Non-karnotation: replace the first "/" with sliceStart
                int slash_pos = line.indexOf('/');
                if (slash_pos >= 0)
                    line.replace(slash_pos, 1, sliceStr);
            } else if (line.startsWith('/')) {
                // Karnotation + solution starts with a slice (leading "/" from change #1):
                // replace it with "\" only when sliceStart is "\"; if " ", leave it alone.
                if (sliceStr == "\\")
                    line.replace(0, 1, QString("\\"));
            } else {
                // Karnotation + solution doesn't start with a slice:
                // replace the first space with sliceStart.
                int space_pos = line.indexOf(' ');
                if (space_pos >= 0)
                    line.replace(space_pos, 1, sliceStr);
            }
        } catch (...) {}
        results.push_back({line, score});
    }
    std::sort(results.begin(), results.end(),
              [](const auto& a, const auto& b){ return a.second > b.second; });
    return results;
}

// -------------------------------------------------------
// SolverWorker
// -------------------------------------------------------
void SolverWorker::run() {
    QString exePath = QCoreApplication::applicationDirPath() + "/sq1opt";
#ifdef Q_OS_WIN
    exePath += ".exe";
#endif
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels); // merge stderr into stdout
    proc.setWorkingDirectory(QCoreApplication::applicationDirPath());
    QStringList args;
    args << "-v5";
    args.append(flags);
    args << positionStr;
    proc.start(exePath, args);
    if (!proc.waitForStarted(3000)) {
        emit lineReady("ERROR: Could not start sq1opt. Make sure sq1opt is in the same folder.");
        emit finished(-1);
        return;
    }

    // Read using a manual line buffer so we never block on canReadLine().
    // waitForReadyRead(200) returns true when data arrives, false on timeout.
    // We exit only once it returns false AND the process has already finished.
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
    // Flush anything left in the pipe after the process exits
    buf += proc.readAll();
    drainLines();
    // Emit any final partial line without a trailing newline
    buf = buf.trimmed();
    if (!buf.isEmpty()) emit lineReady(QString::fromUtf8(buf));

    proc.waitForFinished(1000);
    emit finished(proc.exitCode());
}

// -------------------------------------------------------
// MainWindow
// -------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Square-1 Optimizer");
    setMinimumSize(720, 560);
    resize(860, 700);
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
    QWidget* leftContainer = new QWidget();
    leftContainer->setFixedWidth(300);
    QVBoxLayout* leftCol = new QVBoxLayout(leftContainer);
    leftCol->setContentsMargins(0, 0, 0, 0);
    leftCol->setSpacing(4);
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
    leftCol->addStretch();

    QScrollArea* leftScroll = new QScrollArea();
    leftScroll->setWidget(leftContainer);
    leftScroll->setWidgetResizable(false);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setFixedWidth(320);

    connect(btnU,     &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_J,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnUP,    &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_F,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnSlice, &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_I,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnD,     &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_S,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnDP,    &QPushButton::clicked, cubeWidget, [this]{ cubeWidget->setFocus(); QKeyEvent e(QEvent::KeyPress,Qt::Key_L,Qt::NoModifier); QApplication::sendEvent(cubeWidget,&e); });
    connect(btnReset, &QPushButton::clicked, cubeWidget, &Sq1Widget::reset);
    connect(btnReset, &QPushButton::clicked, this, &MainWindow::onReset);

    root->addWidget(leftScroll);

    // ---- RIGHT: options + output ----
    QVBoxLayout* rightCol = new QVBoxLayout();
    rightCol->setSpacing(6);

    QGroupBox* grpOptions = new QGroupBox("Options");
    QGridLayout* grid = new QGridLayout(grpOptions);
    grid->setVerticalSpacing(4);

    // ── Widgets ──────────────────────────────────────────────────────────────
    chkTwist = new QCheckBox("Twist metric");
    chkTwist->setToolTip("Count only slices (twists) as moves, not layer turns.\n"
                         "Maps to the -w flag.");

    chkAllOptimal = new QCheckBox("All optimal");
    chkAllOptimal->setToolTip("Find every optimal solution, not just the first one.\n"
                              "The spinner sets how many extra moves beyond optimal to also find.\n"
                              "0 = optimal only.  Maps to -a or -a<n>.");
    spnSuboptimal = new QSpinBox();
    spnSuboptimal->setRange(0, 9);
    spnSuboptimal->setValue(0);
    spnSuboptimal->setFixedWidth(48);
    spnSuboptimal->setToolTip("Extra moves beyond optimal (0 = optimal only).\n"
                              "Hidden when 'Specific depths' is active (depth pins the search\n"
                              "depth explicitly, making an extra-move count meaningless).");

    // Container for the All-optimal row so we can control row height and keep
    // "+suboptimal:" flush against the spinbox with a stretch on the left.
    QWidget* allOptRow = new QWidget();
    allOptRow->setMinimumHeight(30);
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
    chkDepths->setToolTip("Search only the listed twist/turn depths instead of iterating from 0.\n"
                          "Comma-separated, e.g. 8,9.  Maps to -d<list>.\n"
                          "When active, the suboptimal-count field is hidden and -a is used\n"
                          "without a number (extra-move count has no effect with fixed depths).");
    txtDepths = new QLineEdit();
    txtDepths->setFixedWidth(80);
    txtDepths->setPlaceholderText("e.g. 8,9");
    txtDepths->setToolTip("Comma-separated list of depths to search.");

    chkGenerator = new QCheckBox("Generator alg");
    chkGenerator->setToolTip("Treat input as a generating sequence (forward) rather than a\n"
                             "solution sequence (reverse).  Maps to -g.");

    chk2gen = new QCheckBox("2Gen  (top layer + slices only)");
    chk2gen->setToolTip("Restrict to 2-generator moves: top-layer turns and slices only.\n"
                        "Requires pieces G, 8, H to already be solved.\n"
                        "Incompatible with 'Stay in cubeshape' and 'Pseudo 2Gen'.\n"
                        "Maps to -2.");

    chkPseudo2gen = new QCheckBox("Pseudo 2Gen  (bottom: ±1 only)");
    chkPseudo2gen->setToolTip("Restrict bottom-layer turns to ±1 only (pseudo-2-generator).\n"
                              "Incompatible with '2Gen'.\n"
                              "Maps to -p.");

    chkCubeshape = new QCheckBox("Stay in cubeshape");
    chkCubeshape->setToolTip("Only generate algs that keep the puzzle in a square/square\n"
                             "(cubeshape) throughout.  Incompatible with '2Gen'.\n"
                             "Maps to -c.");

    chkIgnoreMid = new QCheckBox("Ignore middle layer");
    chkIgnoreMid->setToolTip("Treat the middle layer orientation as irrelevant.\n"
                             "Useful when you don't care whether the middle ends as square or kite.\n"
                             "Maps to -m.");

    chkKarnotation = new QCheckBox("Karnotation output");
    chkKarnotation->setToolTip("Display solutions in Karnotation (named move notation)\n"
                               "instead of raw (x,y)/ tuples.  Maps to -k.");

    chkMaxX = new QCheckBox("Max top turn:");
    chkMaxX->setToolTip("Limit the maximum top-layer turn size (0–6 twelfths).\n"
                        "Enabling this also disables the transformation equivalence (-x).\n"
                        "Maps to -X<n>.");
    spnMaxX = new QSpinBox();
    spnMaxX->setRange(0, 6);
    spnMaxX->setValue(3);
    spnMaxX->setFixedWidth(48);
    spnMaxX->setToolTip("Maximum top-layer turn in twelfths of a full rotation (0–6).");

    chkMaxY = new QCheckBox("Max bottom turn:");
    chkMaxY->setToolTip("Limit the maximum bottom-layer turn size (0–6 twelfths).\n"
                        "Maps to -Y<n>.");
    spnMaxY = new QSpinBox();
    spnMaxY->setRange(0, 6);
    spnMaxY->setValue(3);
    spnMaxY->setFixedWidth(48);
    spnMaxY->setToolTip("Maximum bottom-layer turn in twelfths of a full rotation (0–6).");

    chkMaxTotal = new QCheckBox("Max total turn:");
    chkMaxTotal->setToolTip("Limit the combined |top|+|bottom| turn per move pair (1–12).\n"
                            "Maps to -Z<n>.");
    spnMaxTotal = new QSpinBox();
    spnMaxTotal->setRange(1, 12);
    spnMaxTotal->setValue(6);
    spnMaxTotal->setFixedWidth(48);
    spnMaxTotal->setToolTip("Maximum combined turn amount per move pair (1–12).");

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
    grid->addWidget(chkMaxX,       row,   0); grid->addWidget(spnMaxX,    row++, 1);
    grid->addWidget(chkMaxY,       row,   0); grid->addWidget(spnMaxY,    row++, 1);
    grid->addWidget(chkMaxTotal,   row,   0); grid->addWidget(spnMaxTotal,row++, 1);

    // ── Connections ──────────────────────────────────────────────────────────
    // updateConstraints fires first (may hide/disable widgets), then updateCommand rebuilds
    // the command string from the final valid state.
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
    connect(chkMaxX,       &QCheckBox::toggled, this, upd);
    connect(spnMaxX,       QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxY,       &QCheckBox::toggled, this, upd);
    connect(spnMaxY,       QOverload<int>::of(&QSpinBox::valueChanged), this, upd);
    connect(chkMaxTotal,   &QCheckBox::toggled, this, upd);
    connect(spnMaxTotal,   QOverload<int>::of(&QSpinBox::valueChanged), this, upd);

    rightCol->addWidget(grpOptions);

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

    btnSolve = new QPushButton("▶  Solve");
    btnSolve->setObjectName("btnSolve");
    btnSolve->setFixedHeight(38);
    rightCol->addWidget(btnSolve);

    progressBar = new QProgressBar();
    progressBar->setRange(0,0);
    progressBar->setVisible(false);
    progressBar->setFixedHeight(6);
    rightCol->addWidget(progressBar);

    QLabel* lblOut = new QLabel("Results:");
    rightCol->addWidget(lblOut);
    txtOutput = new QTextEdit();
    txtOutput->setReadOnly(true);
    txtOutput->setObjectName("txtOutput");
    txtOutput->setMinimumHeight(120);
    rightCol->addWidget(txtOutput, 1);

    chkRankErgo = new QCheckBox("Rank based on approximate ergonomics");
    chkRankErgo->setVisible(false);
    chkRankErgo->setObjectName("chkRankErgo");
    rightCol->addWidget(chkRankErgo);

    lblStatus = new QLabel("Ready.");
    lblStatus->setObjectName("lblStatus");
    rightCol->addWidget(lblStatus);

    root->addLayout(rightCol, 1);

    connect(btnSolve,    &QPushButton::clicked,  this, &MainWindow::onSolve);
    connect(btnCopy,     &QPushButton::clicked,  this, &MainWindow::onCopy);
    connect(chkRankErgo, &QCheckBox::toggled,    this, &MainWindow::onRankErgoToggled);

    // Apply initial constraint state
    updateConstraints();
}

// Enforce option incompatibilities and enable/disable dependent fields.
// Called before updateCommand() so the command is always built from valid state.
void MainWindow::updateConstraints() {
    const bool is2gen    = chk2gen->isChecked();
    const bool isPseudo  = chkPseudo2gen->isChecked();
    const bool isAllOpt  = chkAllOptimal->isChecked();
    const bool isDepths  = chkDepths->isChecked();

    // Helper: disable a checkbox and silently uncheck it if currently checked.
    // blockSignals prevents updateConstraints from being re-entered.
    auto disableCheck = [](QCheckBox* cb) {
        cb->setEnabled(false);
        if (cb->isChecked()) {
            cb->blockSignals(true);
            cb->setChecked(false);
            cb->blockSignals(false);
        }
    };

    // ── 2Gen × Cubeshape: sq1opt error 18 ────────────────────────────────────
    // 2gen requires the bottom layer to be fixed; cubeshape solves both layers.
    if (is2gen)   disableCheck(chkCubeshape);
    else          chkCubeshape->setEnabled(true);

    if (chkCubeshape->isChecked()) disableCheck(chk2gen);
    else if (!is2gen)              chk2gen->setEnabled(true);

    // ── 2Gen × Pseudo2Gen: same twoGen variable, last writer wins ────────────
    // Both set twoGen; having both active is meaningless. Disable the other.
    if (is2gen)  disableCheck(chkPseudo2gen);
    else         chkPseudo2gen->setEnabled(true);

    if (isPseudo) disableCheck(chk2gen);
    else if (!chkCubeshape->isChecked()) chk2gen->setEnabled(true);

    // ── Specific depths × suboptimal count ───────────────────────────────────
    // When a fixed depth is set, extraMoves (the +N number) is meaningless:
    // sq1opt simply searches those exact depths without comparing to optimal.
    // Hide the spinbox so the user can't enter a misleading value; emit -a only.
    spnSuboptimal->setVisible(isAllOpt && !isDepths);
    if (QLabel* lbl = findChild<QLabel*>("lblSuboptLabel"))
        lbl->setVisible(isAllOpt && !isDepths);

    // Dependent text field: only editable when its checkbox is active
    txtDepths->setEnabled(isDepths);

    // Max-turn spinboxes: only meaningful when their checkbox is active
    spnMaxX->setEnabled(chkMaxX->isChecked());
    spnMaxY->setEnabled(chkMaxY->isChecked());
    spnMaxTotal->setEnabled(chkMaxTotal->isChecked());
}

void MainWindow::buildStyles() {
    setStyleSheet(R"(
        QMainWindow, QWidget { background: #1a1a2e; color: #e0e0e0; font-family: 'Segoe UI', Arial; font-size: 13px; }
        QGroupBox { border: 1px solid #444; border-radius: 6px; margin-top: 8px; padding-top: 8px; color: #aaa; }
        QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }
        QCheckBox { spacing: 6px; }
        QCheckBox::indicator { width:14px; height:14px; border-radius:3px; border:1px solid #666; background:#2a2a3e; }
        QCheckBox::indicator:checked { background: #4a90d9; border-color: #4a90d9; }
        QCheckBox#chkRankErgo::indicator:checked { background: #d97a4a; border-color: #d97a4a; }
        QCheckBox:disabled { color: #555; }
        QCheckBox::indicator:disabled { border-color: #3a3a4e; background: #1e1e30; }
        QLineEdit { background: #2a2a3e; border: 1px solid #555; border-radius: 4px; padding: 3px 6px; color: #fff; }
        QLineEdit:disabled { color: #555; background: #1e1e30; border-color: #3a3a4e; }
        QLineEdit#txtCommand { font-family: monospace; color: #7fdbff; font-size: 12px; }
        QSpinBox { background: #2a2a3e; border: 1px solid #555; border-radius: 4px;
                   padding: 2px 4px; color: #fff; }
        QSpinBox:disabled { color: #555; background: #1e1e30; border-color: #3a3a4e; }
        QSpinBox::up-button, QSpinBox::down-button {
            background: #3a3a5e; border: none; border-radius: 2px; width: 14px; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: #4a4a7e; }
        QSpinBox::up-arrow  { image: none; border-left: 4px solid transparent;
                              border-right: 4px solid transparent;
                              border-bottom: 5px solid #aaa; width: 0; height: 0; }
        QSpinBox::down-arrow { image: none; border-left: 4px solid transparent;
                               border-right: 4px solid transparent;
                               border-top: 5px solid #aaa; width: 0; height: 0; }
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
        QScrollBar:vertical { background: #0d1117; width: 12px; border-radius: 6px; margin: 0; }
        QScrollBar::handle:vertical { background: #4a4a6e; border-radius: 6px; min-height: 24px; }
        QScrollBar::handle:vertical:hover { background: #6a6aae; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; border: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }
    )");
}

QStringList MainWindow::buildArgList() {
    QStringList args;

    if (chkTwist->isChecked()) args << "-w";

    // -a / -a<n>: when specific depths are active, emit -a only (the +N extra-moves
    // count is meaningless when the search depth is pinned by -d).
    if (chkAllOptimal->isChecked()) {
        const bool useNumber = !chkDepths->isChecked() && spnSuboptimal->value() > 0;
        args << (useNumber ? QString("-a%1").arg(spnSuboptimal->value()) : QString("-a"));
    }

    if (chkDepths->isChecked()) {
        QString dv = txtDepths->text().trimmed().remove(' ');
        if (!dv.isEmpty()) args << QString("-d%1").arg(dv);
    }

    if (chkGenerator->isChecked())  args << "-g";

    // 2gen / pseudo-2gen: updateConstraints() ensures at most one is checked.
    if (chk2gen->isChecked())       args << "-2";
    if (chkPseudo2gen->isChecked()) args << "-p";

    if (chkCubeshape->isChecked())  args << "-c";
    if (chkIgnoreMid->isChecked())  args << "-m";
    if (chkKarnotation->isChecked())args << "-k";

    // Max-turn limits: spinboxes are disabled (not hidden) when their checkbox is off,
    // so we only need to check isChecked().
    if (chkMaxX->isChecked())     args << QString("-X%1").arg(spnMaxX->value());
    if (chkMaxY->isChecked())     args << QString("-Y%1").arg(spnMaxY->value());
    if (chkMaxTotal->isChecked()) args << QString("-Z%1").arg(spnMaxTotal->value());

    return args;
}

void MainWindow::updateCommand() {
    QString pos = cubeWidget->getPositionString();
    QStringList args = buildArgList();
    txtCommand->setText("sq1opt " + args.join(" ") + " " + pos);
}

void MainWindow::onSolve() {
    if (worker && worker->isRunning()) return;
    txtOutput->clear();
    m_rawLines.clear();
    m_solutionLines.clear();
    m_seenSolutions.clear();
    chkRankErgo->setVisible(false);
    chkRankErgo->setChecked(false);
    lblStatus->setText("Solving...");
    btnSolve->setEnabled(false);
    progressBar->setVisible(true);

    worker = new SolverWorker();
    worker->positionStr = cubeWidget->getPositionString();
    m_posHex = worker->positionStr;   // capture alignment for rating, locked to this solve
    worker->flags = buildArgList();
    connect(worker, &SolverWorker::lineReady, this, &MainWindow::onSolverLine, Qt::QueuedConnection);
    connect(worker, &SolverWorker::finished,  this, &MainWindow::onSolverDone, Qt::QueuedConnection);
    // QPointer<SolverWorker> makes deleteLater safe: when the object is freed, `worker`
    // auto-becomes null regardless of event-loop ordering, preventing crashes on re-solve.
    connect(worker, &SolverWorker::finished,  worker, &QObject::deleteLater);
    worker->start();
}

void MainWindow::onSolverLine(QString line) {
    bool isSolution = line.contains('[') && line.contains(']');
    if (isSolution) {
        // Deduplicate on the alg text before the "[tw|tu]" annotation.
        int bracketPos = line.indexOf('[');
        QString algKey = (bracketPos >= 0) ? line.left(bracketPos).trimmed() : line.trimmed();
        if (m_seenSolutions.contains(algKey)) return;
        m_seenSolutions.insert(algKey);
    }
    m_rawLines.append(line);
    if (isSolution) {
        m_solutionLines.append(line);
        txtOutput->append("<span style='color:#00ff88;font-weight:bold;'>" +
                          line.toHtmlEscaped() + "</span>");
    } else {
        txtOutput->append("<span style='color:#888;'>" + line.toHtmlEscaped() + "</span>");
    }
}

void MainWindow::onSolverDone(int code) {
    progressBar->setVisible(false);
    btnSolve->setEnabled(true);
    lblStatus->setText(code == 0 ? "Done." : "Error (code " + QString::number(code) + ")");
    if (chkCubeshape->isChecked() && !m_solutionLines.isEmpty()) {
        chkRankErgo->setVisible(true);
    }
    // worker lifetime is managed by the deleteLater connection in onSolve.
    // QPointer auto-nulls `worker` once the object is destroyed, so repeated
    // calls to onSolve see nullptr and never touch freed memory.
}

void MainWindow::onReset() {
    txtOutput->clear();
    m_rawLines.clear();
    m_solutionLines.clear();
    m_seenSolutions.clear();
    chkRankErgo->setVisible(false);
    chkRankErgo->setChecked(false);
    lblStatus->setText("Ready.");
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    // All input fields are number-only (spinboxes, digit/comma line edits, read-only outputs),
    // so letter keys are never needed for text entry — always route them to the cube.
    auto sendCube = [this](Qt::Key k) {
        QKeyEvent e(QEvent::KeyPress, k, Qt::NoModifier);
        QApplication::sendEvent(cubeWidget, &e);
    };

    switch (event->key()) {
    // Single-move shortcuts — forwarded straight to cubeWidget.
    case Qt::Key_I: case Qt::Key_K:
    case Qt::Key_J: case Qt::Key_F:
    case Qt::Key_S: case Qt::Key_L:
    case Qt::Key_Escape:
        sendCube(static_cast<Qt::Key>(event->key()));
        break;
    // Double-move shortcuts.
    case Qt::Key_H: sendCube(Qt::Key_J); sendCube(Qt::Key_J); break; // h = U U  (jj)
    case Qt::Key_G: sendCube(Qt::Key_F); sendCube(Qt::Key_F); break; // g = U'U' (ff)
    case Qt::Key_O: sendCube(Qt::Key_L); sendCube(Qt::Key_L); break; // o = D'D' (ll)
    case Qt::Key_W: sendCube(Qt::Key_S); sendCube(Qt::Key_S); break; // w = D D  (ss)
    default:
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::onCopy() {
    QApplication::clipboard()->setText(txtCommand->text());
    lblStatus->setText("Copied to clipboard!");
}

void MainWindow::onRankErgoToggled(bool checked) {
    txtOutput->clear();
    if (!checked) {
        // Restore original output order
        for (const QString& line : m_rawLines) {
            bool isSol = line.contains('[') && line.contains(']');
            if (isSol)
                txtOutput->append("<span style='color:#00ff88;font-weight:bold;'>" + line.toHtmlEscaped() + "</span>");
            else
                txtOutput->append("<span style='color:#888;'>" + line.toHtmlEscaped() + "</span>");
        }
        lblStatus->setText("Done.");
        return;
    }
    if (m_solutionLines.isEmpty()) return;
    lblStatus->setText("Rating algorithms...");
    auto rated = rateAndSort(m_solutionLines, m_posHex, chkKarnotation->isChecked());
    for (auto& [origLine, score] : rated) {
        QString ranked = QString("%1  (%2)").arg(origLine).arg(score, 0, 'f', 2);
        txtOutput->append("<span style='color:#00ff88;font-weight:bold;'>" +
                          ranked.toHtmlEscaped() + "</span>");
    }
    lblStatus->setText(QString("Ranked %1 algs by ergonomics.").arg((int)rated.size()));
}
