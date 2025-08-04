#include "drag_circle.h"
#include "circle.h"
#include "shape.h"
#include "square.h"
#include <QtGui>
#include <QApplication>

namespace qgraph {

DragCircle::DragCircle(QGraphicsScene* scene)
{
    //setFlag(QGraphicsItem::ItemIsMovable);
    //setFlag(QGraphicsItem::ItemIsSelectable);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges);
    setAcceptHoverEvents(false);
    setAcceptedMouseButtons(Qt::NoButton);
    setFlag(QGraphicsItem::ItemIsMovable, false);

    setRect(-_currentSize/2, -_currentSize/2, _currentSize, _currentSize);
    // сохранить именно этот размер как базовый
    rememberCurrentAsBase(this);

    if (_isGhost)
    {
        setAcceptedMouseButtons(Qt::NoButton); // запрет перехвата кликов
        setAcceptHoverEvents(true);            // ховер всё равно разрешаем
        setZValue(-1);                         // под живыми точками
    }

    // Устанавливаем высокий z-value, чтобы точки всегда были поверх других элементов
    setZValue(10000); // Очень высокое значение

    // Устанавливаем начальный маленький размер
    setSmallSize();

    //setRect(-_radius, -_radius, _radius * 2, _radius * 2);

    QColor color(255, 255, 255, 200);
    setBrush(QBrush(color));
    setPen(QPen(Qt::black, 1));
    setPen(QPen(color.darker(200)));

    _baseRect  = rect();
    _basePen   = pen();
    _baseBrush = brush();

    setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
    setFlag(QGraphicsItem::ItemIgnoresTransformations); // Игнорируем трансформации вида
    scene->addItem(this);


    // // Инициализация контекстного меню
    // _contextMenu = new QMenu();
    // QAction* deleteAction = _contextMenu->addAction("Удалить точку");
    // QObject::connect(deleteAction, &QAction::triggered, [this]() {
    //     emit deleteRequested(this); // Испускаем сигнал
    // });
}

DragCircle::~DragCircle()
{
}

void DragCircle::setParent(QGraphicsItem* newParent)
{
    this->setParentItem(newParent);
    setZValue(100000);
}

void DragCircle::raiseToTop()
{
    setZValue(100000); // Всегда на самом верху
}

// void DragCircle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
// {
//     if (_hoverSizingEnabled)
//         applyHoverStyle(this, true);

//     QGraphicsRectItem::hoverEnterEvent(event);
// }

// void DragCircle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
// {
//     if (_hoverSizingEnabled)
//         applyHoverStyle(this, false);

//     QGraphicsRectItem::hoverLeaveEvent(event);
// }

void DragCircle::setBaseSize(qreal size)
{
    _smallSize = size;
    _largeSize = size * 1.5; // или другая логика, если нужно
    _currentSize = size;
    _baseRect = QRectF(-size/2, -size/2, size, size);
}

void DragCircle::setSmallSize()
{
    _currentSize = _smallSize;
    setRect(-_smallSize/2, -_smallSize/2, _smallSize, _smallSize);
}

void DragCircle::setLargeSize()
{
    _currentSize = _largeSize;
    setRect(-_largeSize/2, -_largeSize/2, _largeSize, _largeSize);
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
    // Увеличиваем область взаимодействия для удобства
    QPainterPath path;
    path.addRect(rect().adjusted(-4, -4, 4, 4));
    return path;
}

void DragCircle::applyHoverStyle(QGraphicsRectItem* item, bool active)
{
    if (!item) return;

    // Убедимся, что «база» у этого item сохранена
    if (!item->data(kRoleBaseRect).isValid())
        rememberCurrentAsBase(item);

    const QRectF baseRect = item->data(kRoleBaseRect).toRectF();
    const QPen   basePen  = item->data(kRoleBasePen).value<QPen>();
    const QBrush baseBr   = item->data(kRoleBaseBrush).value<QBrush>();

    if (active)
    {
        // Увеличиваем ИМЕННО относительно базового размера (без накопления)
        //const qreal side = std::max(baseRect.width(), baseRect.height()) + 2.0; // или *1.5
        const qreal side = baseRect.width() * 1.5;
        item->setRect(QRectF(-side/2.0, -side/2.0, side, side));
        item->setBrush(QBrush(Qt::yellow));
        item->setPen(QPen(Qt::black, 1));
        item->update();
    }
    else
    {
        // Полный возврат к «базе»
        item->setRect(baseRect);
        item->setPen(basePen);
        item->setBrush(baseBr);
    }
}

void DragCircle::rememberCurrentAsBase(QGraphicsRectItem* item)
{
    if (!item) return;
    // Запоминаем «как есть» — размер/перо/кисть
    item->setData(kRoleBaseRect,  item->rect());
    item->setData(kRoleBasePen,   QVariant::fromValue(item->pen()));
    item->setData(kRoleBaseBrush, QVariant::fromValue(item->brush()));
}

void DragCircle::setHoverStyle(bool hover)
{
    if (hover)
    {
        // Увеличиваем размер
        const qreal side = _baseRect.width() * 1.5;
        setRect(-side/2.0, -side/2.0, side, side);
        setBrush(QBrush(Qt::yellow));
        setPen(QPen(Qt::black, 1));
    }
    else
    {
        // Возвращаем базовый стиль
        restoreBaseStyle();
    }
    update();
}

void DragCircle::restoreBaseStyle()
{
    // Определяем, является ли родителем Circle
    bool isCircleHandle = (dynamic_cast<qgraph::Circle*>(parentItem()) != nullptr);

    if (isCircleHandle)
    {
        // Для кругов - невидимая точка
        setRect(_baseRect);
        setPen(Qt::NoPen);
        setBrush(Qt::NoBrush);
    }
    else
    {
        // Для других фигур - обычный серый стиль
        setRect(_baseRect);
        setPen(_basePen);
        setBrush(_baseBrush);
    }
    update();
}

void DragCircle::setHoverSizingEnabled(bool on)
{
    _hoverSizingEnabled = on;
}

bool DragCircle::containsPoint(const QPointF &point) const
{
    qreal dx = point.x() - m_center.x();
    qreal dy = point.y() - m_center.y();
    return (dx*dx + dy*dy) <= (m_radius * m_radius);
}

void DragCircle::setCenter(const QPointF &center)
{
    m_center = center;
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
        // если эту ручку прямо сейчас тянем мышью — не вмешиваемся
        if (scene() && scene()->mouseGrabberItem() == this)
            return QGraphicsRectItem::itemChange(change, value);

        // определяем родителя-фигуру
        if (auto* shape = dynamic_cast<Shape*>(parentItem()))
        {
            // если фигура сейчас программно раскладывает ручки — не триггерим рекурсию
            if (shape->handlesUpdateBlocked())
                return QGraphicsRectItem::itemChange(change, value);

            // перетаскивание мышью ИЛИ «призрак» активен → это реальное редактирование
            if (QApplication::mouseButtons() != Qt::NoButton || _ghostDriven)
            {
                shape->dragCircleMove(this);
                emit moved(this);
            }
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}


/*void DragCircle::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() == Qt::RightButton)
    {
        emit deleteRequested(this);
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
        applyHoverStyle(this, true);

    setZValue(100000);

    // if (Shape* parentShape = dynamic_cast<Shape*>(parentItem()))
    // {
    //     parentShape->raiseHandlesToTop();
    // }

    event->accept(); // Перехватываем событие, чтобы оно не дошло до родителя
    QGraphicsItem::mousePressEvent(event);
}

void DragCircle::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
    if (Shape* item = dynamic_cast<Shape*>(parentItem()))
    {
        item->dragCircleMove(this);
    }

    emit moved(this); // Испускаем сигнал при перемещении круга
}

void DragCircle::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
    if (Shape* item = dynamic_cast<Shape*>(parentItem()))
    {
        item->dragCircleRelease(this);
    }
    restoreBaseStyle();
    applyHoverStyle(this, false);
    emit released(this); // Испускаем сигнал при отпускании мыши
}*/

} // namespace qgraph
