#pragma once

#include <QGraphicsRectItem>
#include "handle.h"


class SquareDrawingItem : public QGraphicsRectItem
{
public:
    explicit SquareDrawingItem(QGraphicsItem* parent = nullptr)
        : QGraphicsRectItem(parent), topLeftHandle(nullptr), bottomRightHandle(nullptr)
    {
        createHandles(); // Создаём ручки при инициализации
    }

    void updateHandlePosition(Handle* handle)
    {
        if (!handle) return;

        QRectF rect = this->rect();

        // Обновляем положение квадрата в зависимости от ручки
        if (handle == topLeftHandle)
        {
            rect.setTopLeft(handle->pos());
        }
        else if (handle == bottomRightHandle)
        {
            rect.setBottomRight(handle->pos());
        }

        setRect(rect);          // Обновляем квадрат
        updateHandlesPosition(); // Перемещаем ручки
    }

    void setTopLeftHandle(qreal x, qreal y)
    {
        topLeftHandle = new Handle(x, y, this); // Ручка в верхнем левом углу
        setRect(QRectF(QPointF(x, y), rect().bottomRight())); // Устанавливаем часть квадрата
    }

    void setBottomRightHandle(qreal x, qreal y)
    {
        if (!bottomRightHandle)
        {
            bottomRightHandle = new Handle(x, y, this); // Ручка в нижнем правом углу
        }
        setRect(QRectF(rect().topLeft(), QPointF(x, y))); // Обновляем размеры квадрата
    }

private:
    void createHandles()
    {
        // Создаём ручки и привязываем их к объекту
        topLeftHandle = new Handle(rect().topLeft().x(), rect().topLeft().y(), this);
        bottomRightHandle = new Handle(rect().bottomRight().x(), rect().bottomRight().y(), this);

        topLeftHandle->setLinkedItem(this);
        bottomRightHandle->setLinkedItem(this);
    }

    void updateHandlesPosition()
    {
       // Обновляем позицию ручек
       QRectF rect = this->rect();
       topLeftHandle->setPos(rect.topLeft());
       bottomRightHandle->setPos(rect.bottomRight());
   }

public:
    Handle* topLeftHandle  = {nullptr};
    Handle* bottomRightHandle  = {nullptr};
};
