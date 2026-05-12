#pragma once

#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>
#include <QObject>
#include "handle.h"


class LineDrawingItem : public QObject, public QGraphicsLineItem
{
public:
    LineDrawingItem() : startHandle(nullptr), endHandle(nullptr) {}  // Конструктор по умолчанию

    LineDrawingItem(const QLineF& line, QGraphicsItem* parent = nullptr)
        : QObject(), QGraphicsLineItem(line, parent)
    {
        // Устанавливаем стандартный стиль линии
        QPen pen(Qt::red);
        pen.setWidth(2);
        setPen(pen);

        // Создаём ручки для редактирования концов линии
        startHandle = new Handle(line.p1().x(), line.p1().y(), this);
        endHandle = new Handle(line.p2().x(), line.p2().y(), this);

        // Привязываем ручки к этой линии
        startHandle->setLinkedItem(this);
        endHandle->setLinkedItem(this);

        // Задаём координаты ручек относительно линии
        startHandle->setPos(line.p1());
        endHandle->setPos(line.p2());
    }

    // Метод для обновления линии, когда ручка перемещается
    virtual void updateHandlePosition(Handle* handle)
    {
        if (!handle) return;

        QLineF currentLine = line();
        if (handle == startHandle)
        {
            // Если движется начальная ручка, обновляем начальную точку линии
            currentLine.setP1(handle->pos());
        }
        else if (handle == endHandle)
        {
            // Если движется конечная ручка, обновляем конечную точку линии
            currentLine.setP2(handle->pos());
        }

        // Обновляем геометрию линии
        setLine(currentLine);
    }

    void setStartHandle(qreal x, qreal y)
    {
        if (!startHandle)
        {
            startHandle = new Handle(x, y, this);
        }
        else
        {
            startHandle->setPos(QPointF(x, y)); // Передаём QPointF, а не x и y отдельно
        }
    }

    void setEndHandle(qreal x, qreal y)
    {
        if (!endHandle)
        {
            endHandle = new Handle(x, y, this);
        }
        else
        {
            endHandle->setPos(QPointF(x, y));
        }
    }

    void setHandles(Handle* start, Handle* end)
    {
        startHandle = start;
        endHandle = end;

        // Подключение ручек к слоту обновления линии
        connect(startHandle, &Handle::positionChanged, this, &LineDrawingItem::updateLine);
        connect(endHandle, &Handle::positionChanged, this, &LineDrawingItem::updateLine);

        // Установка начальной линии (на случай, если ручки уже размещены)
        updateLine();
    }

    void updateLine()
    {
        if (!startHandle || !endHandle) return; // Проверяем, что обе ручки существуют

        // Устанавливаем новую линию между позициями ручек
        setLine(QLineF(startHandle->pos(), endHandle->pos()));
    }

private:
    Handle* startHandle = {nullptr}; // Ручка для начала линии
    Handle* endHandle;   // Ручка для конца линии
};
