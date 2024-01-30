#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>


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
    void updateHandlePosition();

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

private:
    DragCircle* _circleTL; // Левый верхний угол
    DragCircle* _circleTR; // Правый верхний
    DragCircle* _circleBR; // Правый нижний
    DragCircle* _circleBL; // Левый нижний

    // Добавлено для предотвращения полного схлопывания прямоугольника
    const qreal _minSize = 10.0;

    bool _isDrawing = false;
    QPointF _startPoint;

    QColor _highlightColor = Qt::transparent; // Цвет выделения
};

} // namespace qgraph
