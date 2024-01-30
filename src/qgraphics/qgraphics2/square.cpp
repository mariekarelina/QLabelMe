#include "square.h"
#include "drag_circle.h"
//#include "virtual_line.h"
#include <QtGui>

namespace qgraph {

Square::Square(QGraphicsScene* scene)
{
    setFlags(ItemIsMovable);
    setPenColor(QColor(0, 255, 0));
    setPenWidth(2);

    setZLevel(4);
    scene->addItem(this);

    _circle = new DragCircle(scene);
    _circle->setParent(this);
    _circle->setVisible(false);
}

void Square::setFrameScale(float newScale)
{
    float s = newScale / frameScale();
    QRectF r;
    r.setLeft(0);
    r.setTop (0);
    r.setWidth (rect().width()  * s);
    r.setHeight(rect().height() * s);

    prepareGeometryChange();
    setRect(r);
    setPos(pos().x() * s, pos().y() * s);
    _circle->setPos(r.width(), r.height());

    _frameScale = newScale;
    //changeSignal.emit_(this);
}

void Square::dragCircleMove(DragCircle* circle)
{
    float w = qMax(circle->x(), circle->y());
    QRectF r = rect();
    r.setWidth(w);
    r.setHeight(w);

    prepareGeometryChange();
    setRect(r);

    circleMoveSignal.emit_(this);
}

void Square::dragCircleRelease(DragCircle* circle)
{
    if (circle->parentItem() == this)
    {
        circle->setPos(rect().width(), rect().height());
        circleReleaseSignal.emit_(this);
    }
}

QRectF Square::realSceneRect() const
{
    float s = frameScale();
    QRectF r;
    r.setLeft(pos().x() / s);
    r.setTop (pos().y() / s);
    r.setWidth (rect().width()  / s);
    r.setHeight(rect().height() / s);
    return r;
}

void Square::setRealSceneRect(const QRectF& r)
{
    float s = frameScale();
    QPointF pos;
    pos.setX(r.left() * s);
    pos.setY(r.top()  * s);

    QRectF rect;
    rect.setLeft(0);
    rect.setTop(0);
    rect.setWidth (r.width()  * s);
    rect.setHeight(r.height() * s);

    prepareGeometryChange();
    setRect(rect);
    setPos(pos.x(), pos.y());
    _circle->setPos(rect.width(), rect.height());
}

} // namespace qgraph
