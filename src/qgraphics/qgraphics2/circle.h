#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>

namespace qgraph {

class DragCircle;

class Circle : public ShapeT<QGraphicsEllipseItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Circle)};
    int type() const override {return Type;}

    Circle(QGraphicsScene*);

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

private:
    DragCircle* _circle;
    float _radius = {10};
};

} // namespace qgraph
