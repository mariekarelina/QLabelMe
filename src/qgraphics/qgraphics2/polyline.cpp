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
    QPen circlePen(Qt::red);
    circlePen.setWidth(2);
    startCircle->setPen(circlePen);
    startCircle->setBrush(Qt::white);
    startCircle->setRect(QRectF(-5, -5, 10, 10));
    startCircle->setPos(mapFromScene(scenePos));

    //startCircle->setPos(scenePos.x(), scenePos.y()); // Начальная позиция
    _circles.append(startCircle);

    updateConnections();
    updatePath(); // Обновляем начальный путь
}

Polyline::~Polyline()
{
    qDeleteAll(pointNumbers);    // Удаляем номера
    qDeleteAll(numberBackgrounds); // Удаляем фоны
    pointNumbers.clear();
    numberBackgrounds.clear();
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
        if (QLineF(lastPoint, position).length() < 0.1)
        {
            return;
        }
    }

    // Создаем новую точку (DragCircle)
    DragCircle* newCircle = new DragCircle(scene);
    newCircle->setParentItem(this);
    newCircle->setVisible(true);
    if (!_circles.isEmpty())
    {
        DragCircle* existingCircle = _circles.first();
        newCircle->setPen(existingCircle->pen());
        newCircle->setBrush(existingCircle->brush());
        newCircle->setRect(existingCircle->rect());
        newCircle->setZValue(existingCircle->zValue());
    }

    QPointF localPos = mapFromScene(position);
    newCircle->setPos(localPos);
    DragCircle::rememberCurrentAsBase(newCircle);
    _circles.append(newCircle);

    updateConnections();
    updatePath();
    updatePointNumbers();
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
    if (closestCircle && minDistance < 10.0) // 10.0 — радиус точности зоны удаления
    {
        handlePointDeletion(closestCircle);
    }
}

void Polyline::insertPoint(QPointF position)
{
    if (_circles.size() < 2)
    {
        return;  // Если точек меньше двух, вставка невозможна
    }

    // Преобразуем позицию в локальные координаты
    QPointF localPos = mapFromScene(position);

    // Поиск ближайшего сегмента
    int insertIndex = -1;
    qreal minDistance = std::numeric_limits<qreal>::max();
    QPointF closestPoint;

    for (int i = 0; i < _circles.size() - 1; ++i)
    {
        QPointF p1 = _circles[i]->pos();
        QPointF p2 = _circles[i + 1]->pos();

        // Вычисляем расстояние до сегмента и ближайшую точку
        qreal u = ((localPos.x() - p1.x()) * (p2.x() - p1.x()) +
                   (localPos.y() - p1.y()) * (p2.y() - p1.y())) /
                  ((p2.x() - p1.x()) * (p2.x() - p1.x()) +
                   (p2.y() - p1.y()) * (p2.y() - p1.y()));

        u = qBound(0.0, u, 1.0); // Ограничиваем u между 0 и 1

        QPointF projectedPoint = p1 + u * (p2 - p1);
        qreal distance = QLineF(localPos, projectedPoint).length();

        if (distance < minDistance)
        {
            minDistance = distance;
            insertIndex = i;
            closestPoint = projectedPoint;
        }
    }

    // Проверяем замкнутую полилинию (последний сегмент)
    if (_isClosed && _circles.size() > 2)
    {
        QPointF p1 = _circles.last()->pos();
        QPointF p2 = _circles.first()->pos();

        qreal u = ((localPos.x() - p1.x()) * (p2.x() - p1.x()) +
                   (localPos.y() - p1.y()) * (p2.y() - p1.y())) /
                  ((p2.x() - p1.x()) * (p2.x() - p1.x()) +
                   (p2.y() - p1.y()) * (p2.y() - p1.y()));

        u = qBound(0.0, u, 1.0);
        QPointF projectedPoint = p1 + u * (p2 - p1);
        qreal distance = QLineF(localPos, projectedPoint).length();

        if (distance < minDistance)
        {
            minDistance = distance;
            insertIndex = _circles.size() - 1;
            closestPoint = projectedPoint;
        }
    }

    // Вставляем новую точку, если нашли допустимый сегмент
    if (insertIndex != -1 && minDistance < 20.0)
    {
        // Создаем новую точку
        DragCircle* newCircle = new DragCircle(scene());
        newCircle->setParentItem(this);
        newCircle->setVisible(true);

        if (!_circles.isEmpty())
        {
            DragCircle* existingCircle = _circles.first();
            newCircle->setBaseStyle(existingCircle->baseColor(), existingCircle->baseSize());
            newCircle->setSelectedHandleColor(existingCircle->selectedHandleColor());
            newCircle->restoreBaseStyle();
        }
        newCircle->setPos(closestPoint);

        // Вставляем точку в нужное место
        _circles.insert(insertIndex + 1, newCircle);

        updateConnections();
        updatePath();
        //updatePointNumbers();
    }
    updatePointNumbers();
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
    updatePointNumbers();
}

