#include "sq1widget.h"
#include "styles/theme.h"
#include <QPainter>
#include <QPolygonF>
#include <QtMath>
#include <QDebug>

Sq1Widget::Sq1Widget(QWidget *parent) : QWidget(parent)
{
    setObjectName("sq1Widget");
    setFixedSize(W, H);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_StyledBackground, true);
    setMouseTracking(true);
    hovered = -1;
    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setInterval(16); // ~60fps
    connect(m_hoverTimer, &QTimer::timeout, this, [this]()
            {
        bool anyActive = false;
        const qreal step = 0.08; // speed of fade
        for (auto it = m_hoverProgress.begin(); it != m_hoverProgress.end(); ++it) {
            qreal target = (it.key() == hovered) ? 1.0 : 0.0;
            qreal &val = it.value();
            if (val < target) { val = qMin(val + step, target); anyActive = true; }
            else if (val > target) { val = qMax(val - step, target); anyActive = true; }
        }
        if (!anyActive) m_hoverTimer->stop();
        update(); });
    reset();
}

bool Sq1Widget::setPositionFromString(const QString &pos)
{
    try
    {
        std::string s = pos.toStdString();
        if (s.size() < 15 || s.size() > 17)
            return false;
        int pi[24] = {};
        int j = 0;
        int nextPartialCorner = -3;
        int nextPartialEdge = 18;
        int parArr[24] = {};
        int pieceCount[16] = {};
        int partialUpCorner = 0, partialDownCorner = 0;
        int partialUpEdge = 0, partialDownEdge = 0;

        // i is string index, j is array index
        for (int i = 0; i < 16 && j < 24; i++)
        {
            int ch = (unsigned char)s[i];
            int k;
            int par = 0;

            if (ch >= 'a' && ch <= 'z')
                ch += ('A' - 'a');

            if (ch >= 'A' && ch <= 'H')
            {
                k = ch - 'A';
                par = 0;
            }
            else if (ch >= '1' && ch <= '8')
            {
                k = ch - ('1' - 8);
                par = 0;
            }
            else if (ch == 'U')
            {
                partialUpCorner++;
                k = nextPartialCorner + 0;
                nextPartialCorner -= 3;
                par = 1;
            }
            else if (ch == 'V')
            {
                partialDownCorner++;
                k = nextPartialCorner + 1;
                nextPartialCorner -= 3;
                par = 1;
            }
            else if (ch == 'W')
            {
                k = nextPartialCorner + 2;
                nextPartialCorner -= 3;
                par = 2;
            }
            else if (ch == 'X')
            {
                partialUpEdge++;
                k = nextPartialEdge + 0;
                nextPartialEdge += 3;
                par = 1;
            }
            else if (ch == 'Y')
            {
                partialDownEdge++;
                k = nextPartialEdge + 1;
                nextPartialEdge += 3;
                par = 1;
            }
            else if (ch == 'Z')
            {
                k = nextPartialEdge + 2;
                nextPartialEdge += 3;
                par = 2;
            }
            else
                return false; // reject unrecognized char

            if (k >= 0 && k <= 15)
            {
                pieceCount[k]++;
                if (pieceCount[k] > 1) // a concrete piece occurring more than once
                    return false;
            }

            if (j >= 24)
                return false; // too many pieces
            pi[j] = k;
            parArr[j] = par;
            j++;

            bool isConcreteCorner = (k >= 0 && k < 8);
            bool isPartialCorner = (k < 0);
            if (isConcreteCorner || isPartialCorner)
            {
                // corners are stored twice in the array
                if (j >= 24)
                    return false; // too many pieces
                pi[j] = k;
                parArr[j] = par;
                j++;
            }
        }

        if (j != 24)
            return false; // too few pieces

        // prevent >4 pieces of the same type from the same layer
        int topCorners = (pieceCount[0] > 0) + (pieceCount[1] > 0) + (pieceCount[2] > 0) + (pieceCount[3] > 0) + partialUpCorner;
        int botCorners = (pieceCount[4] > 0) + (pieceCount[5] > 0) + (pieceCount[6] > 0) + (pieceCount[7] > 0) + partialDownCorner;
        int topEdges = (pieceCount[8] > 0) + (pieceCount[9] > 0) + (pieceCount[10] > 0) + (pieceCount[11] > 0) + partialUpEdge;
        int botEdges = (pieceCount[12] > 0) + (pieceCount[13] > 0) + (pieceCount[14] > 0) + (pieceCount[15] > 0) + partialDownEdge;
        if (topCorners > 4 || botCorners > 4 || topEdges > 4 || botEdges > 4)
            return false;

        // barflip handling
        int mid = 1;
        if (s.size() == 17)
        {
            if (s[16] == '-')
                mid = 1;
            else if (s[16] == '+')
                mid = -1;
            else
                mid = 0;
        }
        else if (s.size() == 16)
        {
            mid = 0;
        }

        // dump
        for (int i = 0; i < 24; i++)
        {
            position[i] = pi[i];
            partiality[i] = parArr[i];
        }
        equator = mid;
        selected = -1;
        update();
        emit positionChanged(); // keep command + solvability (updateConstraints) in sync
        return true;
    }
    catch (...)
    {
        return false;
    }
}

