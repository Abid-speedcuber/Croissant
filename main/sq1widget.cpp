#include "sq1widget.h"
#include "styles/theme.h"
#include <QPainter>
#include <QPolygonF>
#include <QtMath>
#include <QDebug>

Sq1Widget::Sq1Widget(QWidget* parent) : QWidget(parent) {
    setObjectName("sq1Widget");
    setFixedSize(W, H);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_StyledBackground, true);
    setMouseTracking(true);
    hovered = -1;
    m_hoverTimer = new QTimer(this);
    m_hoverTimer->setInterval(16); // ~60fps
    connect(m_hoverTimer, &QTimer::timeout, this, [this]() {
        bool anyActive = false;
        const qreal step = 0.08; // speed of fade
        for (auto it = m_hoverProgress.begin(); it != m_hoverProgress.end(); ++it) {
            qreal target = (it.key() == hovered) ? 1.0 : 0.0;
            qreal &val = it.value();
            if (val < target) { val = qMin(val + step, target); anyActive = true; }
            else if (val > target) { val = qMax(val - step, target); anyActive = true; }
        }
        if (!anyActive) m_hoverTimer->stop();
        update();
    });
    reset();
}

bool Sq1Widget::setPositionFromString(const QString& pos) {
    try {
        std::string s = pos.toStdString();
        if (s.size() < 15 || s.size() > 17) return false;
        int pi[24] = {};
        int j = 0;
        int nextPartialCorner = -3;
        int nextPartialEdge = 18;
        int parArr[24] = {};
        int pieceCount[16] = {};

        for (int i = 0; i < 16 && j < 24; i++) {
            int k = (unsigned char)s[i];
            int par = 0;

            // Uppercase only
            if (k >= 'a' && k <= 'z') k += ('A' - 'a');

            if      (k >= 'A' && k <= 'H') { k -= 'A'; par = 0; }
            else if (k >= '1' && k <= '8') { k -= ('1' - 8); par = 0; }
            else if (k == 'U') { k = nextPartialCorner; nextPartialCorner -= 3; par = 1; }
            else if (k == 'V') { k = nextPartialCorner; nextPartialCorner -= 3; par = 1; }
            else if (k == 'W') { k = nextPartialCorner; nextPartialCorner -= 3; par = 2; }
            else if (k == 'X') { k = nextPartialEdge;   nextPartialEdge += 3;  par = 1; }
            else if (k == 'Y') { k = nextPartialEdge;   nextPartialEdge += 3;  par = 1; }
            else if (k == 'Z') { k = nextPartialEdge;   nextPartialEdge += 3;  par = 2; }
            else return false; // reject any unrecognised character immediately

            // Bounds check: concrete piece indices must be 0-15
            if (k >= 0 && k <= 15) {
                pieceCount[k]++;
                if (pieceCount[k] > 2) return false; // impossible piece count
            }

            // Guard j bounds before writing
            if (j >= 24) return false;
            pi[j] = k; parArr[j] = par; j++;

            // Corner occupies two slots
            bool isConcreteCorner = (k >= 0 && k < 8);
            bool isPartialCorner  = (k < 0 && (k % 3 == 0 || k % 3 == -2)); // U or V type
            if (isConcreteCorner || isPartialCorner) {
                if (j >= 24) return false;
                pi[j] = k; parArr[j] = par; j++;
            }
        }

        if (j != 24) return false;

        // Each concrete piece may appear at most once (corners appear twice in array = once as piece)
        for (int i = 0; i < 8; i++)  if (pieceCount[i] > 2) return false; // corners stored twice
        for (int i = 8; i < 16; i++) if (pieceCount[i] > 1) return false; // edges stored once

        int mid = 0, mid_par = 0;
        if (s.size() == 17) {
            if      (s[16] == '-') { mid = 0; mid_par = 0; }
            else if (s[16] == '+') { mid = 1; mid_par = 0; }
            else                   { mid = 0; mid_par = 1; }
        } else if (s.size() == 16) { mid = 0; mid_par = 1; } // ABCDEFGH12345678 sets partial bar

        // All checks passed — commit to state
        for (int i = 0; i < 24; i++) { position[i] = pi[i]; partiality[i] = parArr[i]; }
        equator = mid; equator_partial = mid_par; selected = -1;
        update();
        return true;

    } catch (...) {
        return false; // never crash the UI
    }
}

