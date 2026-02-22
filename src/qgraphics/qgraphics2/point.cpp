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
#include <QStyle>
#include <QMetaObject>
#include <QVariant>
#include <QGraphicsSceneContextMenuEvent>
#include <QStyleOptionGraphicsItem>

namespace qgraph {

Point::Point(QGraphicsScene* scene)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setPen(Qt::NoPen);
    setBrush(Qt::NoBrush);

    // Базовый эллипс диаметром
    const qreal r = _baseDiameter * 0.5;
    setRect(QRectF(-r, -r, _baseDiameter, _baseDiameter));

    setZLevel(1);
    scene->addItem(this);

    // Ручка
    _handle = new DragCircle(scene);
    _handle->setParentItem(this);
    _handle->setHoverSizingEnabled(false);
    _handle->setVisible(true);
    _handle->setZValue(10000);
    QObject::connect(_handle, &DragCircle::hoverEntered, _handle, [this]{ raiseHandlesToTop(); });
    QObject::connect(_handle, &DragCircle::hoverLeft,    _handle, [this]{ raiseHandlesToTop(); });

    updateHandlePosition();
    raiseHandlesToTop();

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

void Point::setDotStyle(const QColor& color, qreal dotDiameterPx, qreal handleSizePx)
{
    _dotColor = color;

    const qreal d = std::max<qreal>(dotDiameterPx, 1.0);
    _dotRadiusPx  = std::max<qreal>(d * 0.5, 0.5);

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
    // if (change == QGraphicsItem::ItemSelectedChange)
    // {
    //     bool willBeSelected = value.toBool();
    //     if (willBeSelected)
    //         hideDot();
    //     else
    //         showDotIfIdle();
    // }
    else if (change == QGraphicsItem::ItemPositionHasChanged)
    {
        updateHandlePosition();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

void Point::hoverEnterEvent(QGraphicsSceneHoverEvent* ev)
{
    //_interacting = true;
    //hideDot();
    QGraphicsItem::hoverEnterEvent(ev);
}

void Point::hoverLeaveEvent(QGraphicsSceneHoverEvent* ev)
{
    //_interacting = false;
    showDotIfIdle();
    QGraphicsItem::hoverLeaveEvent(ev);
}

void Point::mousePressEvent(QGraphicsSceneMouseEvent* ev)
{
    // _interacting = true;
    // hideDot();
    setSelected(true); // Гарантируем, что точка стала выбранной
    QGraphicsEllipseItem::mousePressEvent(ev);
}

void Point::mouseReleaseEvent(QGraphicsSceneMouseEvent* ev)
{
    QGraphicsItem::mouseReleaseEvent(ev);
    //_interacting = false;
    showDotIfIdle();
}

void Point::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;

    QAction* changeClassAction = menu.addAction("Изменить класс");
    menu.addSeparator();
    QAction* deleteAction = menu.addAction("Удалить (Del)");

    QObject::connect(changeClassAction, &QAction::triggered, [this]() {
        auto sc = this->scene();
        if (!sc) return;

        QObject* receiver = sc->property("classChangeReceiver").value<QObject*>();
        if (!receiver) return;

        const qulonglong uid = this->data(0x1337ABCD).toULongLong();
        if (!uid) return;

        QMetaObject::invokeMethod(receiver,
                                  "changeClassByUid",
                                  Qt::DirectConnection,
                                  Q_ARG(qulonglong, uid));
    });

    QObject::connect(deleteAction, &QAction::triggered, [this]() {
        this->deleteItem();
    });

    menu.exec(event->screenPos());
    event->accept();
}

void Point::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(widget);

    if (!(option->state & QStyle::State_Selected))
        return;

    QPen pen;
    pen.setStyle(Qt::DashLine);
    pen.setCosmetic(true);
    pen.setWidthF(1.0);
    pen.setColor(QColor(255, 255, 255, 200));

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    QRectF rc;

    if (auto* dv = ensureDotVis())
    {
        rc = dv->shape().boundingRect();

        const qreal pad = 1.0;
        rc = rc.adjusted(-pad, -pad, pad, pad);
    }
    else
    {
        qreal R = std::max<qreal>(_dotRadiusPx, 1.0);
        rc = QRectF(-R, -R, 2*R, 2*R);
    }

    painter->drawRect(rc);
}

void Point::showDotIfIdle()
{
    if (auto* dv = ensureDotVis())
        dv->setVisible(true);
}

void Point::hideDot()
{
    if (_dotVis)
        _dotVis->setVisible(false);
}


//static constexpr qreal kCoverPaddingPx = 1.0; // запас на неточности между пикселями

void Point::syncDotGeometry()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    if (!dv)
        return;

    const qreal R = std::max<qreal>(_dotRadiusPx, 0.5);
    dv->setRect(QRectF(-R, -R, 2*R, 2*R));
}

void Point::syncDotColors()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    dv->setBrush(QBrush(_dotColor));
    dv->setPen(Qt::NoPen);
}

}