QVariant Polyline::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemPositionHasChanged)
    {
        updatePointNumbers();
    }
    return QGraphicsPathItem::itemChange(change, value);
}


void Polyline::handlePointDeletion(DragCircle* circle)
{
    if (!circle || !scene()) return;

    // Не позволяем удалять точки, если их меньше 3 в замкнутой полилинии
    if (_isClosed && _circles.size() <= 3)
        return;

    // Не позволяем удалять точки, если их меньше 2 в незамкнутой
    if (!_isClosed && _circles.size() <= 2)
        return;

    if (_circles.contains(circle))
    {
        circle->disconnect();
        _circles.removeOne(circle);
        scene()->removeItem(circle);
        delete circle;
        circle = nullptr;

        // Если удалили точку из замкнутой полилинии и осталось 2 точки,
        // автоматически размыкаем её
        if (_isClosed && _circles.size() <= 2)
            _isClosed = false;

        updatePath();
        updateConnections();
        updatePointNumbers();
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
    else if (event->key() == Qt::Key_E)
    {
        rotatePointsCounterClockwise();
        event->accept();
    }
    else if (event->key() == Qt::Key_R)
    {
        rotatePointsClockwise();
        event->accept();
    }
    else if (event->key() == Qt::Key_N)
    {
        togglePointNumbers();
        event->accept();
    }
    else if (event->key() == Qt::Key_B)
    {
        moveToBack();
        event->accept();
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
    QColor baseColor = pen().color();
    QColor highlightColor = baseColor;
    highlightColor.setAlpha(100);

    setBrush(highlightColor);
    update();
}

void Polyline::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    setBrush(Qt::transparent);
    update();
}

void Polyline::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
    QGraphicsItem* item = scene()->itemAt(event->scenePos(), QTransform());
    if (DragCircle* circle = dynamic_cast<DragCircle*>(item))
    {
        // Контекстное меню для точки
        QMenu menu;
        QAction* deleteAction = menu.addAction("Удалить точку");
        // Подключаем удаление точки
        QObject::connect(deleteAction, &QAction::triggered, [this, circle]() {
            this->handlePointDeletion(circle);
        });

        menu.exec(event->screenPos());
        event->accept();
        return;
    }

    QMenu menu;

    QAction* cwAction = menu.addAction("Сдвиг по часовой (R)");
    QAction* ccwAction = menu.addAction("Сдвиг против часовой (E)");
    QString numbersText = _pointNumbersVisible ? "Скрыть нумерацию (N)" : "Показать нумерацию (N)";
    QAction* moveToBackAction = menu.addAction("Переместить на задний план (B)");
    QAction* toggleNumbersAction = menu.addAction(numbersText);

    // Добавляем разделитель
    menu.addSeparator();

    // Добавляем действие удаления
    QAction* deleteAction = menu.addAction("Удалить (Del)");

    // Подключаем действия
    QObject::connect(cwAction, &QAction::triggered, [this]() {
        this->rotatePointsClockwise();
    });

    QObject::connect(ccwAction, &QAction::triggered, [this]() {
        this->rotatePointsCounterClockwise();
    });

    QObject::connect(toggleNumbersAction, &QAction::triggered, [this]() {
        this->togglePointNumbers();
    });

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
    updatePointNumbers();
}

