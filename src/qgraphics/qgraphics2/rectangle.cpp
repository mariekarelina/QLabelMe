#include "rectangle.h"
#include "drag_circle.h"
#include <QtGui>

namespace qgraph {

Rectangle::Rectangle(QGraphicsScene* scene)
{
    setFlags(ItemIsMovable);
    QPen pen = this->pen();
    pen.setColor(QColor(0, 255, 0));
    pen.setWidth(2);
    setPen(pen);

    setZLevel(1);
    scene->addItem(this);

    _circleTL = new DragCircle(scene);
    _circleTL->setParent(this);
    _circleTL->setVisible(false);

    _circleTR = new DragCircle(scene);
    _circleTR->setParent(this);
    _circleTR->setVisible(false);

    _circleBR = new DragCircle(scene);
    _circleBR->setParent(this);
    _circleBR->setVisible(false);
}

void Rectangle::setFrameScale(float newScale)
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

    _circleTL->setPos(0, 0);
    _circleTR->setPos(r.width(), 0);
    _circleBR->setPos(r.width(), r.height());

    _frameScale = newScale;
    //changeSignal.emit_(this);
}

void Rectangle::dragCircleMove(DragCircle* circle)
{
    QRectF r = rect();

    if (_circleTL == circle)
    {
        float x = circle->x();
        float y = circle->y();
        r.setTopLeft(QPoint(x, y));

        _circleTR->setPos(x, 0);
    }
    else if (_circleTR == circle)
    {
        float x = circle->x();
        float y = circle->y();
        r.setTopRight(QPoint(x, y));
    }
    else if (_circleBR == circle)
    {
        //r.setWidth(circle->x());
        //r.setHeight(circle->y());
        r.setBottomRight(QPoint(circle->x(), circle->y()));
    }

    prepareGeometryChange();
    setRect(r);

    circleMoveSignal.emit_(this);
}

void Rectangle::dragCircleRelease(DragCircle* circle)
{
    if (circle->parentItem() == this)
    {
        //circle->setPos(rect().width(), rect().height());
        circleReleaseSignal.emit_(this);
    }
}

QRectF Rectangle::realSceneRect() const
{
    float s = frameScale();
    QRectF r;
    r.setLeft(pos().x() / s);
    r.setTop (pos().y() / s);
    r.setWidth (rect().width()  / s);
    r.setHeight(rect().height() / s);
    return r;
}

void Rectangle::setRealSceneRect(const QRectF& r)
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

    //_circle->setPos(rect.width(), rect.height());
}


} // namespace qgraph
