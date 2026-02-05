#include "circle.h"
#include "drag_circle.h"

#include <QtGui>
#include <QGraphicsRectItem>
#include <cmath>
#include <QApplication>
#include <QCursor>

namespace qgraph {

struct SceneFilter;

Circle::Circle(QGraphicsScene* scene, const QPointF& scenePos)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true); // Включаем обработку событий наведения
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QGraphicsItem::ItemIsSelectable, true);


    _radius = 20;
    setRect(-_radius, -_radius, _radius * 2, _radius * 2);
    //setPos(_radius + 10, _radius + 10);
    setPos(scenePos.x(), scenePos.y());

    QPen pen = this->pen();
    pen.setColor(QColor(144, 238, 144));
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);

    setZLevel(5);
    scene->addItem(this);

    _circle = new DragCircle(scene);
    _circle->setParentItem(this);
    _circle->restoreBaseStyle();
    _circle->setVisible(false);
    _circle->setZValue(this->zValue() + 1);
    _circle->setPos(rect().width() / 2, 0);
    _circle->setHoverSizingEnabled(false);

    if (scene)
        attachSceneFilter(scene);

    // Создаем линии крестика
    _verticalLine = new QGraphicsLineItem(this);
    _horizontalLine = new QGraphicsLineItem(this);

    // Устанавливаем флаги, чтобы линии нельзя было перемещать
    _verticalLine->setFlag(QGraphicsItem::ItemIsMovable, false);
    _verticalLine->setFlag(QGraphicsItem::ItemIsSelectable, false);
    _verticalLine->setFlag(QGraphicsItem::ItemIsFocusable, false);
    _verticalLine->setAcceptedMouseButtons(Qt::NoButton);

    _horizontalLine->setFlag(QGraphicsItem::ItemIsMovable, false);
    _horizontalLine->setFlag(QGraphicsItem::ItemIsSelectable, false);
    _horizontalLine->setFlag(QGraphicsItem::ItemIsFocusable, false);
    _horizontalLine->setAcceptedMouseButtons(Qt::NoButton);

    // Отключаем обработку событий наведения для линий
    _verticalLine->setAcceptHoverEvents(false);
    _horizontalLine->setAcceptHoverEvents(false);
    updateCrossLines();
    _horizontalLine->setVisible(true);
    _verticalLine->setVisible(true);


    _ghostLocalPos = QPointF(rect().right(), rect().center().y());
}

Circle::~Circle()
{
    if (_circle)
    {
        if (_circle->scene())
            _circle->scene()->removeItem(_circle);
        delete _circle;
        _circle = nullptr;
    }

    _verticalLine = nullptr;
    _horizontalLine = nullptr;

    detachSceneFilter();
}

void Circle::setFrameScale(float newScale)
{
    float s = newScale / frameScale();
    QRectF r = rect();
    float x = r.x() * s;
    float y = r.y() * s;
    float w = r.width() * s;
    float h = r.height() * s;

    _radius *= s;

    prepareGeometryChange();
    setRect(x, y, w, h);
    setPos(pos().x() * s, pos().y() * s);

    _circle->setPos(w / 2, 0);
    _frameScale = newScale;

    updateSelectionRect();
    //changeSignal.emit_(this);
}

void Circle::dragCircleMove(DragCircle* circle)
{
    QPointF center = rect().center();
    float deltaX = qAbs(circle->x() - center.x());
    float deltaY = qAbs(circle->y() - center.y());

    _radius = std::sqrt(deltaX * deltaX + deltaY * deltaY);

    QRectF r;
    r.setLeft(center.x() - _radius);
    r.setTop (center.y() - _radius);
    r.setWidth (_radius * 2);
    r.setHeight(_radius * 2);

    updateSelectionRect();
    prepareGeometryChange();
    setRect(r);

    circleMoveSignal.emit_(this);
}

void Circle::dragCircleRelease(DragCircle* circle)
{
    if (circle->parentItem() == this)
    {
        QRectF r = rect();
        circle->setPos(r.width() + r.x(), r.width() / 2 + r.y());
        circleReleaseSignal.emit_(this);
    }
}

QPointF Circle::realCenter() const
{
    float s = frameScale();
    QPointF center = rect().center();
    QPointF center2 {(pos().x() + center.x()) / s,
                     (pos().y() + center.y()) / s};
    return center2;
}

