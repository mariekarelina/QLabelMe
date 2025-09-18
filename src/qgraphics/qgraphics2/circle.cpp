#include "circle.h"
#include "drag_circle.h"

#include <QtGui>
#include <cmath>
#include <QApplication>
#include <QCursor>

namespace qgraph {

Circle::Circle(QGraphicsScene* scene, const QPointF& scenePos)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true); // Включаем обработку событий наведения
    setAcceptedMouseButtons(Qt::LeftButton);

    _radius = 20;
    setRect(-_radius, -_radius, _radius * 2, _radius * 2);
    //setPos(_radius + 10, _radius + 10);
    setPos(scenePos.x(), scenePos.y());

    QPen pen = this->pen();
    pen.setColor(QColor(255, 0, 0));
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);

    setZLevel(5);
    scene->addItem(this);

    _circle = new DragCircle(scene);
    _circle->setParent(this);
    _circle->restoreBaseStyle();
    _circle->setVisible(false);
    _circle->setZValue(this->zValue() + 1);
    _circle->setPos(rect().width() / 2, 0);
    _circle->setHoverSizingEnabled(false);

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
}

Circle::~Circle()
{
    // // Удаляем ручку, если она была создана
    if (_circle && _circle->scene())
    {
        _circle->scene()->removeItem(_circle);
        delete _circle;
    }
    if (_verticalLine)
    {
        if (_verticalLine->scene())
        {
            _verticalLine->scene()->removeItem(_verticalLine);
        }
        delete _verticalLine;
        _verticalLine = nullptr;
    }

    if (_horizontalLine)
    {
        if (_horizontalLine->scene())
        {
            _horizontalLine->scene()->removeItem(_horizontalLine);
        }
        delete _horizontalLine;
        _horizontalLine = nullptr;
    }
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

    // Преобразуем координаты сцены в локальные координаты круга
    QPointF localPos = mapFromScene(scenePos);
    QPointF center = rect().center();

    // Вычисляем вектор от центра к курсору
    QPointF direction = localPos - center;
    qreal distance = std::sqrt(direction.x() * direction.x() + direction.y() * direction.y());

    // Нормализуем вектор и умножаем на радиус
    if (distance > 0)
    {
        direction /= distance;
        direction *= _radius;
    }
    else
    {
        // Если курсор в центре, направляем вправо
        direction = QPointF(_radius, 0);
    }
    // Устанавливаем позицию ручки на окружности
    QPointF handlePos = center + direction;
    _circle->setPos(handlePos);

    // Убедимся, что ручка видима
    _circle->setVisible(true);
    _circle->setHoverStyle(true);
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

    // Длина линий крестика (можно регулировать)
    qreal lineLength = _radius * 0.5; // 80% от радиуса

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
    if (len <= 1e-6) // курсор в центре — направим вправо
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

    // Добавляем действие удаления
    QAction* deleteAction = menu.addAction("Удалить (Del)");

    QObject::connect(moveToBackAction, &QAction::triggered, [this]() {
        this->moveToBack();
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
        _circle->setVisible(false);
    }
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
}

void Circle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsEllipseItem::hoverLeaveEvent(event);
    setBrush(Qt::transparent);

    if (!_circle)
        return;

    _circle->setVisible(false);
    _circle->setHoverStyle(false);
}

void Circle::paint(QPainter* painter,
                   const QStyleOptionGraphicsItem* option,
                   QWidget* widget)
{
    // Сначала стандартная отрисовка окружности (контур, заливка, выделение и т.д.)
    QGraphicsEllipseItem::paint(painter, option, widget);
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

} // namespace qgraph
