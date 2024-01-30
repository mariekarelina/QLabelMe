#include "polyline.h"
#include <QPainter>
#include <QPen>
#include <QKeyEvent>
#include <QRectF>
#include <QGraphicsSceneMouseEvent>

namespace qgraph {

Polyline::Polyline(QGraphicsScene* scene, const QPointF& scenePos)
    : _isClosed(false)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true); // Включаем обработку событий наведения

    QPen pen = this->pen();
    pen.setColor(QColor(0, 0, 255));
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);

    setZValue(1);
    scene->addItem(this);

    // Создаем начальную точку (DragCircle)
    DragCircle* startCircle = new DragCircle(scene);
    startCircle->setParentItem(this);
    startCircle->setVisible(true);
    startCircle->setPos(mapFromScene(scenePos));
    //startCircle->setPos(scenePos.x(), scenePos.y()); // Начальная позиция
    _circles.append(startCircle);

    // Подключаем обработчики событий для точки
    connect(startCircle, &DragCircle::moved, this, &Polyline::dragCircleMove);
    connect(startCircle, &DragCircle::released, this, &Polyline::dragCircleRelease);

    connect(startCircle, &DragCircle::deleteRequested, this, &Polyline::handlePointDeletion);

    updatePath(); // Обновляем начальный путь

//    _circle1 = new DragCircle(scene);
//    _circle1->setParentItem(this);
//    _circle1->setVisible(true);
//    _circle1->setPos(0, 0);

//    _circle2 = new DragCircle(scene);
//    _circle2->setParentItem(this);
//    _circle2->setVisible(true);
//    _circle2->setPos(50, 0);

//    _circle3 = new DragCircle(scene);
//    _circle3->setParentItem(this);
//    _circle3->setVisible(true);
//    _circle3->setPos(100, 50);

//    _circle4 = new DragCircle(scene);
//    _circle4->setParentItem(this);
//    _circle4->setVisible(true);
//    _circle4->setPos(150, 0);

//    // Соединяем сигналы перемещения кругов с обновлением пути
//    connect(_circle1, &DragCircle::moved, this, &Polyline::dragCircleMove);
//    connect(_circle2, &DragCircle::moved, this, &Polyline::dragCircleMove);
//    connect(_circle3, &DragCircle::moved, this, &Polyline::dragCircleMove);
//    connect(_circle4, &DragCircle::moved, this, &Polyline::dragCircleMove);

//    connect(_circle1, &DragCircle::released, this, &Polyline::dragCircleRelease);
//    connect(_circle2, &DragCircle::released, this, &Polyline::dragCircleRelease);
//    connect(_circle3, &DragCircle::released, this, &Polyline::dragCircleRelease);
//    connect(_circle4, &DragCircle::released, this, &Polyline::dragCircleRelease);

//    updatePath(); // Инициализируем начальный путь
}

void Polyline::setFrameScale(float newScale)
{
    float s = newScale / _frameScale;

    for (DragCircle* circle : _circles)
    {
        circle->setPos(circle->pos() * s); // Масштабируем точки
    }

    _frameScale = newScale;
    updatePath(); // Обновляем путь
}

void Polyline::setRealSceneRect(const QRectF& r)
{
    float s = frameScale();

    // Задаем новую позицию полилинии
    //QPointF pos = QPointF(r.left() * s, r.top() * s);

    // Масштабируем позиции ручек по новому масштабу
//    _circle1->setPos(pos + (_circle1->pos() - QPointF(0, 0)) * s);
//    _circle2->setPos(pos + (_circle2->pos() - QPointF(0, 0)) * s);
//    _circle3->setPos(pos + (_circle3->pos() - QPointF(0, 0)) * s);
//    _circle4->setPos(pos + (_circle4->pos() - QPointF(0, 0)) * s);

    // Корректируем масштаб
    _frameScale = s;

    // Обновляем путь
    updatePath();
}

void Polyline::updateHandlePosition()
{
//    for (auto circle : _circles)
//    {
//        circle->moveBy(delta.x(), delta.y());
//    }
}

void Polyline::addPoint(const QPointF& position, QGraphicsScene* scene)
{
    // Проверяем, совпадает ли последняя точка с новой
    if (!_circles.isEmpty())
    {
        QPointF lastPoint = _circles.last()->scenePos();
        if (QLineF(lastPoint, position).length() < 0.1) // Учитываем погрешность (например, 0.1)
        {
            return; // Новая точка не добавляется
        }
    }

    // Создаем новую точку (DragCircle)
    DragCircle* newCircle = new DragCircle(scene);
    newCircle->setParentItem(this);
    newCircle->setVisible(true);
    //newCircle->setPos(position); // Устанавливаем позицию новой точки
    QPointF localPos = mapFromScene(position);
    newCircle->setPos(localPos);
    _circles.append(newCircle);

    updateConnections();
    updatePath();
}