void Sq1Widget::reset() {
    int defPos[] = {0,0,8,1,1,9,2,2,10,3,3,11,12,4,4,13,5,5,14,6,6,15,7,7};
    for(int i=0;i<24;i++) { position[i]=defPos[i]; partiality[i]=0; }
    equator=0; equator_partial=0; selected=-1; hovered=-1;
    update();
    emit positionChanged();
}

// Convert polar coords to QPointF
QPointF Sq1Widget::polar(QPointF center, double angleDeg, double radius) {
    double rad = qDegreesToRadians(angleDeg);
    return { center.x() + qCos(rad)*radius, center.y() - qSin(rad)*radius };
}

void Sq1Widget::drawPoly(QPainter& p, QVector<QPointF> pts, QColor fill, qreal hoverT) {
    p.setBrush(fill);
    p.setPen(QPen(QColor(Theme::cubeBorder()), 1));
    p.drawPolygon(QPolygonF(pts));

    if (hoverT > 0.0) {
        QColor overlay(255, 255, 255, static_cast<int>(hoverT * 60));
        p.setBrush(overlay);
        p.setPen(Qt::NoPen);
        p.drawPolygon(QPolygonF(pts));
    }
}

void Sq1Widget::drawSelection(QPainter& p, QVector<QPointF> pts) {
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(Theme::cubeSelection()), 3));
    p.drawPolygon(QPolygonF(pts));
}

void Sq1Widget::drawLayer(QPainter& p, int start, int end, QPointF center, double startAngle) {
    double angle = startAngle;
    QVector<QPointF> selPts;

    for(int i=start; i<end; ) {
        int x = position[i];
        int pi = i;
        qreal hT = m_hoverProgress.value(pi, 0.0);
        if(x < 8) {
            QPointF p1 = polar(center, angle,          MAIN_LEN);
            QPointF p2 = polar(center, angle-30,       MAIN_LEN/CORNER_FACTOR);
            QPointF p3 = polar(center, angle-60,       MAIN_LEN);
            QPointF p1x= polar(center, angle,          MAIN_LEN+SUB_LEN);
            QPointF p2x= polar(center, angle-30,       (MAIN_LEN+SUB_LEN)/CORNER_FACTOR);
            QPointF p3x= polar(center, angle-60,       MAIN_LEN+SUB_LEN);
            drawPoly(p, {center, p1, p2, p3}, partiality[i]>1 ? colors[6] : colors[x<4?0:1], hT);
            drawPoly(p, {p1, p1x, p2x, p2}, partiality[i]>0 ? colors[6] : colors[side_colors[x][0]], hT);
            drawPoly(p, {p2, p2x, p3x, p3}, partiality[i]>0 ? colors[6] : colors[side_colors[x][1]], hT);
            if(selected == pi) selPts = {center, p1x, p2x, p3x};
            i++;
            angle -= 60;
        } else {
            QPointF p1 = polar(center, angle,    MAIN_LEN);
            QPointF p2 = polar(center, angle-30, MAIN_LEN);
            QPointF p1x= polar(center, angle,    MAIN_LEN+SUB_LEN);
            QPointF p2x= polar(center, angle-30, MAIN_LEN+SUB_LEN);
            drawPoly(p, {center, p1, p2}, partiality[i]>1 ? colors[6] : colors[x<12?0:1], hT);
            drawPoly(p, {p1, p1x, p2x, p2}, partiality[i]>0 ? colors[6] : colors[side_colors[x][0]], hT);
            if(selected == pi) selPts = {center, p1x, p2x};
            angle -= 30;
        }
        i++;
    }
    if (!selPts.isEmpty()) drawSelection(p, selPts);
}

