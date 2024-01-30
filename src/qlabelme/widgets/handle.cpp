#include "handle.h"
#include "line.h" // Подключение полного определения LineDrawingItem

Handle::Handle(qreal x, qreal y, QGraphicsItem* parent)
    : QObject(), QGraphicsEllipseItem(x, y, 10, 10, parent), linkedItem(nullptr)
{
    setBrush(Qt::red);

    setFlag(QGraphicsItem::ItemIsMovable);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges); // Уведомления об изменении позиции
}

void Handle::setLinkedItem(QGraphicsItem* item)
{
    linkedItem = item;
}

void Handle::setPos(const QPointF& pos)
{
    QGraphicsEllipseItem::setPos(pos); // Меняем позицию ручки
    emit positionChanged(); // Отправляем сигнал, что позиция изменилась
}

QVariant Handle::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionChange) {
        if (auto parent = dynamic_cast<LineDrawingItem*>(linkedItem)) {
            // Убедитесь, что linkedItem является LineDrawingItem
            parent->updateHandlePosition(this);
        }
    }

    return QGraphicsEllipseItem::itemChange(change, value);
}