void Polyline::removePoint(QPointF position)
{
    if (_circles.size() <= 2)
    {
        // Если точек меньше двух, не удаляем последние точки, чтобы сохранить полилинию
        return;
    }

    // Поиск ближайшей точки
    DragCircle* closestCircle = nullptr;
    qreal minDistance = std::numeric_limits<qreal>::max();

    for (DragCircle* circle : _circles)
    {
        qreal distance = QLineF(circle->pos(), mapFromScene(position)).length();
        if (distance < minDistance)
        {
            minDistance = distance;
            closestCircle = circle;
        }
    }

    // Удаляем круг, если он есть
    if (closestCircle && minDistance < 10.0)
    {  // 10.0 — радиус точности зоны удаления
        handlePointDeletion(closestCircle); // Используем новый метод
    }
}

void Polyline::insertPoint(QPointF position)
{
//    if (_circles.size() < 2)
//    {
//        return;  // Если точек меньше двух, вставка невозможна
//    }

//    // Поиск ближайшего сегмента
//    int insertIndex = -1;
//    qreal minDistance = std::numeric_limits<qreal>::max();
//    QPointF newPoint;

//    for (int i = 0; i < _circles.size() - 1; ++i)
//    {
//        QPointF p1 = _circles[i]->pos();
//        QPointF p2 = _circles[i + 1]->pos();

//        // Вычисляем расстояние до сегмента
//        QLineF segment(p1, p2);
//        qreal distance = segment.normalVector().lengthTo(position);

//        if (distance < minDistance) {
//            minDistance = distance;
//            insertIndex = i;
//            newPoint = segment.pointAt(segment.projectedPointRatio(position));
//        }
//    }

//    // Вставляем новую точку, если нашли допустимый сегмент
//    if (insertIndex != -1 && minDistance < 10.0)
//    {  // 10.0 — радиус точности добавления
//        addPoint(mapToScene(newPoint), scene(), insertIndex + 1);
//    }
}


void Polyline::closePolyline()
{
    if (_circles.size() > 2 && !_isClosed)
   {
       _isClosed = true;

       // Для замкнутой полилинии просто устанавливаем флаг
       // и обновляем соединения между точками
       updateConnections();
       updatePath();
   }
}

void Polyline::updatePath()
{
    if (_circles.isEmpty())
        return;

    QPainterPath path;
    path.moveTo(_circles[0]->pos());

    // Рисуем линию через все точки
    for (int i = 1; i < _circles.size(); ++i)
    {
        path.lineTo(_circles[i]->pos());
    }

    // Если полилиния замкнута, соединяем последнюю точку с первой
    if (_isClosed && _circles.size() > 1)
    {
        path.lineTo(_circles[0]->pos());
        path.closeSubpath();
    }

    setPath(path);
}

QVariant Polyline::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemPositionChange)
    {
        QPointF delta = value.toPointF() - pos(); // Разница между новой и старой позицией

        // Корректируем положение ручек
        for (auto circle : _circles)
        {
            circle->moveBy(delta.x(), delta.y());
        }
    }
    return QGraphicsPathItem::itemChange(change, value);
}


void Polyline::handlePointDeletion(DragCircle* circle)
{
    // Не позволяем удалять точки, если их меньше 3 в замкнутой полилинии
    if (_isClosed && _circles.size() <= 3)
        return;

    // Не позволяем удалять точки, если их меньше 2 в незамкнутой
    if (!_isClosed && _circles.size() <= 2)
        return;

    if (_circles.contains(circle))
    {
        _circles.removeOne(circle);
        scene()->removeItem(circle);
        delete circle;

        // Если удалили точку из замкнутой полилинии и осталось 2 точки,
        // автоматически размыкаем её
        if (_isClosed && _circles.size() <= 2)
            _isClosed = false;

        updatePath();
        updateConnections();
    }
}

void Polyline::keyPressEvent(QKeyEvent* event)
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
    else
    {
        // Передаем событие дальше, если это не `Del`
        QGraphicsPathItem::keyPressEvent(event);
    }
}