void Sq1Widget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QColor bg = palette().color(QPalette::Window);
    if (!bg.isValid()) bg = QColor("#1a1a2e");
    p.fillRect(rect(), bg);

    drawLayer(p, 0,  12, {TOP_CX, TOP_CY}, -105);  // top layer, starts at -105 deg
    drawLayer(p, 12, 24, {BOT_CX, BOT_CY},  105);  // bot layer, starts at 105 deg

    // Equator band
    // x1..x2 = thin left strip (always red).
    // Square: x2..x3 = wide right strip (red). Kite: x2..x2b = narrow right strip (orange), rest black.
    double r_len = (MAIN_LEN + SUB_LEN);
    double x1  = TOP_CX - r_len * 0.97;
    double x2  = TOP_CX - r_len * 0.28;   // split: left strip is narrow
    double x3  = TOP_CX + r_len * 0.97;
    double x2b = TOP_CX + r_len * 0.28;   // kite end: mirrors left strip width
    qreal midT = m_hoverProgress.value(-2, 0.0);
    drawPoly(p, {{x1,MID_TOP},{x2,MID_TOP},{x2,MID_BOT},{x1,MID_BOT}}, colors[2], midT);
    QColor rightColor = equator_partial > 0 ? colors[6] : colors[equator == 0 ? 2 : 4];
    double x_end = (equator == 0 || equator_partial > 0) ? x3 : x2b;
    drawPoly(p, {{x2,MID_TOP},{x_end,MID_TOP},{x_end,MID_BOT},{x2,MID_BOT}}, rightColor, midT);
}

// ------- Hit testing -------

int Sq1Widget::hitTestTop(QPointF pt) {
    // Mirror drawLayer(0,12,{TOP_CX,TOP_CY},-105) exactly, test each piece polygon.
    QPointF center(TOP_CX, TOP_CY);
    double angle = -105.0;
    for (int i = 0; i < 12; ) {
        int x  = position[i];
        int pi = i;
        QPolygonF poly;
        if (x < 8) {
            // Corner hull: (center, p1x, p2x, p3x)
            poly << center
                 << polar(center, angle,    MAIN_LEN + SUB_LEN)
                 << polar(center, angle-30, (MAIN_LEN + SUB_LEN) / CORNER_FACTOR)
                 << polar(center, angle-60, MAIN_LEN + SUB_LEN);
            i++;          // skip duplicate slot
            angle -= 60.0;
        } else {
            // Edge hull: (center, p1x, p2x)
            poly << center
                 << polar(center, angle,    MAIN_LEN + SUB_LEN)
                 << polar(center, angle-30, MAIN_LEN + SUB_LEN);
            angle -= 30.0;
        }
        i++;
        if (poly.containsPoint(pt, Qt::OddEvenFill)) return pi;
    }
    return -1;
}

int Sq1Widget::hitTestBot(QPointF pt) {
    // Mirror drawLayer(12,24,{BOT_CX,BOT_CY},105) exactly, test each piece polygon.
    QPointF center(BOT_CX, BOT_CY);
    double angle = 105.0;
    for (int i = 12; i < 24; ) {
        int x  = position[i];
        int pi = i;
        QPolygonF poly;
        if (x < 8) {
            poly << center
                 << polar(center, angle,    MAIN_LEN + SUB_LEN)
                 << polar(center, angle-30, (MAIN_LEN + SUB_LEN) / CORNER_FACTOR)
                 << polar(center, angle-60, MAIN_LEN + SUB_LEN);
            i++;
            angle -= 60.0;
        } else {
            poly << center
                 << polar(center, angle,    MAIN_LEN + SUB_LEN)
                 << polar(center, angle-30, MAIN_LEN + SUB_LEN);
            angle -= 30.0;
        }
        i++;
        if (poly.containsPoint(pt, Qt::OddEvenFill)) return pi;
    }
    return -1;
}

