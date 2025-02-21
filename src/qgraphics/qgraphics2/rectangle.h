#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>

namespace qgraph {

class DragCircle;

class Rectangle : public ShapeT<QGraphicsRectItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Rectangle)};
    int type() const override {return Type;}

    Rectangle(QGraphicsScene*);

    void setFrameScale(float) override;
    void dragCircleMove(DragCircle*) override;
    void dragCircleRelease(DragCircle*) override;

    // Возвращает координаты расположения прямоугольника на сцене с учетом
    // масштабного коэффициента frameScale()
    QRectF realSceneRect() const;
    void setRealSceneRect(const QRectF&);

private:
    DragCircle* _circleTL; // Левый верхний угол
    DragCircle* _circleTR; // Правый верхний
    DragCircle* _circleBR; // Правый нижний
};

} // namespace qgraph