void Polyline::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    // Устанавливаем бледный цвет заливки при наведении
    setBrush(QColor(2, 200, 255, 150)); // Бледно-красный с прозрачностью
    update(); // Обновляем отображение
}

void Polyline::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    // Возвращаем прозрачную заливку при уходе курсора
    setBrush(Qt::transparent);
    update(); // Обновляем отображение
}

void Polyline::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    // Проверяем, не было ли клика по точке-ручке
    QGraphicsItem* item = scene()->itemAt(event->scenePos(), QTransform());
    if (dynamic_cast<DragCircle*>(item))
    {
        event->ignore(); // Игнорируем событие, если клик был по точке
        return;
    }

    QMenu menu;
    QAction* addPointAction = menu.addAction("Добавить точку");

    // Подключаем действие добавления точки
    connect(addPointAction, &QAction::triggered, [this, event]()
    {
        // Получаем позицию клика в координатах полилинии
        QPointF clickPos = mapFromScene(event->scenePos());

        // Находим ближайший сегмент для вставки
        int insertIndex = findClosestSegment(clickPos);
        if (insertIndex != -1)
        {
            // Вставляем точку в найденный сегмент
            insertPointAtSegment(insertIndex, clickPos);
        }
    });

    menu.exec(event->screenPos());
    event->accept();
}


int Polyline::findClosestSegment(const QPointF& pos) const
{
    if (_circles.size() < 2) return -1;

    qreal minDistance = std::numeric_limits<qreal>::max();
    int closestSegment = -1;

    for (int i = 0; i < _circles.size() - 1; ++i) {
        QLineF segment(_circles[i]->pos(), _circles[i+1]->pos());

        // Реализация distanceToPoint для Qt 5
        qreal distance = 0;
        if (segment.length() > 0)
        {
            qreal u = ((pos.x() - segment.p1().x()) * (segment.p2().x() - segment.p1().x()) +
                      (pos.y() - segment.p1().y()) * (segment.p2().y() - segment.p1().y())) /
                     (segment.length() * segment.length());

            u = qBound(0.0, u, 1.0); // Ограничиваем u между 0 и 1

            QPointF closestPoint = segment.p1() + u * (segment.p2() - segment.p1());
            distance = QLineF(pos, closestPoint).length();
        }
        else
        {
            // Если сегмент нулевой длины (точки совпадают)
            distance = QLineF(pos, segment.p1()).length();
        }

        if (distance < minDistance)
        {
            minDistance = distance;
            closestSegment = i;
        }
    }

    return (minDistance < 15.0) ? closestSegment : -1;
}

void Polyline::insertPointAtSegment(int segmentIndex, const QPointF& pos)
{
    if (segmentIndex < 0 || segmentIndex >= _circles.size() - 1)
        return;

    // Создаем новую точку
    DragCircle* newCircle = new DragCircle(scene());
    newCircle->setParentItem(this);
    newCircle->setPos(pos);

    // Вставляем точку в нужное место
    _circles.insert(segmentIndex + 1, newCircle);

    // Обновляем соединения
    updateConnections();
    updatePath();
}

void Polyline::dragCircleMove(DragCircle* circle)
{
    Q_UNUSED(circle);
    updatePath(); // Обновляем путь при любом перемещении
}

void Polyline::dragCircleRelease(DragCircle* circle)
{
    Q_UNUSED(circle);
    emit lineChanged(this); // Сигнал о завершении изменения
}


void Polyline::updateConnections()
{
    // Отключаем все старые соединения
    for (DragCircle* circle : _circles)
    {
        disconnect(circle, nullptr, this, nullptr);
    }

    // Устанавливаем новые соединения для перемещения точек
    for (DragCircle* circle : _circles)
    {
        connect(circle, &DragCircle::moved, this, &Polyline::dragCircleMove);
        connect(circle, &DragCircle::released, this, &Polyline::dragCircleRelease);
        connect(circle, &DragCircle::deleteRequested, this, &Polyline::handlePointDeletion);
    }

    // Для замкнутой полилинии не создаем специальных соединений между
    // первой и последней точкой - они остаются независимыми
}

void Polyline::updateClosedState()
{
    if (_isClosed && _circles.size() > 2)
    {
        // Связываем первую и последнюю точки
        connect(_circles.first(), &DragCircle::moved, this, [this]() {
            if (_isClosed) _circles.last()->setPos(_circles.first()->pos());
        });

        connect(_circles.last(), &DragCircle::moved, this, [this]() {
            if (_isClosed) _circles.first()->setPos(_circles.last()->pos());
        });
    }
}

} // namespace qgraph
