#include "drag_circle.h"
#include "shape.h"
#include "square.h"
#include <QtGui>

namespace qgraph {

DragCircle::DragCircle(QGraphicsScene* scene)
{
    setFlag(QGraphicsItem::ItemIsMovable);
    //setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setAcceptHoverEvents(true);

    setRect(-_radius, -_radius, _radius * 2, _radius * 2);

    QColor color(255, 255, 255, 100);
    setBrush(QBrush(color));

    setPen(QPen(color.darker(200)));
    scene->addItem(this);


    // Инициализация контекстного меню
    _contextMenu = new QMenu();
    QAction* deleteAction = _contextMenu->addAction("Удалить точку");
    connect(deleteAction, &QAction::triggered, [this]() {
        emit deleteRequested(this); // Испускаем сигнал
    });

}

void DragCircle::setParent(QGraphicsItem* newParent)
{
    this->setParentItem(newParent);
    setZValue(newParent->zValue() + 1);
}

void DragCircle::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if (_contextMenu)
    {
        _contextMenu->exec(event->screenPos());
        event->accept(); // Помечаем событие как обработанное
    }
}

void DragCircle::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept(); // Перехватываем событие, чтобы оно не дошло до родителя
    QGraphicsItem::mousePressEvent(event);
}

void DragCircle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if (Shape* item = dynamic_cast<Shape*>(parentItem()))
        item->dragCircleMove(this);

    emit moved(this); // Испускаем сигнал при перемещении круга
}

//void DragCircle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
//{

//    if (event->buttons() & Qt::LeftButton)
//    {
//        // Обновляем позицию круга
//        setPos(mapToParent(event->pos() - event->buttonDownPos(Qt::LeftButton)));
//        emit moved(this);
//        event->accept();
//    }
//    else
//    {
//        QGraphicsItem::mouseMoveEvent(event);
//    }
//}

void DragCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (Shape* item = dynamic_cast<Shape*>(parentItem()))
        item->dragCircleRelease(this);

    emit released(this); // Испускаем сигнал при отпускании мыши
}

//void DragCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
//{
//    event->accept();
//    emit released(this);
//    QGraphicsItem::mouseReleaseEvent(event);
//}

} // namespace qgraph
