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
    setAcceptHoverEvents(true);

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
    QPointF pos = QPointF(r.left() * s, r.top() * s);

    // Масштабируем позиции ручек по новому масштабу
    _circle1->setPos(pos + (_circle1->pos() - QPointF(0, 0)) * s);
    _circle2->setPos(pos + (_circle2->pos() - QPointF(0, 0)) * s);
    _circle3->setPos(pos + (_circle3->pos() - QPointF(0, 0)) * s);
    _circle4->setPos(pos + (_circle4->pos() - QPointF(0, 0)) * s);

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

//    // Вставляем точку в нужное место, если указан index
//    if (index < 0 || index >= _circles.size())
//    {
//        _circles.append(newCircle);
//    }
//    else
//    {
//        _circles.insert(index, newCircle);
//    }

    // Подключаем сигналы перемещения
//    connect(newCircle, &DragCircle::moved, this, &Polyline::dragCircleMove);
//    connect(newCircle, &DragCircle::released, this, &Polyline::dragCircleRelease);

//    connect(newCircle, &DragCircle::deleteRequested, this, &Polyline::handlePointDeletion);
    updateConnections();

    updatePath();
}

//void Polyline::addPoint(const QPointF& position, QGraphicsScene* scene)
//{
//    // Создаем новую точку (DragCircle)
//    DragCircle* newCircle = new DragCircle(scene);
//    newCircle->setParentItem(this);
//    newCircle->setVisible(true);
//    //newCircle->setPos(position); // Устанавливаем позицию новой точки
//    QPointF localPos = mapFromScene(position);
//    newCircle->setPos(localPos);
//    _circles.append(newCircle);

//    // Подключаем обработчики перемещения и завершения перемещения
//    connect(newCircle, &DragCircle::moved, this, &Polyline::dragCircleMove);
//    connect(newCircle, &DragCircle::released, this, &Polyline::dragCircleRelease);

