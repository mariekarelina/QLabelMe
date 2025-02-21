#include "drag_circle.h"
#include "shape.h"
#include "square.h"
#include <QtGui>

namespace qgraph {

DragCircle::DragCircle(QGraphicsScene* scene)
{
    setFlags(ItemIsMovable);
    setRect(-_radius, -_radius, _radius * 2, _radius * 2);

    QColor color(255, 255, 255, 100);
    setBrush(QBrush(color));

    setPen(QPen(color.darker(200)));
    scene->addItem(this);
}

void DragCircle::setParent(QGraphicsItem* newParent)
{
    this->setParentItem(newParent);
    setZValue(newParent->zValue() + 1);
}

void DragCircle::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if (Shape* item = dynamic_cast<Shape*>(parentItem()))
        item->dragCircleMove(this);
}

void DragCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (Shape* item = dynamic_cast<Shape*>(parentItem()))
        item->dragCircleRelease(this);
}

} // namespace qgraph
