#include "drag_circle.h"
#include "circle.h"
#include "square.h"
#include "point.h"
#include "shape.h"

#include <QApplication>
#include <QtGui>

#include <algorithm>

namespace qgraph {

DragCircle::DragCircle(QGraphicsScene* scene)
{

    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    setRect(-_currentSize/2, -_currentSize/2, _currentSize, _currentSize);
    // Сохранить именно этот размер как базовый
    rememberCurrentAsBase(this);

    // Устанавливаем начальные значения
    _baseColor = Qt::red; // Цвет по умолчанию, будет переопределен из настроек
    _smallSize = 8.0;
    _largeSize = 12.0;
    _currentSize = _smallSize;

    qreal half = _smallSize / 2.0;
    _baseRect = QRectF(-half, -half, _smallSize, _smallSize);
    _basePen = QPen(_baseColor.darker(150), 1);
    _baseBrush = QBrush(_baseColor);

    setRect(_baseRect);
    setPen(_basePen);
    setBrush(_baseBrush);

    if (_isGhost)
    {
        setAcceptedMouseButtons(Qt::NoButton); // запрет перехвата кликов
        setAcceptHoverEvents(true);            // ховер всё равно разрешаем
        setZValue(-1);                         // под живыми точками
    }

    // Устанавливаем высокий z-value, чтобы точки всегда были поверх других элементов
    setZValue(10000);

    // Устанавливаем начальный маленький размер
    setSmallSize();

    QColor color(123, 104, 238);
    setBrush(QBrush(color));
    setPen(QPen(Qt::black, 1));
    setPen(QPen(color.darker(200)));

    _baseRect  = rect();
    _basePen   = pen();
    _baseBrush = brush();

    setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
    setFlag(QGraphicsItem::ItemIgnoresTransformations);
    scene->addItem(this);

    _selectedHandleColor = Qt::yellow;
}

DragCircle::~DragCircle()
{
    _isValid = false;
}

void DragCircle::setParent(QGraphicsItem* newParent)
{
    this->setParentItem(newParent);
    setZValue(100000);
}

void DragCircle::raiseToTop()
{
    setZValue(100000);
}

void DragCircle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    setHoverStyle(true);
    QGraphicsRectItem::hoverEnterEvent(event);
}

void DragCircle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    setHoverStyle(false);
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void DragCircle::setSmallSize()
{
    _currentSize = _smallSize;
    setRect(-_smallSize/2, -_smallSize/2, _smallSize, _smallSize);
}

bool DragCircle::collidesWithItem(const QGraphicsItem* other,
                                  Qt::ItemSelectionMode mode) const
{
    // Всегда разрешаем взаимодействие с другими DragCircle
    if (dynamic_cast<const DragCircle*>(other))
    {
        return true;
    }
    // Игнорируем столкновения с родительскими фигурами
    if (other == parentItem())
    {
        return false;
    }
    // Для остальных элементов используем стандартную логику
    return QGraphicsRectItem::collidesWithItem(other, mode);
}

QPainterPath DragCircle::shape() const
{
    const qreal r = std::max<qreal>(_interactionRadius, 0.0);
    QPainterPath path;
    path.addRect(rect().adjusted(-r, -r, r, r));
    return path;
}

void DragCircle::rememberCurrentAsBase(QGraphicsRectItem* item)
{
    if (!item) return;
    // Запоминаем размер/перо/кисть
    item->setData(kRoleBaseRect,  item->rect());
    item->setData(kRoleBasePen,   QVariant::fromValue(item->pen()));
    item->setData(kRoleBaseBrush, QVariant::fromValue(item->brush()));
}

