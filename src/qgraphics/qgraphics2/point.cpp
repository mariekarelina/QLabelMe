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

    syncDotGeometry();
    showDotIfIdle();
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
    syncDotGeometry();
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

    const bool ignores = _handle ? (_handle->flags() & QGraphicsItem::ItemIgnoresTransformations) : true;
    _dotVis->setFlag(QGraphicsItem::ItemIgnoresTransformations, ignores);
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
    if (change == QGraphicsItem::ItemSceneHasChanged)
    {
        // Когда элемент окончательно попал в сцену – гарантируем корректное состояние круга
        ensureDotVis();
        syncDotGeometry();
        syncDotColors();
        showDotIfIdle();
    }
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
    _interacting = true;
    hideDot();
    QGraphicsItem::hoverEnterEvent(ev);
}

void Point::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{
    _interacting = false;
    showDotIfIdle();
    QGraphicsItem::hoverLeaveEvent(ev);
}

void Point::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    _interacting = true;
    hideDot();
    setSelected(true); // Гарантируем, что точка стала выбранной
    QGraphicsEllipseItem::mousePressEvent(ev);
}

void Point::mouseReleaseEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mouseReleaseEvent(ev);
    _interacting = false;
    showDotIfIdle();
}

void Point::showDotIfIdle()
{
    const bool idle = !_interacting;
    if (auto* dv = ensureDotVis())
        dv->setVisible(idle);
}

void Point::hideDot()
{
    if (_dotVis)
        _dotVis->setVisible(false);
}


static constexpr qreal kCoverPaddingPx = 1.0; // запас на неточности между пикселями

void Point::syncDotGeometry()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    if (!dv) return;

    if (_handle)
    {
        const QRectF hr = _handle->rect();
        const qreal w   = std::abs(hr.width());
        const qreal h   = std::abs(hr.height());

        // Радиус должен быть >= половины диагонали квадрата
        const qreal halfDiag = 0.5 * std::hypot(w, h);

        // Толщина пера квадрата может быть видна по краю - добавим ее половину
        qreal penHalf = 0.0;
        const QPen pen = _handle->pen();
        penHalf = 0.5 * pen.widthF();

        // Если ручка игнорирует трансформации, добавим небольшой пиксельный запас
        const bool ignores = _handle->flags() & QGraphicsItem::ItemIgnoresTransformations;
        const qreal pad = ignores ? kCoverPaddingPx : 0.0;

        const qreal R = halfDiag + penHalf + pad;

        dv->setRect(QRectF(-R, -R, 2*R, 2*R));
        return;
    }

    const qreal R = 4.0;
    dv->setRect(QRectF(-R, -R, 2*R, 2*R));
}

void Point::syncDotColors()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    dv->setBrush(QBrush(_dotColor));
    dv->setPen(Qt::NoPen);
}

}
