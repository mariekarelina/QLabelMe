#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>

namespace qgraph {

class DragCircle;

class Circle : public ShapeT<QGraphicsEllipseItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Circle)};
    int type() const override {return Type;}

    Circle(QGraphicsScene*, const QPointF&);

    void setFrameScale(float) override;
    void dragCircleMove(DragCircle*) override;
    void dragCircleRelease(DragCircle*) override;

    // Возвращает координаты расположения центра окружности на сцене с учетом
    // масштабного коэффициента frameScale()
    QPointF realCenter() const;
    void setRealCenter(const QPointF&);

    // Возвращает радиус окружности с учетом масштабного коэффициента
    int realRadius() const;
    void setRealRadius(int);

    void updateHandlePosition();

    int radius() const { return _radius; }

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    DragCircle* _circle;
    float _radius = {10};

    QColor _highlightColor = Qt::transparent; // Цвет выделения
};

} // namespace qgraph