void Polyline::dragCircleRelease(DragCircle* circle)
{
    Q_UNUSED(circle);
    updatePath(); // Обновляем путь при любом перемещении
    updatePointNumbers(); // Обновляем позиции номеров
    // emit lineChanged(this); // Сигнал о завершении изменения
}

void Polyline::updateHandlesZValue()
{
    for (DragCircle* circle : _circles)
    {
        circle->setZValue(this->zValue() + 1);
    }
}

void Polyline::raiseHandlesToTop()
{
    for (DragCircle* circle : _circles)
    {
        circle->raiseToTop();
        circle->setVisible(true);
    }
}

void Polyline::moveToBack()
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

    // Ручки всегда поверх
    for (QGraphicsItem* child : childItems())
    {
        if (DragCircle* circle = dynamic_cast<DragCircle*>(child))
        {
            circle->setZValue(10000);
        }
    }

    scene()->update();
}

void Polyline::updateConnections()
{
    // // Отключаем все старые соединения
    // for (DragCircle* circle : _circles)
    // {
    //     QObject::disconnect(circle, nullptr, nullptr, nullptr);
    // }
    // // Устанавливаем новые соединения для всех точек
    // for (DragCircle* circle : _circles)
    // {
    //     QObject::connect(circle, &DragCircle::moved, [this, circle]() {
    //         this->dragCircleMove(circle);
    //     });

    //     QObject::connect(circle, &DragCircle::released, [this, circle]() {
    //         this->dragCircleRelease(circle);
    //     });

    //     QObject::connect(circle, &DragCircle::deleteRequested, [this, circle]() {
    //         this->handlePointDeletion(circle);
    //     });
    // }
}

void Polyline::updateClosedState()
{
    // if (_isClosed && _circles.size() > 2)
    // {
    //     // Удаляем все предыдущие соединения
    //     for (DragCircle* circle : _circles)
    //     {
    //         QObject::disconnect(circle, nullptr, nullptr, nullptr);
    //     }

    //     // Устанавливаем новые соединения с помощью лямбда-функций
    //     QObject::connect(_circles.first(), &DragCircle::moved, [this]() {
    //         if (_isClosed && !_circles.isEmpty())
    //             _circles.last()->setPos(_circles.first()->pos());
    //     });

    //     QObject::connect(_circles.last(), &DragCircle::moved, [this]() {
    //         if (_isClosed && !_circles.isEmpty())
    //             _circles.first()->setPos(_circles.last()->pos());
    //     });

    //     // Восстанавливаем остальные соединения
    //     updateConnections();
    // }
}

void Polyline::updatePointNumbersAfterReorder()
{
    // Удаляем старые номера и фоны
    qDeleteAll(pointNumbers);
    qDeleteAll(numberBackgrounds);
    pointNumbers.clear();
    numberBackgrounds.clear();

    // Создаем новые номера с обновленными индексами
    updatePointNumbers();
}