void Sq1Widget::reset()
{
    int defPos[] = {0, 0, 8, 1, 1, 9, 2, 2, 10, 3, 3, 11, 12, 4, 4, 13, 5, 5, 14, 6, 6, 15, 7, 7};
    for (int i = 0; i < 24; i++)
    {
        position[i] = defPos[i];
        partiality[i] = 0;
    }
    equator = 1;
    selected = -1;
    hovered = -1;
    update();
    emit positionChanged();
}

// Convert polar coords to QPointF
QPointF Sq1Widget::polar(QPointF center, double angleDeg, double radius)
{
    double rad = qDegreesToRadians(angleDeg);
    return {center.x() + qCos(rad) * radius, center.y() - qSin(rad) * radius};
}

void Sq1Widget::drawPoly(QPainter &p, QVector<QPointF> pts, QColor fill, qreal hoverT)
{
    p.setBrush(fill);
    p.setPen(QPen(QColor(Theme::cubeBorder()), 1));
    p.drawPolygon(QPolygonF(pts));

    if (hoverT > 0.0)
    {
        QColor overlay(255, 255, 255, static_cast<int>(hoverT * 60));
        p.setBrush(overlay);
        p.setPen(Qt::NoPen);
        p.drawPolygon(QPolygonF(pts));
    }
}

void Sq1Widget::drawSelection(QPainter &p, QVector<QPointF> pts)
{
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(Theme::cubeSelection()), 3));
    p.drawPolygon(QPolygonF(pts));
}

void Sq1Widget::drawLayer(QPainter &p, int start, int end, QPointF center, double startAngle)
{
    double angle = startAngle;
    QVector<QPointF> selPts;

    for (int i = start; i < end;)
    {
        int x = position[i];
        int pi = i;
        qreal hT = m_hoverProgress.value(pi, 0.0);
        if (x < 8)
        {
            QPointF p1 = polar(center, angle, MAIN_LEN);
            QPointF p2 = polar(center, angle - 30, MAIN_LEN / CORNER_FACTOR);
            QPointF p3 = polar(center, angle - 60, MAIN_LEN);
            QPointF p1x = polar(center, angle, MAIN_LEN + SUB_LEN);
            QPointF p2x = polar(center, angle - 30, (MAIN_LEN + SUB_LEN) / CORNER_FACTOR);
            QPointF p3x = polar(center, angle - 60, MAIN_LEN + SUB_LEN);
            drawPoly(p, {center, p1, p2, p3}, partiality[i] > 1 ? colors[6] : colors[(x < 0 ? (x % 3 == 0 ? 0 : 1) : (x < 4 ? 0 : 1))], hT);
            drawPoly(p, {p1, p1x, p2x, p2}, (partiality[i] > 0 || x < 0 || x > 15) ? colors[6] : colors[side_colors[x][0]], hT);
            drawPoly(p, {p2, p2x, p3x, p3}, (partiality[i] > 0 || x < 0 || x > 15) ? colors[6] : colors[side_colors[x][1]], hT);
            if (selected == pi)
                selPts = {center, p1x, p2x, p3x};
            i++;
            angle -= 60;
        }
        else
        {
            QPointF p1 = polar(center, angle, MAIN_LEN);
            QPointF p2 = polar(center, angle - 30, MAIN_LEN);
            QPointF p1x = polar(center, angle, MAIN_LEN + SUB_LEN);
            QPointF p2x = polar(center, angle - 30, MAIN_LEN + SUB_LEN);
            drawPoly(p, {center, p1, p2}, partiality[i] > 1 ? colors[6] : colors[(x > 15 ? (x % 3 == 0 ? 0 : 1) : (x < 12 ? 0 : 1))], hT);
            drawPoly(p, {p1, p1x, p2x, p2}, (partiality[i] > 0 || x < 0 || x > 15) ? colors[6] : colors[side_colors[x][0]], hT);
            if (selected == pi)
                selPts = {center, p1x, p2x};
            angle -= 30;
        }
        i++;
    }
    if (!selPts.isEmpty())
        drawSelection(p, selPts);
}

