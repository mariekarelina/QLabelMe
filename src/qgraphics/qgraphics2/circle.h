#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QPainter>

namespace qgraph {

class DragCircle;

class Circle : public ShapeT<QGraphicsEllipseItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Circle)};
    int type() const override {return Type;}

    Circle(QGraphicsScene*, const QPointF&);
    ~Circle();

    void setFrameScale(float) override;
    void dragCircleMove(DragCircle*) override;
    void dragCircleRelease(DragCircle*) override;

    // Возвращает координаты расположения центра окружности на сцене с учетом
    // масштабного коэффициента frameScale()
    QPointF realCenter() const;
    void setRealCenter(const QPointF&);

    QPoint center() const;

    // Возвращает радиус окружности с учетом масштабного коэффициента
    int realRadius() const;
    void setRealRadius(int);

    void updateHandlePosition();
    void updateHandlePosition(const QPointF& scenePos);

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    int radius() const { return _radius; }
    void updateHandleZValue();
    void updateCrossLines();
    void applyLineStyle(qreal lineWidth);
    void raiseHandleToTop();
    static QPointF pointOnCircle(const QRectF& rect, const QPointF& pos);
    void moveToBack();

    bool isCursorNearCircle(const QPointF& cursorPos) const;

    DragCircle* getDragCircle() const { return _circle; }

    QVariant saveState() const;
    void loadState(const QVariant& v);

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

private:
    void attachSceneFilter(QGraphicsScene* s);
    void detachSceneFilter();

private:
    // Временная ручка при наведении
    QPointer<DragCircle> _hoverHandle = {nullptr};
    //DragCircle* _circle;
    QPointer<DragCircle> _circle = {nullptr};
    QPointF _ghostLocalPos {0, 0};
    float _radius = {10};
    // Линии крестика
    QGraphicsLineItem* _verticalLine;
    QGraphicsLineItem* _horizontalLine;

    QColor _highlightColor = Qt::transparent; // Цвет выделения

    struct SceneFilter;
    SceneFilter* _sceneFilter = {nullptr};
};

} // namespace qgraph
