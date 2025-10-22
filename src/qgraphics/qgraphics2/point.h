#pragma once

#include "user_type.h"
#include "shape.h"
//#include "drag_circle.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsRectItem>
#include <QObject>

namespace qgraph {

class DragCircle;

class Point : public ShapeT<QGraphicsEllipseItem>
{
public:
    explicit Point(QGraphicsScene* scene);
    ~Point() override;

    void setFrameScale(float newScale) override;
    void dragCircleMove(DragCircle* circle) override;
    void dragCircleRelease(DragCircle* circle) override;
    void raiseHandlesToTop() override;

    // Позиция точки в координатах сцены
    QPoint center() const { return QPoint(std::round(scenePos().x()), std::round(scenePos().y())); }
    void setCenter(const QPointF& p);

    // Обновление позиций и Z-уровней
    void updateHandlePosition();
    void updateHandlesZValue();

    QPainterPath shape() const override;
    QRectF boundingRect() const override;

public slots:
    void deleteItem();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* ev) override;

private:
    DragCircle* _handle = nullptr;

    // Вспомогательная визуализация самой "точки":
    // рисуем крошечную окружность, чтобы точка была видна даже без hover.
    // Диаметр подбирается от point size.
    qreal _baseDiameter = 6.0;

    QColor _highlightColor = Qt::transparent;
};

}