void Circle::setRealCenter(const QPointF& val)
{
    float s = frameScale();
    QPointF center = rect().center();
    QPointF pos;
    pos.setX(val.x() * s - center.x());
    pos.setY(val.y() * s - center.y());

    prepareGeometryChange();
    setPos(pos.x(), pos.y());

    _circle->setPos(rect().width() / 2, 0);
    updateCrossLines();
    updateSelectionRect();
}

QPoint Circle::center() const
{
    QPointF c = realCenter();
    return {int(c.x()), int(c.y())};
}

int Circle::realRadius() const
{
    return _radius / frameScale();
}

void Circle::setRealRadius(int val)
{
    float s = frameScale();
    _radius = val * s;

    QPointF center = rect().center();

    QRectF r;
    r.setLeft(center.x() - _radius);
    r.setTop (center.y() - _radius);
    r.setWidth (_radius * 2);
    r.setHeight(_radius * 2);

    prepareGeometryChange();
    setRect(r);

    _circle->setPos(rect().width() / 2, 0);
    updateSelectionRect();
}

void Circle::updateHandlePosition()
{
    // Обновляем положение ручку относительно центра круга
    if (_circle)
    {
        _circle->setPos(rect().width() / 2, 0);
        _circle->setVisible(false);
    }
}

void Circle::updateHandlePosition(const QPointF& scenePos)
{
    if (!_circle) return;

    QPointF localPos = mapFromScene(scenePos);

    // Проверяем, находится ли курсор рядом с окружностью
    if (isCursorNearCircle(localPos))
    {
        QPointF center = rect().center();
        QPointF direction = localPos - center;
        qreal distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());

        if (distance > 0)
        {
            direction /= distance;
            direction *= _radius;
        }
        else
        {
            direction = QPointF(_radius, 0);
        }

        QPointF handlePos = center + direction;
        _circle->setPos(handlePos);
        _circle->setVisible(true);
        _circle->setHoverStyle(true);
    }
    else
    {
        // Курсор не рядом - скрываем ручку
        _circle->setVisible(false);
        _circle->setHoverStyle(false);
    }
}

QVariant Circle::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        update();
        updateSelectionRect();
    }
    return QGraphicsItem::itemChange(change, value);
}

void Circle::updateHandleZValue()
{
    if (_circle)
    {
        _circle->setZValue(this->zValue() + 1);
    }
}

void Circle::updateCrossLines()
{
    if (!_verticalLine || !_horizontalLine)
    {
        return;
    }
    QRectF r = rect();
    QPointF center = r.center();

    _verticalLine->setFlag(QGraphicsItem::ItemIgnoresTransformations,
                           this->flags() & QGraphicsItem::ItemIgnoresTransformations);
    _horizontalLine->setFlag(QGraphicsItem::ItemIgnoresTransformations,
                             this->flags() & QGraphicsItem::ItemIgnoresTransformations);

    // Длина линий крестика
    qreal lineLength = _radius * 0.5;

    // Вертикальная линия
    QLineF verticalLine(center.x(), center.y() - lineLength,
                        center.x(), center.y() + lineLength);

    // Горизонтальная линия
    QLineF horizontalLine(center.x() - lineLength, center.y(),
                          center.x() + lineLength, center.y());

    _verticalLine->setLine(verticalLine);
    _horizontalLine->setLine(horizontalLine);

    // Наследуем стиль пера от круга
    QPen pen = this->pen();
    _verticalLine->setPen(pen);
    _horizontalLine->setPen(pen);
}

void Circle::applyLineStyle(qreal lineWidth)
{
    // Обновляем стиль круга
    QPen pen = this->pen();
    pen.setWidthF(lineWidth);
    pen.setCosmetic(true);
    setPen(pen);

    if (_verticalLine)
        _verticalLine->setPen(pen);
    if (_horizontalLine)
        _horizontalLine->setPen(pen);
}

void Circle::raiseHandleToTop()
{
    if (_circle)
    {
        _circle->raiseToTop();
    }
}

QPointF Circle::pointOnCircle(const QRectF& rect, const QPointF& pos)
{
    const QPointF c = rect.center();
    QPointF v = pos - c;
    const qreal len = std::hypot(v.x(), v.y());
    if (len <= 1e-6) // курсор в центре - направим вправо
        return c + QPointF(rect.width()*0.5, 0.0);
    const qreal r = rect.width()*0.5;
    v /= len;
    return c + v * r;
}

