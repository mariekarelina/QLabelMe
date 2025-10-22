#include "point.h"
#include "drag_circle.h"
#include <QtGui>
#include <QMenu>

namespace qgraph {

Point::Point(QGraphicsScene* scene)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    // Тонкая «рамка» точки
    QPen pen = this->pen();
    pen.setColor(QColor(0, 255, 0));
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);
    setBrush(Qt::transparent);

    // Базовый эллипс диаметром
    const qreal r = _baseDiameter * 0.5;
    setRect(QRectF(-r, -r, _baseDiameter, _baseDiameter));

    setZLevel(1);
    scene->addItem(this);

    // Ручка
    _handle = new DragCircle(scene);
    _handle->setParentItem(this);
    _handle->setHoverSizingEnabled(true);
    _handle->setVisible(true);
    _handle->setZValue(10000);
    QObject::connect(_handle, &DragCircle::hoverEntered, _handle, [this]{ raiseHandlesToTop(); });
    QObject::connect(_handle, &DragCircle::hoverLeft,    _handle, [this]{ raiseHandlesToTop(); });

    updateHandlePosition();
    raiseHandlesToTop();
}

Point::~Point()
{
    _handle = nullptr;
}

void Point::setCenter(const QPointF& p)
{
    if (pos() == p) return;
    setPos(p);
    updateHandlePosition();
}

void Point::setFrameScale(float newScale)
{
    float s = newScale / frameScale();

    setPos(pos().x() * s, pos().y() * s);

    _baseDiameter *= s;
    const qreal r = _baseDiameter * 0.5;
    prepareGeometryChange();
    setRect(QRectF(-r, -r, _baseDiameter, _baseDiameter));

    updateHandlePosition();
    _frameScale = newScale;
    raiseHandlesToTop();
}

void Point::dragCircleMove(DragCircle* circle)
{
    qgraph::Shape::HandleBlocker guard(this);

    if (circle != _handle) return;

    // Для точки центр = позиция ручки в сцене
    const QPointF newScenePos = _handle->mapToScene(QPointF(0,0));
    setPos(newScenePos);
    updateHandlePosition();
}

void Point::dragCircleRelease(DragCircle*)
{

}

void Point::raiseHandlesToTop()
{
    if (!_handle) return;
    _handle->setZValue(10000);
}

void Point::updateHandlePosition()
{
    if (!_handle) return;

    qgraph::Shape::HandleBlocker guard(this);
    _handle->setPos(0, 0);
}

void Point::updateHandlesZValue()
{
    raiseHandlesToTop();
}

QPainterPath Point::shape() const
{
    // Радиус «попадания» — чтобы по точке было легко кликнуть.
    const qreal r = 8.0;
    QPainterPath p;
    p.addEllipse(QPointF(0,0), r, r);
    return p;
}

QRectF Point::boundingRect() const
{
    // Должен покрывать shape() + небольшой запас под перо, если рисуете контур
    const qreal r = 8.0;
    const qreal pad = 1.5;
    return QRectF(-r - pad, -r - pad, 2*(r + pad), 2*(r + pad));
}

void Point::deleteItem()
{
    if (scene())
        scene()->removeItem(this);
    delete this;
}

QVariant Point::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        const bool sel = value.toBool();
        if (sel) {
            _highlightColor = QColor(255, 255, 0, 100);
        } else {
            _highlightColor = Qt::transparent;
        }
        setBrush(_highlightColor);
        raiseHandlesToTop();
    }
    else if (change == QGraphicsItem::ItemPositionHasChanged)
    {
        updateHandlePosition();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void Point::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    setSelected(true); // Гарантируем, что точка стала выбранной
    QGraphicsEllipseItem::mousePressEvent(ev);
}

}
