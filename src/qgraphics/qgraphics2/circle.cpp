#include "circle.h"
#include "drag_circle.h"

#include <QtGui>
#include <cmath>

namespace qgraph {

Circle::Circle(QGraphicsScene* scene)
{
    setFlags(ItemIsMovable);
    _radius = 20;
    setRect(-_radius, -_radius, _radius * 2, _radius * 2);
    setPos(_radius + 10, _radius + 10);

    setPenColor(QColor(255, 0, 0));
    setPenWidth(2);
    setZLevel(5);
    scene->addItem(this);

    _circle = new DragCircle(scene);
    _circle->setParent(this);
    _circle->setVisible(false);
    _circle->setPos(rect().width() / 2, 0);
}

void Circle::setFrameScale(float newScale)
{
    float s = newScale / frameScale();
    QRectF r = rect();
    float x = r.x() * s;
    float y = r.y() * s;
    float w = r.width() * s;
    float h = r.height() * s;

    _radius *= s;

    prepareGeometryChange();
    setRect(x, y, w, h);
    setPos(pos().x() * s, pos().y() * s);

    _circle->setPos(w / 2, 0);
    _frameScale = newScale;
    //changeSignal.emit_(this);
}

void Circle::dragCircleMove(DragCircle* circle)
{
    QPointF center = rect().center();
    float deltaX = qAbs(circle->x() - center.x());
    float deltaY = qAbs(circle->y() - center.y());

    _radius = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    QRectF r;
    r.setLeft(center.x() - _radius);
    r.setTop (center.y() - _radius);
    r.setWidth (_radius * 2);
    r.setHeight(_radius * 2);

    prepareGeometryChange();
    setRect(r);

    circleMoveSignal.emit_(this);
}

void Circle::dragCircleRelease(DragCircle* circle)
{
    if (circle->parentItem() == this)
    {
        QRectF r = rect();
        circle->setPos(r.width() + r.x(), r.width() / 2 + r.y());
        circleReleaseSignal.emit_(this);
    }
}

QPointF Circle::realCenter() const
{
    float s = frameScale();
    QPointF center = rect().center();
    QPointF center2 {(pos().x() + center.x()) / s,
                     (pos().y() + center.y()) / s};
    return center2;
}

void Circle::setRealCenter(const QPointF& val)
{
    float s = frameScale();
    QPointF center = rect().center();
    QPointF pos;
    pos.setX(val.x() * s - center.x());
    pos.setY(val.y() * s - center.y());

    prepareGeometryChange();
    setPos(pos.x(), pos.y());

    _circle->setPos(rect().width() / 2, 0);
}

int Circle::realRadius() const
{
    return _radius / frameScale();
}

void Circle::setRealRadius(int val)
{
    float s = frameScale();
    _radius = val * s;

    QPointF center = rect().center();

    QRectF r;
    r.setLeft(center.x() - _radius);
    r.setTop (center.y() - _radius);
    r.setWidth (_radius * 2);
    r.setHeight(_radius * 2);

    prepareGeometryChange();
    setRect(r);

    _circle->setPos(rect().width() / 2, 0);
}

} // namespace qgraph
