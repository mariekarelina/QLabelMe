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
#include <QPointer>

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

    // Получить стиль из настроек
    void setDotStyle(const QColor& color, qreal diameterPx);
    QGraphicsEllipseItem* ensureDotVis();

public slots:
    void deleteItem();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent* ev) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* ev) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* ev) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* ev) override;

private:
    void showDotIfIdle();
    void hideDot();
    void appearanceFromSettings(qreal handleSizePx,
                                const QColor& pointColor,
                                qreal outlineWidthPx);
    void syncDotGeometry();
    void syncDotColors();

private:
    DragCircle* _handle = {nullptr};

    // Вспомогательная визуализация самой "точки":
    // рисуем крошечную окружность, чтобы точка была видна даже без hover.
    // Диаметр подбирается от point size.
    qreal _baseDiameter = 6.0;

    QColor _highlightColor = Qt::transparent;

    QGraphicsEllipseItem* _dotVis = {nullptr}; // Кружок точки
    //QPointer<QGraphicsEllipseItem> _dotVis;
    qreal  _dotRadiusPx = 3.0;
    QColor _dotColor = Qt::white;

    bool _interacting = false; // true только во время активного взаимодействия

    qreal _coverScale = 1.15; // 15% больше квадрата-узла

};

}