void Circle::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete)
    {
        auto scene = this->scene();
        if (scene)
        {
            scene->removeItem(this);
            delete this; // Удаляем объект
        }
    }
    else if (event->key() == Qt::Key_B)
    {
        moveToBack();
        event->accept();
    }
    else
    {
        // Передаем событие дальше, если это не `Del`
        QGraphicsEllipseItem::keyPressEvent(event);
    }
}

void Circle::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QMenu menu;

    QAction* moveToBackAction = menu.addAction("Переместить на задний план (B)");

    // Добавляем разделитель
    menu.addSeparator();

    QString frameText = _selectionRectVisible ? "Скрыть рамку (W)" : "Показать рамку (W)";

    // Добавляем действие удаления
    QAction* deleteAction = menu.addAction("Удалить (Del)");
    QAction* toggleFrameAction = menu.addAction(frameText);

    QObject::connect(moveToBackAction, &QAction::triggered, [this]() {
        this->moveToBack();
    });

    QObject::connect(toggleFrameAction, &QAction::triggered, [this]() {
        this->toggleSelectionRect();
    });

    QObject::connect(deleteAction, &QAction::triggered, [this]() {
        auto scene = this->scene();
        if (scene) {
            scene->removeItem(this);
            delete this;
        }
    });

    menu.exec(event->screenPos());
    event->accept();
}

void Circle::moveToBack()
{
    if (!scene()) return;

    // Собираем информацию о всех фигурах
    QList<QGraphicsItem*> allItems;
    qreal photoZValue = 0;

    for (QGraphicsItem* item : scene()->items())
    {
        if (!item)
            continue;

        if (dynamic_cast<DragCircle*>(item))
            continue;

        if (item->zValue() <= 1 && item != this)
        {
            photoZValue = item->zValue();
        }

        allItems.append(item);
    }

    // Сортируем по z-value
    std::sort(allItems.begin(), allItems.end(),
              [](QGraphicsItem* a, QGraphicsItem* b) {
                  return a->zValue() < b->zValue();
              });

    // Находим индекс текущей фигуры
    int currentIndex = -1;
    for (int i = 0; i < allItems.size(); ++i)
    {
        if (allItems[i] == this)
        {
            currentIndex = i;
            break;
        }
    }

    if (currentIndex == -1) return;

    // Если фигура уже самая нижняя (после фотографии)
    if (currentIndex == 1) // индекс 0 - фотография, индекс 1 - самая нижняя фигура
    {
        // Перемещаем на самый верх
        QGraphicsItem* topItem = allItems.last();
        setZValue(topItem->zValue() + 1);
    }
    else
    {
        // Перемещаем на одну позицию ниже
        QGraphicsItem* lowerItem = allItems[currentIndex - 1];
        setZValue(lowerItem->zValue() - 0.1);
    }

    // Гарантируем, что остаемся над фотографией
    if (zValue() <= photoZValue)
    {
        setZValue(photoZValue + 0.1);
    }

    // Ручка всегда поверх
    if (_circle)
    {
        _circle->setZValue(10000);
    }

    scene()->update();
}

void Circle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QColor baseColor = pen().color();
    QColor highlightColor = baseColor;
    highlightColor.setAlpha(100);

    setBrush(highlightColor);

    if (!_circle)
        return;
    // Проверяем, находится ли курсор рядом с окружностью
    if (isCursorNearCircle(event->pos()))
    {
        // Обновляем позицию ручки в зависимости от положения курсора
        updateHandlePosition(mapToScene(event->pos()));

        const QPointF onCirc = pointOnCircle(rect(), event->pos());
        _circle->setHoverStyle(true);
        _circle->setPos(onCirc - _circle->boundingRect().center());
        _circle->setVisible(true);
    }
    else
    {
        _circle->setHoverStyle(false);
        _circle->setVisible(false);
    }
    _ghostLocalPos = pointOnCircle(rect(), event->pos());
    update();
}

void Circle::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    if (_circle)
    {
        // Проверяем, находится ли курсор рядом с окружностью
        if (isCursorNearCircle(event->pos()))
        {
            // Обновляем позицию ручки в зависимости от положения курсора
            updateHandlePosition(mapToScene(event->pos()));
            const QPointF onCirc = pointOnCircle(rect(), event->pos());
            _circle->setHoverStyle(true);
            _circle->setPos(onCirc - _circle->boundingRect().center());
            if (!_circle->isVisible())
                _circle->setVisible(true);
        }
        else
        {
            // Курсор не рядом с окружностью - скрываем ручку
            _circle->setVisible(false);
            _circle->setHoverStyle(false);

        }
    }
    _ghostLocalPos = pointOnCircle(rect(), event->pos());
    update();
}