void Sq1Widget::mousePressEvent(QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pt = event->position();
#else
    QPointF pt = event->localPos();
#endif
    int piece = -1;
    bool isEquator = false;

    if (pt.y() < MID_TOP) {
        // Top layer: polygon hit test handles exact containment.
        piece = hitTestTop(pt);
    } else if (pt.y() < MID_BOT) {
        // Equator band: accept clicks within the drawn rectangle x-extent.
        constexpr double r_len = MAIN_LEN + SUB_LEN;
        double x1 = TOP_CX - r_len * 0.97;
        double x3 = TOP_CX + r_len * 0.97;
        if (pt.x() >= x1 && pt.x() <= x3)
            isEquator = true;
    } else {
        // Bottom layer: polygon hit test handles exact containment.
        piece = hitTestBot(pt);
    }

    if(isEquator) {
        emit userInteracted();
        bool rightClick = (event->button() == Qt::RightButton);
        if (!rightClick) {
            // Left click cycles forward: square → kite → gray → square
            if (equator_partial == 0 && equator == 0) {
                equator = 1;
            } else if (equator_partial == 0 && equator == 1) {
                equator_partial = 1;
            } else {
                equator = 0;
                equator_partial = 0;
            }
        } else {
            // Right click cycles backward: square → gray → kite → square
            if (equator_partial == 0 && equator == 0) {
                equator_partial = 1;
            } else if (equator_partial > 0) {
                equator_partial = 0;
                equator = 1;
            } else {
                equator = 0;
            }
        }
        emit equatorStateChanged(equator_partial > 0 ? 2 : equator);
    } else if(piece >= 0) {
        if(event->button() == Qt::RightButton) {
            emit userInteracted();
            partiality[piece] = (partiality[piece]+1)%3;
            if(piece<23 && position[piece]==position[piece+1])
                partiality[piece+1] = partiality[piece];
        } else {
            if(selected == -1) {
                selected = piece;
                // no state change yet — undo pushed when swap completes
            } else {
                swapSelected(piece);
            }
        }
    }

    update();
    emit positionChanged();
}

void Sq1Widget::mouseMoveEvent(QMouseEvent* event) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPointF pt = event->position();
#else
    QPointF pt = event->localPos();
#endif
    int hoveredPiece = -1;
    bool overPiece = false;

    if (pt.y() < MID_TOP) {
        // Top layer: polygon hit test handles exact containment.
        hoveredPiece = hitTestTop(pt);
        overPiece = (hoveredPiece >= 0);
    } else if (pt.y() < MID_BOT) {
        // Equator band: accept clicks within the drawn rectangle x-extent.
        constexpr double r_len = MAIN_LEN + SUB_LEN;
        double x1 = TOP_CX - r_len * 0.97;
        double x3 = TOP_CX + r_len * 0.97;
        if (pt.x() >= x1 && pt.x() <= x3) {
            overPiece = true;
            // Equator band is one clickable element, but not a "piece" - use a sentinel value
            hoveredPiece = -2;  // Special value: hovering equator, not a real piece index
        }
    } else {
        // Bottom layer: polygon hit test handles exact containment.
        hoveredPiece = hitTestBot(pt);
        overPiece = (hoveredPiece >= 0);
    }

    // Update cursor
    setCursor(overPiece ? Qt::PointingHandCursor : Qt::ArrowCursor);

    // Update hover state and repaint if changed
    if (hovered != hoveredPiece) {
        // Ensure both old and new piece have entries in the map
        if (hovered >= -2) m_hoverProgress.insert(hovered, m_hoverProgress.value(hovered, 0.0));
        if (hoveredPiece >= -2) m_hoverProgress.insert(hoveredPiece, m_hoverProgress.value(hoveredPiece, 0.0));
        hovered = hoveredPiece;
        m_hoverTimer->start();
    }
}