void Polyline::updatePointNumbers()
{
    // Удаляем старые номера и фоны
    qDeleteAll(pointNumbers);
    qDeleteAll(numberBackgrounds);
    pointNumbers.clear();
    numberBackgrounds.clear();

    if (!_pointNumbersVisible) return;

    for (int i = 0; i < _circles.size(); ++i)
    {
        QPointF circlePos = _circles[i]->pos();
        QPointF direction(0, -1); // Направление по умолчанию (вверх)

        // Вычисляем нормаль к сегменту
        if (_circles.size() > 1)
        {
            if (i == 0) // Первая точка
            {
                QPointF nextPos = _circles[1]->pos();
                QPointF segment = nextPos - circlePos;

                if (segment.manhattanLength() > 0)
                {
                    // Вычисляем нормаль (перпендикуляр к сегменту)
                    direction = QPointF(-segment.y(), segment.x());
                    direction = direction / QLineF(0, 0, direction.x(), direction.y()).length();

                    // Проверяем оба направления и выбираем то, которое не пересекает полилинию
                    direction = findBestDirection(circlePos, direction, i);
                }
            }
            else if (i == _circles.size() - 1) // Последняя точка
            {
                QPointF prevPos = _circles[i-1]->pos();
                QPointF segment = circlePos - prevPos;

                if (segment.manhattanLength() > 0)
                {
                    // Вычисляем нормаль (перпендикуляр к сегменту)
                    direction = QPointF(-segment.y(), segment.x());
                    direction = direction / QLineF(0, 0, direction.x(), direction.y()).length();

                    // Проверяем оба направления и выбираем то, которое не пересекает полилинию
                    direction = findBestDirection(circlePos, direction, i);
                }
            }
            else // Промежуточные точки
            {
                QPointF prevPos = _circles[i-1]->pos();
                QPointF nextPos = _circles[i+1]->pos();

                QPointF inSegment = circlePos - prevPos;
                QPointF outSegment = nextPos - circlePos;

                if (inSegment.manhattanLength() > 0 && outSegment.manhattanLength() > 0)
                {
                    // Нормализуем векторы
                    inSegment = inSegment / QLineF(0, 0, inSegment.x(), inSegment.y()).length();
                    outSegment = outSegment / QLineF(0, 0, outSegment.x(), outSegment.y()).length();

                    // Вычисляем биссектрису угла
                    QPointF bisector = inSegment + outSegment;

                    if (bisector.manhattanLength() > 0)
                    {
                        // Перпендикуляр к биссектрисе
                        direction = QPointF(-bisector.y(), bisector.x());
                        direction = direction / QLineF(0, 0, direction.x(), direction.y()).length();

                        // Проверяем оба направления и выбираем то, которое не пересекает полилинию
                        direction = findBestDirection(circlePos, direction, i);
                    }
                }
            }
        }

        const qreal handleRadius = _circles.isEmpty() ? 5.0 : _circles[0]->rect().width() / 2.0;
        const qreal fontHeight = _numberFontSize > 0 ? _numberFontSize : 10.0;
        const qreal offsetDistance = handleRadius + fontHeight * 0.8;
        QPointF numberPos = circlePos + direction * offsetDistance;

        // Создаем номер
        QGraphicsSimpleTextItem* number = new QGraphicsSimpleTextItem(QString::number(i), this);
        number->setBrush(_numberColor);
        number->setZValue(1001);
        number->setPen(Qt::NoPen);
        number->setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);

        // Применяем стиль шрифта
        if (_numberFontSize > 0)
        {
            QFont font = number->font();
            font.setPointSizeF(_numberFontSize);
            number->setFont(font);
        }

        // Центрируем номер
        QRectF textRect = number->boundingRect();
        QPointF finalNumberPos(numberPos.x() - textRect.width()/2,
                               numberPos.y() - textRect.height()/2);
        number->setPos(finalNumberPos);

        // Создаем фон
        QGraphicsRectItem* bg = new QGraphicsRectItem(textRect, this);
        bg->setPos(finalNumberPos);
        bg->setBrush(_numberBgColor);
        bg->setPen(Qt::NoPen);
        bg->setZValue(1000);
        bg->setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);

        // Запрещаем взаимодействие
        number->setFlag(QGraphicsItem::ItemIsSelectable, false);
        number->setFlag(QGraphicsItem::ItemIsMovable, false);
        number->setFlag(QGraphicsItem::ItemIsFocusable, false);
        bg->setFlag(QGraphicsItem::ItemIsSelectable, false);
        bg->setFlag(QGraphicsItem::ItemIsMovable, false);
        bg->setFlag(QGraphicsItem::ItemIsFocusable, false);

        pointNumbers.append(number);
        numberBackgrounds.append(bg);
    }
}

