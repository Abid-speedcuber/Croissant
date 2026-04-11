#pragma once
#include <QWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <array>
#include <functional>
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

    // Reset to solved state
    void reset();
    // Parse a position string and update the cube display
    bool setPositionFromString(const QString& pos);

signals:
    void positionChanged(); // emitted whenever cube state changes
    void userInteracted();  // emitted on mouse-driven piece swap/state change (for undo)

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
    int middle;           // 0 = square, 1 = kite
    int middle_partial;   // 0 or 1
    int selected;         // index of selected piece, or -1
    int hovered;          // index of hovered piece, or -1

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
    void drawPoly(QPainter& p, QVector<QPointF> pts, QColor fill, bool isHovered = false);
    void drawSelection(QPainter& p, QVector<QPointF> pts);

    void drawLayer(QPainter& p, int start, int end, QPointF center, double startAngle);

    bool isSliceable();
    void doU();
    void doUPrime();
    void doD();
    void doDPrime();
    void doSlice();
    void swapSelected(int piece);

    int hitTestTop(QPointF pt);
    int hitTestBot(QPointF pt);
};
