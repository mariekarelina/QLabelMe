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
    qreal radius() const { return _radius; }

    QPointF center() const;

    // Возвращает радиус окружности с учетом масштабного коэффициента
    qreal realRadius() const;
    void setRealRadius(qreal);

    void updateHandlePosition();
    void updateHandlePosition(const QPointF& scenePos);

    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void updateHandleZValue();
    void updateCrossLines();
    void applyLineStyle(qreal lineWidth);
    void raiseHandleToTop();
    static QPointF pointOnCircle(const QRectF& rect, const QPointF& pos);
    void moveToBack();

    bool isCursorNearCircle(const QPointF& cursorPos) const;

    void toggleSelectionRect();
    bool isSelectionRectVisible() const { return _selectionRectVisible; }

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
    void updateSelectionRect();

private:
    // Временная ручка при наведении
    QPointer<DragCircle> _hoverHandle = {nullptr};
    QPointer<DragCircle> _circle = {nullptr};
    QPointF _ghostLocalPos {0, 0};
    qreal _radius = {10};
    // Линии крестика
    QGraphicsLineItem* _verticalLine;
    QGraphicsLineItem* _horizontalLine;

    QColor _highlightColor = Qt::transparent;
    bool _hovered = false;
    bool _suspendHandleCallbacks = false;

    struct SceneFilter;
    SceneFilter* _sceneFilter = {nullptr};

    QGraphicsRectItem* _selectionRect = nullptr;
    bool _selectionRectVisible = true;
};

} // namespace qgraph