void Sq1Widget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor bg = palette().color(QPalette::Window);
    if (!bg.isValid())
        bg = QColor("#1a1a2e");
    p.fillRect(rect(), bg);

    drawLayer(p, 0, 12, {TOP_CX, TOP_CY}, -105); // top layer, starts at -105 deg
    drawLayer(p, 12, 24, {BOT_CX, BOT_CY}, 105); // bot layer, starts at 105 deg

    // Equator band
    double r_len = (MAIN_LEN + SUB_LEN);
    double x1 = TOP_CX - r_len * 0.97;
    double x2 = TOP_CX - r_len * 0.28;
    double x3 = TOP_CX + r_len * 0.97;
    double x2b = TOP_CX + r_len * 0.28;
    qreal midT = m_hoverProgress.value(-2, 0.0);
    drawPoly(p, {{x1, MID_TOP}, {x2, MID_TOP}, {x2, MID_BOT}, {x1, MID_BOT}}, colors[2], midT);
    QColor rightColor = equator == 0 ? colors[6] : colors[equator == 1 ? 2 : 4];
    double x_end = (equator != -1) ? x3 : x2b;
    drawPoly(p, {{x2, MID_TOP}, {x_end, MID_TOP}, {x_end, MID_BOT}, {x2, MID_BOT}}, rightColor, midT);
}

// ------- Hit testing -------

int Sq1Widget::hitTestTop(QPointF pt)
{
    // Mirror drawLayer(0,12,{TOP_CX,TOP_CY},-105) exactly, test each piece polygon.
    QPointF center(TOP_CX, TOP_CY);
    double angle = -105.0;
    for (int i = 0; i < 12;)
    {
        int x = position[i];
        int pi = i;
        QPolygonF poly;
        if (x < 8)
        {
            // Corner hull: (center, p1x, p2x, p3x)
            poly << center
                 << polar(center, angle, MAIN_LEN + SUB_LEN)
                 << polar(center, angle - 30, (MAIN_LEN + SUB_LEN) / CORNER_FACTOR)
                 << polar(center, angle - 60, MAIN_LEN + SUB_LEN);
            i++; // skip duplicate slot
            angle -= 60.0;
        }
        else
        {
            // Edge hull: (center, p1x, p2x)
            poly << center
                 << polar(center, angle, MAIN_LEN + SUB_LEN)
                 << polar(center, angle - 30, MAIN_LEN + SUB_LEN);
            angle -= 30.0;
        }
        i++;
        if (poly.containsPoint(pt, Qt::OddEvenFill))
            return pi;
    }
    return -1;
}

