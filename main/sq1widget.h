#pragma once
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <array>
#include <functional>
#include <QTimer>
#include <QMap>
#include "styles/theme.h"

// -------------------------------------------------------
// Sq1Widget: draws the Square-1 cube, handles clicks/keys
// Mirrors the canvas logic in helper.html exactly
// -------------------------------------------------------
class Sq1Widget : public QWidget {
    Q_OBJECT
public:
    explicit Sq1Widget(QWidget* parent = nullptr);

    // Called by MainWindow to get position string for the solver
    QString getPositionString();

    void reset();
    int  getEquatorState() const { return equator_partial > 0 ? 2 : equator; }
    bool inCubeshape() const {
        // Each half (0-11, 12-23) must have alternating corner/edge slots.
        // Corners occupy 2 slots (same value <8 twice), edges occupy 1 slot.
        // Cubeshape = 4 corners + 4 edges per layer in alternating pattern.
        for (int base = 0; base < 24; base += 12) {
            // Try all 3 rotations of the pattern: edge at offset 0, 1, or 2 (mod 3)
            bool ok = false;
            for (int rem = 0; rem < 3; rem++) {
                bool match = true;
                for (int i = 0; i < 12; i++) {
                    bool expectEdge = (i % 3 == rem);
                    bool isEdge = (position[base + i] >= 8);
                    if (isEdge != expectEdge) { match = false; break; }
                }
                if (match) { ok = true; break; }
            }
            if (!ok) return false;
        }
        return true;
    }
    void setEquatorState(int state) {
        if (state == 2) { equator_partial = 1; }
        else { equator_partial = 0; equator = state; }
        update();
        emit positionChanged();
    }
    // Parse a position string and update the cube display
    bool setPositionFromString(const QString& pos);

    struct MoveStep { bool isSlice; int x; int y; };
    // Apply a sequence of pre-parsed moves to the current state.
    // Works correctly whether or not the state contains partial pieces.
    // Returns false if a slice move is attempted on an unsliceable position.
    bool applyMoves(const QVector<MoveStep>& moves);

signals:
    void positionChanged(); // emitted whenever cube state changes
    void userInteracted();  // emitted on mouse-driven piece swap/state change (for undo)
    void equatorStateChanged(int state); // 0=square, 1=kite, 2=gray
    void cubeshapeChanged(bool inCS);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void keyPressEvent(QKeyEvent*) override;

private:
    // --- State (mirrors helper.html JS variables) ---
    std::array<int,24> position;
    std::array<int,24> partiality;  // 0=full, 1=top/bottom, 2=any
    int equator;           // 0 = square, 1 = kite
    int equator_partial;   // 0 or 1
    int selected;         // index of selected piece, or -1
    int hovered;          // index of hovered piece, or -1
    QMap<int, qreal> m_hoverProgress; // 0.0 = no hover, 1.0 = full hover
    QTimer* m_hoverTimer{nullptr};

    // --- Drawing constants ---
    static constexpr int W = 300, H = 500;
    static constexpr int TOP_CX = 150, TOP_CY = 125;
    static constexpr int BOT_CX = 150, BOT_CY = 375;
    static constexpr int MAIN_LEN = 50, SUB_LEN = 25;
    static constexpr double CORNER_FACTOR = 0.73205080756;
    static constexpr int MID_TOP = 235, MID_BOT = 265;

    // Colors: 0=darkgrey(top face), 1=white(bot face), 2=red, 3=blue, 4=orange, 5=green, 6=grey(partial)
    QColor colors[7] = {
        QColor(Theme::cubeTopFace()),
        QColor(Theme::cubeBotFace()),
        QColor(Theme::cubeRed()),
        QColor(Theme::cubeBlue()),
        QColor(Theme::cubeOrange()),
        QColor(Theme::cubeGreen()),
        QColor(Theme::cubePartial())
    };

    // side_colors[piece] = which color indices to use for side faces
    int side_colors[16][2] = {
        {2,3},{3,4},{4,5},{5,2},{2,5},{5,4},{4,3},{3,2}, // corners 0-7
        {3},{4},{5},{2},{2},{5},{4},{3}                   // edges 8-15
    };

    // --- Helpers ---
    QPointF polar(QPointF center, double angleDeg, double radius);
    void drawPoly(QPainter& p, QVector<QPointF> pts, QColor fill, qreal hoverT = 0.0);
    void drawSelection(QPainter& p, QVector<QPointF> pts);

    void drawLayer(QPainter& p, int start, int end, QPointF center, double startAngle);

    bool isSliceable();
    void doU();
    void doUPrime();
    void doD();
    void doDPrime();
    void doSlice();
    void rotateTopRaw(int twelfths); // raw rotation, no emit, no sliceability stop
    void rotateBotRaw(int twelfths);
    void swapSelected(int piece);

    int hitTestTop(QPointF pt);
    int hitTestBot(QPointF pt);
};
