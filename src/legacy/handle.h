#pragma once

#include <QGraphicsEllipseItem>
#include <QObject>

// Forward-декларация классов
class LineDrawingItem;

class Handle : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    Handle(qreal x, qreal y, QGraphicsItem* parent = nullptr);

    void setLinkedItem(QGraphicsItem* item);
    void setPos(const QPointF& pos);

signals:
    void positionChanged(); // Сигнал для извещения об изменении позиции

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    // Переопределяем метод для отслеживания изменений позиции ручки
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override
    {
        QGraphicsEllipseItem::mouseReleaseEvent(event);
        emit positionChanged(); // Отправляем сигнал при отпускании мыши
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override
    {
        QGraphicsEllipseItem::mouseMoveEvent(event);
        emit positionChanged(); // Отправляем сигнал при перемещении ручки
    }

private:
    QGraphicsItem* linkedItem = nullptr; // Указатель на связанный объект
};