int Sq1Widget::hitTestBot(QPointF pt)
{
    // Mirror drawLayer(12,24,{BOT_CX,BOT_CY},105) exactly, test each piece polygon.
    QPointF center(BOT_CX, BOT_CY);
    double angle = 105.0;
    for (int i = 12; i < 24;)
    {
        int x = position[i];
        int pi = i;
        QPolygonF poly;
        if (x < 8)
        {
            poly << center
                 << polar(center, angle, MAIN_LEN + SUB_LEN)
                 << polar(center, angle - 30, (MAIN_LEN + SUB_LEN) / CORNER_FACTOR)
                 << polar(center, angle - 60, MAIN_LEN + SUB_LEN);
            i++;
            angle -= 60.0;
        }
        else
        {
            poly << center
                 << polar(center, angle, MAIN_LEN + SUB_LEN)
                 << polar(center, angle - 30, MAIN_LEN + SUB_LEN);
            angle -= 30.0;
        }
        i++;
        if (poly.containsPoint(pt, Qt::OddEvenFill))
            return pi;
    }
    return -1;
}

void Sq1Widget::mousePressEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pt = event->position();
#else
    QPointF pt = event->localPos();
#endif
    int piece = -1;
    bool isEquator = false;

    if (pt.y() < MID_TOP)
    {
        // Top layer: polygon hit test handles exact containment.
        piece = hitTestTop(pt);
    }
    else if (pt.y() < MID_BOT)
    {
        // Equator band: accept clicks within the drawn rectangle x-extent.
        constexpr double r_len = MAIN_LEN + SUB_LEN;
        double x1 = TOP_CX - r_len * 0.97;
        double x3 = TOP_CX + r_len * 0.97;
        if (pt.x() >= x1 && pt.x() <= x3)
            isEquator = true;
    }
    else
    {
        // Bottom layer: polygon hit test handles exact containment.
        piece = hitTestBot(pt);
    }

    if (isEquator)
    {
        emit userInteracted();
        bool rightClick = (event->button() == Qt::RightButton);
        if (!rightClick)
        {
            // Left click cycles forward: square → kite → gray → square
            if (equator == 1)
                equator = -1;
            else if (equator == -1)
                equator = 0;
            else
                equator = 1;
        }
        else
        {
            // Right click cycles backward: square → gray → kite → square
            if (equator == 1)
                equator = 0;
            else if (equator == 0)
                equator = -1;
            else
                equator = 1;
        }
        emit equatorStateChanged(equator);
    }
    else if (piece >= 0)
    {
        if (event->button() == Qt::RightButton)
        {
            emit userInteracted();
            {
                int x = position[piece];
                if (x < 0 || x > 15)
                {
                    // partial piece: keep partiality in {1,2} — 0 would mean "full" which is inconsistent
                    partiality[piece] = (partiality[piece] == 1) ? 2 : 1;
                }
                else
                {
                    partiality[piece] = (partiality[piece] + 1) % 3;
                }
            }
            if (piece < 23 && position[piece] == position[piece + 1])
                partiality[piece + 1] = partiality[piece];
        }
        else
        {
            if (selected == -1)
            {
                selected = piece;
                // no state change yet — undo pushed when swap completes
            }
            else
            {
                swapSelected(piece);
            }
        }
    }

    update();
    emit positionChanged();
}

void Sq1Widget::mouseMoveEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pt = event->position();
#else
    QPointF pt = event->localPos();
#endif
    int hoveredPiece = -1;
    bool overPiece = false;

    if (pt.y() < MID_TOP)
    {
        // Top layer: polygon hit test handles exact containment.
        hoveredPiece = hitTestTop(pt);
        overPiece = (hoveredPiece >= 0);
    }
    else if (pt.y() < MID_BOT)
    {
        // Equator band: accept clicks within the drawn rectangle x-extent.
        constexpr double r_len = MAIN_LEN + SUB_LEN;
        double x1 = TOP_CX - r_len * 0.97;
        double x3 = TOP_CX + r_len * 0.97;
        if (pt.x() >= x1 && pt.x() <= x3)
        {
            overPiece = true;
            // Equator band is one clickable element, but not a "piece" - use a sentinel value
            hoveredPiece = -2; // Special value: hovering equator, not a real piece index
        }
    }
    else
    {
        // Bottom layer: polygon hit test handles exact containment.
        hoveredPiece = hitTestBot(pt);
        overPiece = (hoveredPiece >= 0);
    }

    // Update cursor
    setCursor(overPiece ? Qt::PointingHandCursor : Qt::ArrowCursor);

    // Update hover state and repaint if changed
    if (hovered != hoveredPiece)
    {
        // Ensure both old and new piece have entries in the map
        if (hovered >= -2)
            m_hoverProgress.insert(hovered, m_hoverProgress.value(hovered, 0.0));
        if (hoveredPiece >= -2)
            m_hoverProgress.insert(hoveredPiece, m_hoverProgress.value(hoveredPiece, 0.0));
        hovered = hoveredPiece;
        m_hoverTimer->start();
    }
}

