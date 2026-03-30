#include "point.h"
#include "drag_circle.h"
#include <QtGui>
#include <QMenu>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
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
    const qreal r = dotRadiusLocal();
    setRect(QRectF(-r, -r, 2 * r, 2 * r));

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
    const float s = newScale / frameScale();

    setPos(pos().x() * s, pos().y() * s);

    _frameScale = newScale;

    updateHandlePosition();
    raiseHandlesToTop();
    syncDotGeometry();
    syncDotColors();
    showDotIfIdle();
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
    QPainterPath p;

    const qreal R = dotRadiusLocal() * _coverScale;
    p.addEllipse(QPointF(0, 0), R, R);

    return p;
}

QRectF Point::boundingRect() const
{
    const qreal R = dotRadiusLocal() * _coverScale;
    const qreal pad = 0.5 * onePixelLocal();

    return QRectF(-R - pad, -R - pad, 2.0 * (R + pad), 2.0 * (R + pad));
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
    auto sc = this->scene();
    if (!sc)
        return;

    if (QObject* receiver = sc->property("shapeDeleteReceiver").value<QObject*>())
    {
        QMetaObject::invokeMethod(receiver,
                                  "on_actDelete_triggered",
                                  Qt::DirectConnection);
    }
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
    if (!isSelected())
    {
        if (scene())
        {
            const auto selected = scene()->selectedItems();
            for (QGraphicsItem* it : selected)
            {
                if (it && it != this)
                    it->setSelected(false);
            }
        }
        setSelected(true);
    }
    setFocus();

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

    const qreal R = dotRadiusLocal() * _coverScale;
    const qreal pad = 0.5 * onePixelLocal();
    const QRectF rc(-R - pad, -R - pad, 2.0 * (R + pad), 2.0 * (R + pad));

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
    const qreal localR = dotRadiusLocal() * _coverScale;

    prepareGeometryChange();
    setRect(QRectF(-localR, -localR, 2 * localR, 2 * localR));

    QGraphicsEllipseItem* dv = ensureDotVis();
    if (!dv)
        return;

    const qreal pixelR = std::max<qreal>(_dotRadiusPx, 0.5);
    dv->setRect(QRectF(-pixelR, -pixelR, 2 * pixelR, 2 * pixelR));
}

void Point::syncDotColors()
{
    QGraphicsEllipseItem* dv = ensureDotVis();
    dv->setBrush(QBrush(_dotColor));
    dv->setPen(Qt::NoPen);
}

qreal Point::dotRadiusLocal() const
{
    const qreal px = std::max<qreal>(_dotRadiusPx, 0.5);

    if (scene() && !scene()->views().isEmpty() && scene()->views().first())
    {
        QGraphicsView* view = scene()->views().first();
        const qreal viewScale = std::max<qreal>(view->transform().m11(), 0.000001);
        return px / viewScale;
    }

    return px;
}

qreal Point::onePixelLocal() const
{
    if (scene() && !scene()->views().isEmpty() && scene()->views().first())
    {
        QGraphicsView* view = scene()->views().first();
        const qreal viewScale = std::max<qreal>(view->transform().m11(), 0.000001);
        return 1.0 / viewScale;
    }

    return 1.0;
}

}