void DragCircle::setHoverStyle(bool hover)
{
    if (!_isValid) return;

    const bool isCircleHandle = (dynamic_cast<qgraph::Circle*>(parentItem()) != nullptr);
    const bool isPointHandle  = (dynamic_cast<qgraph::Point*>(parentItem())  != nullptr);

    if (hover)
    {
        // Размер увеличиваем (чтобы удобнее было попадать)
        const qreal side = _baseRect.width() * 1.5;
        setRect(-side/2.0, -side/2.0, side, side);

        if (isCircleHandle)
        {
            setBrush(QBrush(_selectedHandleColor));
            setPen(QPen(Qt::black, 1));
        }
        else if (isPointHandle)
        {
            setBrush(QBrush(_selectedHandleColor));
            setPen(QPen(Qt::black, 1));
        }
        else
        {
            setBrush(QBrush(_selectedHandleColor));
            setPen(QPen(Qt::black, 1));
        }
    }
    else
    {
        restoreBaseStyle();
    }

    update();
}


void DragCircle::restoreBaseStyle()
{
    if (!_isValid) return;

    // Определяем, является ли родителем Circle
    bool isCircleHandle = (dynamic_cast<qgraph::Circle*>(parentItem()) != nullptr);
    bool isPointHandle  = (dynamic_cast<qgraph::Point*>(parentItem())  != nullptr);

    if (isCircleHandle || isPointHandle)
    {
        // Для кругов - невидимая точка
        setRect(_baseRect);
        setPen(Qt::NoPen);
        setBrush(Qt::NoBrush);
    }
    else
    {
        // Для других фигур - обычный стиль
        setRect(_baseRect);
        setPen(_basePen);
        setBrush(_baseBrush);
    }
    update();
}

void DragCircle::setBaseStyle(const QColor& color, qreal size)
{
    _baseColor = color;
    _smallSize = size;
    _largeSize = size * 1.5;
    _currentSize = size;

    // Обновляем базовые параметры
    qreal half = size / 2.0;
    _baseRect = QRectF(-half, -half, size, size);
    _basePen = QPen(color.darker(150), 1);
    _baseBrush = QBrush(color);

    restoreBaseStyle();
}

void DragCircle::setHoverSizingEnabled(bool on)
{
    _hoverSizingEnabled = on;
}

void DragCircle::setSelectedHandleColor(const QColor& color)
{
    _selectedHandleColor = color;
}

void DragCircle::setCenter(const QPointF &center)
{
    _center = center;
}

void DragCircle::setUserHidden(bool on)
{
    if (_userHidden == on)
           return;

       _userHidden = on;
       QGraphicsItem::setVisible(!_userHidden && _runtimeVisible);
}

void DragCircle::setInteractionRadius(qreal r)
{
    _interactionRadius = std::max<qreal>(0.0, r);
    prepareGeometryChange();
    update();
}

void DragCircle::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    if (_contextMenu)
    {
        _contextMenu->exec(event->screenPos());
        event->accept(); // Помечаем событие как обработанное
    }
}

QVariant DragCircle::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == QGraphicsItem::ItemPositionChange ||
        change == QGraphicsItem::ItemPositionHasChanged)
    {
        // Проверяем, что родительский элемент все еще существует
        if (!parentItem())
        {
            return QGraphicsRectItem::itemChange(change, value);
        }
        // Если эту ручку прямо сейчас тянем мышью - не вмешиваемся
        if (scene() && scene()->mouseGrabberItem() == this)
            return QGraphicsRectItem::itemChange(change, value);

        // Определяем родителя-фигуру
        if (Shape* shape = dynamic_cast<Shape*>(parentItem()))
        {
            // Если фигура сейчас программно раскладывает ручки - не триггерим рекурсию
            if (shape->handlesUpdateBlocked())
                return QGraphicsRectItem::itemChange(change, value);

            // Перетаскивание мышью или «призрак» активен - это реальное редактирование
            if (QApplication::mouseButtons() != Qt::NoButton || _ghostDriven)
            {
                shape->dragCircleMove(this);
                emit moved(this);
            }
        }
    }

    if (change == QGraphicsItem::ItemVisibleChange)
    {
        const bool requested = value.toBool();
        _runtimeVisible = requested;

        // Если ручка скрыта пользователем, запрещаем включать видимость
        if (_userHidden && requested)
            return false; // Блокируем попытку сделать видимой
    }

    return QGraphicsRectItem::itemChange(change, value);
}

} // namespace qgraph