void Sq1Widget::leaveEvent(QEvent *)
{
    // Clear hover state when mouse leaves the widget
    if (hovered != -1)
    {
        hovered = -1;
        update();
    }
    setCursor(Qt::ArrowCursor);
}

void Sq1Widget::swapSelected(int piece)
{
    bool selCorner = (selected < 23 && position[selected] == position[selected + 1]);
    bool pieCorner = (piece < 23 && position[piece] == position[piece + 1]);
    if (selCorner != pieCorner)
    {
        selected = piece;
        update();
        return;
    }
    if (selected == piece)
    {
        selected = -1;
        update();
        return;
    }
    emit userInteracted();

    if (selCorner)
    {
        std::swap(position[selected + 1], position[piece + 1]);
        std::swap(partiality[selected + 1], partiality[piece + 1]);
    }
    std::swap(position[selected], position[piece]);
    std::swap(partiality[selected], partiality[piece]);
    selected = -1;
}

bool Sq1Widget::isSliceable()
{
    return position[0] != position[11] && position[5] != position[6] &&
           position[12] != position[23] && position[17] != position[18];
}

void Sq1Widget::rotateTopRaw(int twelfths)
{
    twelfths = ((twelfths % 12) + 12) % 12;
    for (int n = 0; n < twelfths; n++)
    {
        int c = position[11];
        int d = partiality[11];
        for (int i = 11; i > 0; i--)
        {
            position[i] = position[i - 1];
            partiality[i] = partiality[i - 1];
        }
        position[0] = c;
        partiality[0] = d;
    }
}

void Sq1Widget::rotateBotRaw(int twelfths)
{
    twelfths = ((twelfths % 12) + 12) % 12;
    for (int n = 0; n < twelfths; n++)
    {
        int c = position[23];
        int d = partiality[23];
        for (int i = 23; i > 12; i--)
        {
            position[i] = position[i - 1];
            partiality[i] = partiality[i - 1];
        }
        position[12] = c;
        partiality[12] = d;
    }
}

bool Sq1Widget::applyMoves(const QVector<MoveStep> &moves)
{
    selected = -1;
    for (const MoveStep &mv : moves)
    {
        if (mv.isSlice)
        {
            if (!isSliceable())
                return false;
            for (int i = 6; i < 12; i++)
            {
                std::swap(position[i], position[i + 6]);
                std::swap(partiality[i], partiality[i + 6]);
            }
            equator = -equator;
        }
        else
        {
            rotateTopRaw(mv.x);
            rotateBotRaw(mv.y);
        }
    }
    update();
    emit positionChanged();
    return isSliceable();
}

void Sq1Widget::doU()
{
    selected = -1;
    for (int moves = 0; moves < 12; moves++)
    {
        int c = position[11];
        int d = partiality[11];
        for (int i = 11; i > 0; i--)
        {
            position[i] = position[i - 1];
            partiality[i] = partiality[i - 1];
        }
        position[0] = c;
        partiality[0] = d;
        if (isSliceable())
            break;
    }
    update();
    emit positionChanged();
}

void Sq1Widget::doUPrime()
{
    selected = -1;
    for (int moves = 0; moves < 12; moves++)
    {
        int c = position[0];
        int d = partiality[0];
        for (int i = 0; i < 11; i++)
        {
            position[i] = position[i + 1];
            partiality[i] = partiality[i + 1];
        }
        position[11] = c;
        partiality[11] = d;
        if (isSliceable())
            break;
    }
    update();
    emit positionChanged();
}