void Sq1Widget::leaveEvent(QEvent*) {
    // Clear hover state when mouse leaves the widget
    if (hovered != -1) {
        hovered = -1;
        update();
    }
    setCursor(Qt::ArrowCursor);
}

void Sq1Widget::swapSelected(int piece) {
    bool selCorner = (selected<23 && position[selected]==position[selected+1]);
    bool pieCorner = (piece<23   && position[piece]  ==position[piece+1]);
    if(selCorner != pieCorner) { selected=piece; update(); return; }
    if(selected == piece) { selected=-1; update(); return; }
    emit userInteracted();

    if(selCorner) {
        std::swap(position[selected+1],   position[piece+1]);
        std::swap(partiality[selected+1], partiality[piece+1]);
    }
    std::swap(position[selected],   position[piece]);
    std::swap(partiality[selected], partiality[piece]);
    selected = -1;
}

bool Sq1Widget::isSliceable() {
    return position[0]!=position[11] && position[5]!=position[6] &&
           position[12]!=position[23] && position[17]!=position[18];
}

void Sq1Widget::doU() {
    selected = -1;
    for(int moves=0;moves<12;moves++) {
        int c=position[11]; int d=partiality[11];
        for(int i=11;i>0;i--) { position[i]=position[i-1]; partiality[i]=partiality[i-1]; }
        position[0]=c; partiality[0]=d;
        if(isSliceable()) break;
    }
    update(); emit positionChanged();
}

void Sq1Widget::doUPrime() {
    selected = -1;
    for(int moves=0;moves<12;moves++) {
        int c=position[0]; int d=partiality[0];
        for(int i=0;i<11;i++) { position[i]=position[i+1]; partiality[i]=partiality[i+1]; }
        position[11]=c; partiality[11]=d;
        if(isSliceable()) break;
    }
    update(); emit positionChanged();
}

void Sq1Widget::doD() {
    selected = -1;
    for(int moves=0;moves<12;moves++) {
        int c=position[23]; int d=partiality[23];
        for(int i=23;i>12;i--) { position[i]=position[i-1]; partiality[i]=partiality[i-1]; }
        position[12]=c; partiality[12]=d;
        if(isSliceable()) break;
    }
    update(); emit positionChanged();
}

void Sq1Widget::doDPrime() {
    selected = -1;
    for(int moves=0;moves<12;moves++) {
        int c=position[12]; int d=partiality[12];
        for(int i=12;i<23;i++) { position[i]=position[i+1]; partiality[i]=partiality[i+1]; }
        position[23]=c; partiality[23]=d;
        if(isSliceable()) break;
    }
    update(); emit positionChanged();
}

void Sq1Widget::doSlice() {
    selected = -1;
    if(!isSliceable()) return;
    for(int i=6;i<12;i++) {
        std::swap(position[i],   position[i+6]);
        std::swap(partiality[i], partiality[i+6]);
    }
    equator = 1 - equator;
    update(); emit positionChanged();
}

void Sq1Widget::keyPressEvent(QKeyEvent* e) {
    switch(e->key()) {
    case Qt::Key_I: case Qt::Key_K: doSlice(); break;
    case Qt::Key_J: doU(); break;
    case Qt::Key_F: doUPrime(); break;
    case Qt::Key_S: doD(); break;
    case Qt::Key_L: doDPrime(); break;
    case Qt::Key_Escape: reset(); break;
    default: QWidget::keyPressEvent(e);
    }
}

QString Sq1Widget::getPositionString() {
    QString out;
    for(int i=0;i<24;i++) {
        int pi = partiality[i];
        int x  = position[i];
        if(pi==0) {
            out += QString("ABCDEFGH12345678")[x];
        } else if(pi==1) {
            if(x<4) out+='U'; else if(x<8) out+='V';
            else if(x<12) out+='X'; else out+='Y';
        } else {
            out += (x<8 ? 'W' : 'Z');
        }
        if(x<8) i++; // skip duplicate corner slot
    }
    if(equator_partial==0) out += (equator==0 ? '-' : '/');
    return out;
}