void Circle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsEllipseItem::hoverLeaveEvent(event);
    setBrush(Qt::transparent);

    if (!_circle)
        return;

    _circle->setVisible(false);
    _circle->setHoverStyle(false);

    update();
}

void Circle::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(option);

    QPainterPath path;
    path.addEllipse(rect());

    if (isSelected() || isUnderMouse())
    {
        QColor fill = pen().color();
        fill.setAlpha(80);
        painter->save();
        painter->setBrush(fill);
        painter->setPen(Qt::NoPen);
        painter->drawPath(path);
        painter->restore();
    }

    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(pen());
    painter->drawPath(path);
    painter->restore();
}

struct Circle::SceneFilter : public QObject
{
    Circle* owner = nullptr;
    explicit SceneFilter(Circle* o) : owner(o) {}
    bool eventFilter(QObject* obj, QEvent* ev) override
    {
        if (!owner)
            return false;
        if (ev->type() == QEvent::GraphicsSceneMouseMove)
        {
            auto* me = static_cast<QGraphicsSceneMouseEvent*>(ev);
            owner->updateHandlePosition(me->scenePos());
        }
        return false;
    }
};

void Circle::attachSceneFilter(QGraphicsScene* s)
{
    if (!s || _sceneFilter)
        return;
    _sceneFilter = new SceneFilter(this);
    s->installEventFilter(_sceneFilter);
}

void Circle::detachSceneFilter()
{
    if (!_sceneFilter)
        return;
    if (scene())
        scene()->removeEventFilter(_sceneFilter);
    delete _sceneFilter;
    _sceneFilter = nullptr;
}

void Circle::updateSelectionRect()
{
    if (!_selectionRect)
    {
        _selectionRect = new QGraphicsRectItem(this);
        _selectionRect->setBrush(Qt::NoBrush);

        QPen pen(Qt::DashLine);
        pen.setWidthF(0.0);
        pen.setColor(QColor(220, 220, 220));
        _selectionRect->setPen(pen);

        _selectionRect->setFlag(QGraphicsItem::ItemIsMovable, false);
        _selectionRect->setFlag(QGraphicsItem::ItemIsSelectable, false);
        _selectionRect->setAcceptedMouseButtons(Qt::NoButton);
        _selectionRect->setAcceptHoverEvents(false);

        _selectionRect->setVisible(false);
    }

    _selectionRect->setRect(boundingRect());

    const bool visible = isSelected() && _selectionRectVisible;
    _selectionRect->setVisible(visible);
    if (visible)
        _selectionRect->setZValue(zValue() + 0.5);
}

void Circle::toggleSelectionRect()
{
    _selectionRectVisible = !_selectionRectVisible;
    updateSelectionRect();
}

bool Circle::isCursorNearCircle(const QPointF& cursorPos) const
{
    // Преобразуем координаты в систему сцены
    const QPointF sceneCenter = mapToScene(rect().center());
    const QPointF sceneCursorPos = mapToScene(cursorPos);

    const qreal radius = rect().width() / 2.0;
    const qreal sceneRadius = radius * frameScale(); // Учитываем масштаб

    // Расстояние от центра до курсора в сцене
    const qreal distanceToCenter = QLineF(sceneCenter, sceneCursorPos).length();

    // Определяем зону вокруг окружности с учетом масштаба
    const qreal margin = 10.0 * frameScale();

    // Проверяем, находится ли курсор в зоне вокруг окружности
    return std::abs(distanceToCenter - sceneRadius) <= margin;
}

QVariant Circle::saveState() const
{
    QVariantMap m;
    m["type"]    = "circle";
    m["center"]  = realCenter();
    m["radius"]  = realRadius();
    m["z"]       = int(zValue());
    m["visible"] = isVisible();
    return m;
}

void Circle::loadState(const QVariant& v)
{
    const auto m = v.toMap();
    if (m.contains("center"))  setRealCenter(m["center"].toPointF());
    if (m.contains("radius"))  setRealRadius(m["radius"].toInt());
    if (m.contains("z"))       setZValue(m["z"].toInt());
    if (m.contains("visible")) setVisible(m["visible"].toBool());

    updateHandlePosition();
    updateCrossLines();
    raiseHandleToTop();
    update();
}

} // namespace qgraph