void Sq1Widget::doD()
{
    selected = -1;
    for (int moves = 0; moves < 12; moves++)
    {
        int c = position[23];
        int d = partiality[23];
        for (int i = 23; i > 12; i--)
        {
            position[i] = position[i - 1];
            partiality[i] = partiality[i - 1];
        }
        position[12] = c;
        partiality[12] = d;
        if (isSliceable())
            break;
    }
    update();
    emit positionChanged();
}

void Sq1Widget::doDPrime()
{
    selected = -1;
    for (int moves = 0; moves < 12; moves++)
    {
        int c = position[12];
        int d = partiality[12];
        for (int i = 12; i < 23; i++)
        {
            position[i] = position[i + 1];
            partiality[i] = partiality[i + 1];
        }
        position[23] = c;
        partiality[23] = d;
        if (isSliceable())
            break;
    }
    update();
    emit positionChanged();
}

void Sq1Widget::doSlice()
{
    selected = -1;
    if (!isSliceable())
        return;
    for (int i = 6; i < 12; i++)
    {
        std::swap(position[i], position[i + 6]);
        std::swap(partiality[i], partiality[i + 6]);
    }
    equator = -equator;
    update();
    emit positionChanged();
}

void Sq1Widget::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_I:
    case Qt::Key_K:
        doSlice();
        break;
    case Qt::Key_J:
        doU();
        break;
    case Qt::Key_F:
        doUPrime();
        break;
    case Qt::Key_S:
        doD();
        break;
    case Qt::Key_L:
        doDPrime();
        break;
    case Qt::Key_Escape:
        reset();
        break;
    default:
        QWidget::keyPressEvent(e);
    }
}

// Produce the canonical FullPosition-style encoding, honoring partiality[]:
//   concrete pieces            -> 0..15
//   partial corner U/V/W       -> negative, value%3 == 0 / -2 / -1
//   partial edge   X/Y/Z       -> >15,      value%3 ==  0 /  1 /  2
// Each partial gets a distinct value (so the solver treats two "any" pieces as
// two independent unknowns rather than the specific pieces they happen to store).
// Corner/edge is decided by x<8 (covers concrete 0-7 and already-encoded partial
// corners <0), and the home layer mirrors the draw code's classification.
Sq1Widget::RawState Sq1Widget::getRawState() const
{
    RawState s;
    int nextPartialCorner = -3;
    int nextPartialEdge = 18;
    for (int i = 0; i < 24; i++)
    {
        int x = position[i];
        int pi = partiality[i];
        bool isCorner = (x < 8);
        int val;
        if (pi == 0)
        {
            val = x; // concrete (or an already-encoded partial)
        }
        else if (isCorner)
        {
            bool top = (x < 0) ? (x % 3 == 0) : (x < 4); // top-home corner?
            val = nextPartialCorner + (pi == 2 ? 2 : (top ? 0 : 1)); // W / U / V
            nextPartialCorner -= 3;
        }
        else
        {
            bool top = (x > 15) ? (x % 3 == 0) : (x < 12); // top-home edge?
            val = nextPartialEdge + (pi == 2 ? 2 : (top ? 0 : 1)); // Z / X / Y
            nextPartialEdge += 3;
        }
        s.pos[i] = val;
        if (isCorner)
        {
            i++; // corners occupy two slots, stored with the same value
            if (i < 24)
                s.pos[i] = val;
        }
    }
    s.middle = equator;
    return s;
}

QString Sq1Widget::getPositionString()
{
    RawState s = getRawState();
    QString out;
    for (int i = 0; i < 24; i++)
    {
        int x = s.pos[i];
        if (x >= 0 && x <= 15)
            out += QString("ABCDEFGH12345678")[x];
        else if (x < 0)
            out += (x % 3 == 0 ? 'U' : (x % 3 == -2 ? 'V' : 'W')); // partial corner
        else
            out += (x % 3 == 0 ? 'X' : (x % 3 == 1 ? 'Y' : 'Z'));  // partial edge
        if (x < 8)
            i++; // skip duplicate corner slot
    }
    if (s.middle != 0)
        out += (s.middle == 1 ? '-' : '/');
    return out;
}
