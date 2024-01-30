#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>

namespace qgraph {

class DragCircle;

class Square : public ShapeT<QGraphicsRectItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Square)};
    int type() const override {return Type;}

    Square(QGraphicsScene*);

    void setFrameScale(float) override;
    void dragCircleMove(DragCircle*) override;
    void dragCircleRelease(DragCircle*) override;

    // Возвращает координаты расположения прямоугольника на сцене с учетом
    // масштабного коэффициента frameScale()
    QRectF realSceneRect() const;
    void setRealSceneRect(const QRectF&);

private:
    DragCircle* _circle;
};

} // namespace qgraph