//    updatePath(); // Обновляем путь при добавлении новой точки
//}

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
    if (closestCircle && minDistance < 10.0) {  // 10.0 — радиус точности зоны удаления
//        _circles.removeOne(closestCircle);
//        delete closestCircle;
//        updatePath();
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

//void Polyline::closePolyline()
//{
////    if (_circles.size() > 1 && !_isClosed)
////    {
////        _isClosed = true;
////        updateClosedState();
////        updatePath();
////    }
//    if (_circles.size() > 2 && !_isClosed)
//    {

//        // Проверяем, совпадает ли последняя точка с первой
//        QPointF firstPoint = _circles.first()->scenePos();
//        QPointF lastPoint = _circles.last()->scenePos();

//        if (QLineF(firstPoint, lastPoint).length() < 0.1) // Учитываем допустимую погрешность
//        {
//            // Если совпадают, просто замыкаем путь, не создавая новой точки
//            _isClosed = true;
//            updateClosedState();
//            updatePath();
//            return;
//        }
//        // Просто соединяем последнюю точку с первой, не добавляя новую
//        _isClosed = true;

//        // Удаляем последнюю точку (она будет заменена первой точкой)
//        DragCircle* lastCircle = _circles.last();
//        _circles.removeLast();
//        scene()->removeItem(lastCircle);
//        delete lastCircle;

//        // Синхронизируем перемещение первой и последней точек
//        connect(_circles.first(), &DragCircle::moved, this, [this]() {
//            if (_isClosed) _circles.last()->setPos(_circles.first()->pos());
//        });

////        connect(_circles.last(), &DragCircle::moved, this, [this]() {
////            if (_isClosed) _circles.first()->setPos(_circles.last()->pos());
////        });

//        updatePath();
//    }
//    else if (_circles.size() == 2 && !_isClosed)
//    {
//        // Для случая с двумя точками - просто соединяем их
//        _isClosed = true;
//        updatePath();
//    }
//}

//void Polyline::closePolyline()
//{
//    if (_circles.size() > 2 && !_isClosed)
//    {
//        // Убедимся, что путь замкнут корректно
//        QPointF firstPoint = _circles.first()->scenePos();
//        QPointF lastPoint = _circles.last()->scenePos();

//        if (QLineF(firstPoint, lastPoint).length() < 0.1) // Достаточно близки - просто замкнем путь
//        {
//            _isClosed = true;
//            updatePath(); // Обновляем путь для замкнутой полилинии
//            return;
//        }

//        // Если последняя точка не совпадает с первой, соединяем явно
//        _isClosed = true;

//        // Синхронизация перемещения точек
//        connect(_circles.first(), &DragCircle::moved, this, [this]() {
//            if (_isClosed) _circles.last()->setPos(_circles.first()->pos());
//        });

//        connect(_circles.last(), &DragCircle::moved, this, [this]() {
//            if (_isClosed) _circles.first()->setPos(_circles.last()->pos());
//        });

//        // Обновляем путь
//        updatePath();
//    }
//}

void Polyline::closePolyline()
{
    if (_circles.size() > 2 && !_isClosed)
    {
        _isClosed = true;

        // Устанавливаем двустороннюю синхронизацию между первой и последней точками
        DragCircle* firstCircle = _circles.first();
        DragCircle* lastCircle = _circles.last();

        connect(firstCircle, &DragCircle::moved, this, [this, lastCircle]() {
            if (_isClosed) {
                disconnect(lastCircle, &DragCircle::moved, nullptr, nullptr); // Предотвращаем зацикливание
                lastCircle->setPos(_circles.first()->pos());
                connect(lastCircle, &DragCircle::moved, this, [this]() {
                    if (_isClosed) _circles.first()->setPos(_circles.last()->pos());
                });
            }
        });

        connect(lastCircle, &DragCircle::moved, this, [this, firstCircle]() {
            if (_isClosed) {
                disconnect(firstCircle, &DragCircle::moved, nullptr, nullptr); // Предотвращаем зацикливание
                firstCircle->setPos(_circles.last()->pos());
                connect(firstCircle, &DragCircle::moved, this, [this]() {
                    if (_isClosed) _circles.last()->setPos(_circles.first()->pos());
                });
            }
        });

        updatePath(); // Обновляем путь, чтобы включить замыкающую линию
    }
}

void Polyline::updatePath()
{
//    if (_circles.isEmpty()) return;

//    QPainterPath path;
//    path.moveTo(_circles[0]->pos());

//    for (int i = 1; i < _circles.size(); ++i)
//    {
//        path.lineTo(_circles[i]->pos());
//    }

//    if (_isClosed && _circles.size() > 1)
//    {
//        path.lineTo(_circles[0]->pos()); // Замыкаем путь
//        path.closeSubpath();
//    }

//    setPath(path);
//    if (_circles.isEmpty()) return;

//    QPainterPath path;
//    path.moveTo(_circles[0]->pos());

//    // Рисуем основные сегменты
//    for (int i = 1; i < _circles.size(); ++i)
//    {
//        path.lineTo(_circles[i]->pos());
//    }

//    // Замыкаем путь, если полилиния закрыта
//    if (_isClosed && _circles.size() > 1)
//    {
//        path.lineTo(_circles[0]->pos());
//        path.closeSubpath();
//    }

//    setPath(path);

    if (_circles.isEmpty()) return;

    QPainterPath path;
    path.moveTo(_circles[0]->pos());

    // Строим линию между всеми точками
    for (int i = 1; i < _circles.size(); ++i) {
        path.lineTo(_circles[i]->pos());
    }

    // Если фигура замкнута, добавляем замыкающую линию
    if (_isClosed && _circles.size() > 1) {
        path.lineTo(_circles[0]->pos());
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

//void Polyline::handlePointDeletion(DragCircle* circle)
//{
////    if (_circles.size() <= 2 || _isClosed) // Не удаляем, если точек мало или фигура замкнута
////        return;

////    if (_circles.contains(circle))
////    {
////        _circles.removeOne(circle);
////        delete circle;
////        updatePath();
////    }
//    // Не удаляем, если точек меньше 3 или полилиния замкнута
//    if (_circles.size() < 3)
//    {
//        return;
//    }

//    // Находим индекс удаляемой точки
//    int index = _circles.indexOf(circle);
//    if (index == -1) return;

////    // Если это первая или последняя точка в замкнутой полилинии, нужно обновить связи
////    if (_isClosed && (index == 0 || index == _circles.size() - 1))
////    {
////        disconnect(_circles.first(), nullptr, this, nullptr);
////        disconnect(_circles.last(), nullptr, this, nullptr);
////    }

//    // Удаляем точку
//    _circles.removeAt(index);
//    scene()->removeItem(circle);
//    delete circle;

//    // Обновляем путь
//    updatePath();

//    // Если удалили первую или последнюю точку у незамкнутой полилинии,
//    // нужно обновить связи
//    if (!_isClosed && (index == 0 || index == _circles.size())) {
//        updateConnections();
//    }
//}

void Polyline::handlePointDeletion(DragCircle* circle)
{
    if (_circles.size() <= 3 && _isClosed) {
        // Нельзя удалить точки из замкнутой фигуры, если их меньше трех
        return;
    }

    if (_circles.contains(circle)) {
        _circles.removeOne(circle);
        scene()->removeItem(circle);
        delete circle;

        // Если полилиния была замкнута, проверяем состояние
        if (_isClosed) {
            _isClosed = false; // Размыкаем фигуру, если точек стало меньше трех
        }

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
        if (segment.length() > 0) {
            qreal u = ((pos.x() - segment.p1().x()) * (segment.p2().x() - segment.p1().x()) +
                      (pos.y() - segment.p1().y()) * (segment.p2().y() - segment.p1().y())) /
                     (segment.length() * segment.length());

            u = qBound(0.0, u, 1.0); // Ограничиваем u между 0 и 1

            QPointF closestPoint = segment.p1() + u * (segment.p2() - segment.p1());
            distance = QLineF(pos, closestPoint).length();
        } else {
            // Если сегмент нулевой длины (точки совпадают)
            distance = QLineF(pos, segment.p1()).length();
        }

        if (distance < minDistance) {
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
    //QPointF pos = circle->pos();
    //QRectF bounds = rect(); // Ограничения внутри ректанга
    //Q_UNUSED(bounds);

    // Ограничиваем минимальную длину между соседними точками
//    if (circle == _circle2) {
//        if (pos.x() < _circle1->pos().x() + _minSegmentLength)
//        {
//            pos.setX(_circle1->pos().x() + _minSegmentLength);
//        }
//        if (pos.x() > _circle3->pos().x() - _minSegmentLength)
//        {
//            pos.setX(_circle3->pos().x() - _minSegmentLength);
//        }
//    }
//    else if (circle == _circle3)
//    {
//        if (pos.x() < _circle2->pos().x() + _minSegmentLength)
//        {
//            pos.setX(_circle2->pos().x() + _minSegmentLength);
//        }
//        if (pos.x() > _circle4->pos().x() - _minSegmentLength)
//        {
//            pos.setX(_circle4->pos().x() - _minSegmentLength);
//        }
//    }

    Q_UNUSED(circle);
    updatePath(); // Обновляем путь при любом перемещении

    //circle->setPos(pos); // Устанавливаем скорректированную позицию
    //updatePath(); // Обновляем путь
}

void Polyline::dragCircleRelease(DragCircle* circle)
{
    Q_UNUSED(circle);
    emit lineChanged(this); // Сигнал о завершении изменения
}

//void Polyline::updatePath()
//{
//    QPainterPath path;
//    path.moveTo(_circle1->pos());
//    path.lineTo(_circle2->pos());
//    path.lineTo(_circle3->pos());
//    path.lineTo(_circle4->pos());
//    setPath(path);
//}

//void Polyline::updateConnections()
//{
//    if (_circles.size() < 2) return;

//    // Отключаем все старые соединения
//    for (DragCircle* circle : _circles) {
////        disconnect(circle, &DragCircle::moved, this, &Polyline::dragCircleMove);
////        disconnect(circle, &DragCircle::released, this, &Polyline::dragCircleRelease);
////        disconnect(circle, &DragCircle::deleteRequested, this, &Polyline::handlePointDeletion);
//        disconnect(circle, nullptr, this, nullptr); // Убираем все сигналы для точек
//    }

//    // Устанавливаем новые соединения
//    for (DragCircle* circle : _circles) {
//        connect(circle, &DragCircle::moved, this, &Polyline::dragCircleMove);
//        connect(circle, &DragCircle::released, this, &Polyline::dragCircleRelease);
//        connect(circle, &DragCircle::deleteRequested, this, &Polyline::handlePointDeletion);
//    }

//    if (_isClosed && _circles.size() > 2) {
//            // Связываем первую и последнюю точки, если фигура замкнутая
//            connect(_circles.first(), &DragCircle::moved, this, [this]() {
//                if (_isClosed) _circles.last()->setPos(_circles.first()->pos());
//            });

//            connect(_circles.last(), &DragCircle::moved, this, [this]() {
//                if (_isClosed) _circles.first()->setPos(_circles.last()->pos());
//            });
//    }
//}

void Polyline::updateConnections()
{
    if (_circles.size() < 2) return;

    // Отключаем все старые соединения
    for (DragCircle* circle : _circles) {
        disconnect(circle, nullptr, this, nullptr);
    }

    // Устанавливаем новые соединения
    for (DragCircle* circle : _circles) {
        connect(circle, &DragCircle::moved, this, &Polyline::dragCircleMove);
        connect(circle, &DragCircle::released, this, &Polyline::dragCircleRelease);
        connect(circle, &DragCircle::deleteRequested, this, &Polyline::handlePointDeletion);
    }

    if (_isClosed && _circles.size() > 2) {
        DragCircle* firstCircle = _circles.first();
        DragCircle* lastCircle = _circles.last();

        // Синхронизация последней и первой точек
        connect(firstCircle, &DragCircle::moved, this, [this, lastCircle]() {
            if (_isClosed) lastCircle->setPos(_circles.first()->pos());
        });

        connect(lastCircle, &DragCircle::moved, this, [this, firstCircle]() {
            if (_isClosed) firstCircle->setPos(_circles.last()->pos());
        });
    }
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