QPointF Polyline::findBestDirection(const QPointF& pointPos, const QPointF& initialDirection, int pointIndex)
{
    // Проверяем оба возможных направления
    QPointF direction1 = initialDirection;
    QPointF direction2 = -initialDirection;

    // Создаем тестовые сегменты для проверки пересечений
    const qreal testDistance = 50.0; // Достаточно большое расстояние для проверки
    QPointF testEnd1 = pointPos + direction1 * testDistance;
    QPointF testEnd2 = pointPos + direction2 * testDistance;

    QLineF testLine1(pointPos, testEnd1);
    QLineF testLine2(pointPos, testEnd2);

    // Проверяем количество пересечений с полилинией для каждого направления
    int intersections1 = countIntersections(testLine1, pointIndex);
    int intersections2 = countIntersections(testLine2, pointIndex);

    // Выбираем направление с меньшим количеством пересечений
    // (меньше пересечений = больше вероятность, что направление наружу)
    if (intersections1 < intersections2)
    {
        return direction1;
    }
    else
    {
        return direction2;
    }
}

int Polyline::countIntersections(const QLineF& testLine, int excludePointIndex)
{
    int intersectionCount = 0;

    // Проверяем пересечения со всеми сегментами полилинии
    for (int i = 0; i < _circles.size() - 1; ++i)
    {
        QPointF p1 = _circles[i]->pos();
        QPointF p2 = _circles[i + 1]->pos();
        QLineF segment(p1, p2);

        // Пропускаем сегменты, смежные с проверяемой точкой
        if (i == excludePointIndex - 1 || i == excludePointIndex)
        {
            continue;
        }

        // Проверяем пересечение
        QPointF intersectionPoint;
        if (segment.intersect(testLine, &intersectionPoint) == QLineF::BoundedIntersection)
        {
            intersectionCount++;
        }
    }

    // Проверяем последний сегмент для замкнутой полилинии
    if (_isClosed && _circles.size() > 2)
    {
        QPointF p1 = _circles.last()->pos();
        QPointF p2 = _circles.first()->pos();
        QLineF segment(p1, p2);

        // Пропускаем сегменты, смежные с проверяемой точкой
        bool isAdjacent = (excludePointIndex == 0 && _circles.size() - 1 == excludePointIndex - 1) ||
                          (excludePointIndex == _circles.size() - 1 && 0 == excludePointIndex + 1);

        if (!isAdjacent)
        {
            QPointF intersectionPoint;
            if (segment.intersect(testLine, &intersectionPoint) == QLineF::BoundedIntersection)
            {
                intersectionCount++;
            }
        }
    }

    return intersectionCount;
}

void Polyline::applyNumberStyle(qreal fontSize, const QColor& textColor, const QColor& bgColor)
{
    _numberFontSize = fontSize; // Сохраняем размер шрифта
    _numberColor = textColor;   // Сохраняем цвет текста
    _numberBgColor = bgColor;   // Сохраняем цвет фона

    // Сохраняем текущие позиции кругов
    QVector<QPointF> circlePositions;
    for (DragCircle* circle : _circles)
    {
        circlePositions.append(circle->pos());
    }

    // Полностью пересоздаем номера с новыми цветами
    updatePointNumbers();
}

void Polyline::rotatePointsClockwise()
{
    if (_circles.size() < 2) return;

    // Сдвигаем точки по часовой стрелке
    DragCircle* first = _circles.takeFirst();
    _circles.append(first);

    updatePath();
    updatePointNumbers();
}

void Polyline::rotatePointsCounterClockwise()
{
    if (_circles.size() < 2) return;

    // Сдвигаем точки против часовой стрелки
    DragCircle* last = _circles.takeLast();
    _circles.prepend(last);

    updatePath();
    updatePointNumbers();
}

void Polyline::handleKeyPressEvent(QKeyEvent* event)
{
    keyPressEvent(event);
}

void Polyline::togglePointNumbers()
{
    _pointNumbersVisible = !_pointNumbersVisible;
    updatePointNumbers();
}

} // namespace qgraph
