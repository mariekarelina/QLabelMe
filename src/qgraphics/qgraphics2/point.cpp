#include "point.h"
#include "drag_circle.h"
#include <QtGui>
#include <QMenu>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QPen>
#include <QBrush>
#include <QObject>

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

    // _dotVis = new QGraphicsEllipseItem(this);
    // _dotVis->setZValue(1e5);
    // _dotVis->setAcceptedMouseButtons(Qt::NoButton);
    // _dotVis->setFlag(QGraphicsItem::ItemIsSelectable, false);
    // _dotVis->setFlag(QGraphicsItem::ItemIsMovable, false);
    // _dotVis->setPen(Qt::NoPen);

    ensureDotVis();
    syncDotGeometry();
    syncDotColors();
    showDotIfIdle();
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

    // if (_dotVis)
    //     _dotVis->setRect(-_dotRadiusPx, -_dotRadiusPx, 2*_dotRadiusPx, 2*_dotRadiusPx);
    QGraphicsEllipseItem* dv = ensureDotVis();
    dv->setRect(-_dotRadiusPx, -_dotRadiusPx, 2*_dotRadiusPx, 2*_dotRadiusPx);

    updateHandlePosition();
    _frameScale = newScale;
    raiseHandlesToTop();
    showDotIfIdle();
    syncDotGeometry();
    syncDotColors();
}

void Point::dragCircleMove(DragCircle* circle)
{
    qgraph::Shape::HandleBlocker guard(this);

    if (circle != _handle) return;
    hideDot();

    // Для точки центр = позиция ручки в сцене
    const QPointF newScenePos = _handle->mapToScene(QPointF(0,0));
    setPos(newScenePos);
    updateHandlePosition();
}

void Point::dragCircleRelease(DragCircle*)
{
    showDotIfIdle();
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
    // Радиус «попадания» — чтобы по точке было легко кликнуть
    const qreal r = 8.0;
    QPainterPath p;
    p.addEllipse(QPointF(0,0), r, r);
    return p;
}

QRectF Point::boundingRect() const
{
    const qreal r = 8.0;
    const qreal pad = 1.5;
    return QRectF(-r - pad, -r - pad, 2*(r + pad), 2*(r + pad));
}

void Point::setDotStyle(const QColor& color, qreal diameterPx)
{
    _dotColor = color;
    //_dotRadiusPx = diameterPx * 0.5;
    _dotRadiusPx  = std::max<qreal>(diameterPx * 0.5, 0.5);

    ensureDotVis();
    syncDotColors();
    syncDotGeometry();
    showDotIfIdle();
}

QGraphicsEllipseItem* Point::ensureDotVis()
{
    if (_dotVis)
        return _dotVis;

    _dotVis = new QGraphicsEllipseItem(this);
    _dotVis->setAcceptedMouseButtons(Qt::NoButton);
    _dotVis->setFlag(QGraphicsItem::ItemIsSelectable, false);
    _dotVis->setFlag(QGraphicsItem::ItemIsMovable, false);
    _dotVis->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    _dotVis->setZValue(100000);
    _dotVis->setOpacity(1.0);
    _dotVis->setPen(Qt::NoPen);
    return _dotVis;
}


void Point::deleteItem()
{
    if (scene())
        scene()->removeItem(this);
    delete this;
}

QVariant Point::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemSelectedChange)
    {
        bool willBeSelected = value.toBool();
        if (willBeSelected)
            hideDot();
        else
            showDotIfIdle();
    }
    else if (change == QGraphicsItem::ItemPositionHasChanged)
    {
        updateHandlePosition();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void Point::hoverEnterEvent(QGraphicsSceneHoverEvent* ev)
{
    hideDot();
    QGraphicsItem::hoverEnterEvent(ev);
}

void Point::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{
    showDotIfIdle();
    QGraphicsItem::hoverLeaveEvent(ev);
}

void Point::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    hideDot();
    setSelected(true); // Гарантируем, что точка стала выбранной
    QGraphicsEllipseItem::mousePressEvent(ev);
}

void Point::mouseReleaseEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mouseReleaseEvent(ev);
    showDotIfIdle();
}

void Point::showDotIfIdle()
{
    const bool idle = !isSelected();
    if (auto* dv = ensureDotVis())
        dv->setVisible(idle);
}

void Point::hideDot()
{
    if (_dotVis)
        _dotVis->setVisible(false);
}

void Point::syncDotGeometry()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    const qreal r = std::max<qreal>(_dotRadiusPx, 0.5);
    dv->setRect(QRectF(-r, -r, 2*r, 2*r));
}

void Point::syncDotColors()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    dv->setBrush(QBrush(_dotColor));
    dv->setPen(Qt::NoPen);
}

}
