#include "line.h"
#include <QPainter>
#include <QPen>
#include <QKeyEvent>
#include <QRectF>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QCursor>
#include <QGraphicsView>


namespace qgraph {

qgraph::Line::CloseMode qgraph::Line::s_closeMode =
    qgraph::Line::CloseMode::CtrlModifier;

Line::Line(QGraphicsScene* scene, const QPointF& scenePos)
    : _isClosed(false)
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true); // Включаем обработку событий наведения
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    QPen pen = this->pen();
    pen.setColor(QColor(220, 221, 106));
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
    _circles.append(startCircle);

    updateConnections();
    updatePath(); // Обновляем начальный путь
}

Line::~Line()
{
    qDeleteAll(pointNumbers); // Удаляем номера
    qDeleteAll(numberBackgrounds); // Удаляем фоны
    pointNumbers.clear();
    numberBackgrounds.clear();
}

void Line::setFrameScale(float newScale)
{
    float s = newScale / _frameScale;

    for (DragCircle* circle : _circles)
    {
        circle->setPos(circle->pos() * s); // Масштабируем точки
    }

    _frameScale = newScale;
    updatePath(); // Обновляем путь
}

void Line::setRealSceneRect(const QRectF& r)
{
    float s = frameScale();
    // Корректируем масштаб
    _frameScale = s;
    // Обновляем путь
    updatePath();
}

void Line::updateHandlePosition()
{
//    for (auto circle : _circles)
//    {
//        circle->moveBy(delta.x(), delta.y());
//    }
}

void Line::addPoint(const QPointF& position, QGraphicsScene* scene)
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
    if (_modificationCallback)
    {
        _modificationCallback();
    }
}

void Line::removePoint(QPointF position)
{
    if (_circles.size() <= 2)
    {
        // Если точек меньше двух, не удаляем последние точки, чтобы сохранить линию
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
    {
        handlePointDeletion(closestCircle);
    }
}

void Line::insertPoint(QPointF position)
{
    if (_circles.size() < 2)
    {
        return; // Если точек меньше двух, вставка невозможна
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

    // Проверяем замкнутую линию (последний сегмент)
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
        if (_modificationCallback)
        {
            _modificationCallback();
        }
    }
    updatePointNumbers();
}


void Line::closeLine()
{
    setClosed(true, true);
}

bool Line::isClickOnAnyPoint(const QPointF& scenePos, int* idx) const
{
    for (int i = 0; i < _circles.size(); ++i) {
        const QPointF pi = _circles[i]->scenePos();
        if (QLineF(scenePos, pi).length() <= 8.0)
        {
            if (idx) *idx = i;
            return true;
        }
    }
    return false;
}

void Line::updatePath()
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

    setPath(path);
    updatePointNumbers();
    updateSelectionRect();
}

QVariant Line::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemPositionHasChanged)
    {
        updatePointNumbers();
    }
    else if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        update();              // Перерисовать заливку/контур
        updatePointNumbers();  // Чтобы номера/фон у номеров тоже реагировали
        updateSelectionRect();
    }
    return QGraphicsPathItem::itemChange(change, value);
}

bool Line::removeLastPointForce(bool callCallback)
{
    if (!scene())
        return false;

    if (_circles.size() <= 1)
        return false;

    DragCircle* circle = _circles.takeLast();
    if (!circle)
        return false;

    circle->invalidate();
    circle->disconnect();

    if (scene())
        scene()->removeItem(circle);

    delete circle;

    if (_isClosed && _circles.size() <= 2)
        _isClosed = false;

    updatePath();
    updatePointNumbers();

    if (callCallback && _modificationCallback)
        _modificationCallback();

    return true;
}

void Line::setClosed(bool closed, bool callCallback)
{
    if (closed)
    {
        if (_circles.size() <= 1) return;
        if (_isClosed) return;
        _isClosed = true;
    }
    else
    {
        if (!_isClosed) return;
        _isClosed = false;
    }

    updateConnections();
    updatePath();
    updatePointNumbers();

    if (callCallback && _modificationCallback)
        _modificationCallback();
}


void Line::handlePointDeletion(DragCircle* circle)
{
    if (!circle || !scene()) return;

    // Не позволяем удалять точки, если их меньше 2 в незамкнутой
    if (!_isClosed && _circles.size() <= 2)
        return;

    if (_circles.contains(circle))
    {
        // Помечаем как невалидный перед удалением
        circle->invalidate();
        // Отключаем все сигналы
        circle->disconnect();
        _circles.removeOne(circle);

        if (scene())
        {
            scene()->removeItem(circle);
        }
        delete circle;

        updatePath();
        updatePointNumbers();
        if (_modificationCallback)
        {
            _modificationCallback();
        }
    }
}

void Line::setNumberingFromLast(bool v)
{
    if (_numberingFromLast == v)
        return;

    _numberingFromLast = v;
    updatePointNumbers();
}

QVector<QPointF> Line::points() const
{
    QVector<QPointF> result;
    for (DragCircle* circle : _circles)
    {
        result.append(circle->scenePos());
    }
    return result;
}

void Line::keyPressEvent(QKeyEvent* event)
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
    else if (s_closeMode == CloseMode::KeyC &&
            (event->key() == Qt::Key_C))
    {
        closeLine();
        event->accept();
        return;
    }
    else
    {
        // Передаем событие дальше, если это не `Del`
        QGraphicsPathItem::keyPressEvent(event);
    }
}

void Line::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
    // Завершение по двойному клику на любой существующей точке
    if (s_closeMode == CloseMode::DoubleClick &&
        event->button() == Qt::LeftButton)
    {
        int idx = -1;
        if (isClickOnAnyPoint(event->scenePos(), &idx))
        {
            closeLine();
            event->accept();
            return;
        }
    }
    // QGraphicsPathItem::mouseDoubleClickEvent(event);
}

void Line::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    QColor baseColor = pen().color();
    QColor highlightColor = baseColor;
    highlightColor.setAlpha(100);

    setBrush(highlightColor);
    update();
}

void Line::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    setBrush(Qt::transparent);
    update();
}

void Line::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
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
    QString frameText = _selectionRectVisible ? "Скрыть рамку (W)" : "Показать рамку (W)";

    QAction* moveToBackAction = menu.addAction("Переместить на задний план (B)");
    QAction* toggleNumbersAction = menu.addAction(numbersText);
    QAction* toggleFrameAction = menu.addAction(frameText);

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


int Line::findClosestSegment(const QPointF& pos) const
{
    if (_circles.size() < 2) return -1;

    qreal minDistance = std::numeric_limits<qreal>::max();
    int closestSegment = -1;

    for (int i = 0; i < _circles.size() - 1; ++i) {
        QLineF segment(_circles[i]->pos(), _circles[i+1]->pos());

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

void Line::insertPointAtSegment(int segmentIndex, const QPointF& pos)
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

void Line::paint(QPainter* painter,
                     const QStyleOptionGraphicsItem* option,
                     QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QPainterPath path = this->path();

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

void Line::dragCircleMove(DragCircle* circle)
{
    Q_UNUSED(circle);
    updatePath(); // Обновляем путь при любом перемещении
    updatePointNumbers();
}

void Line::dragCircleRelease(DragCircle* circle)
{
    Q_UNUSED(circle);
    updatePath(); // Обновляем путь при любом перемещении
    updatePointNumbers(); // Обновляем позиции номеров
    if (_modificationCallback)
    {
        _modificationCallback();
    }
}

void Line::updateSelectionRect()
{
    if (!_selectionRect)
    {
        _selectionRect = new QGraphicsRectItem(this);
        _selectionRect->setBrush(Qt::NoBrush);

        QPen pen(Qt::DashLine);
        pen.setWidthF(0.0);
        pen.setColor(QColor(220, 220, 220));
        _selectionRect->setPen(pen);

        _selectionRect->setVisible(false);
    }

    _selectionRect->setRect(boundingRect());

    const bool visible = isSelected() && _selectionRectVisible;
    _selectionRect->setVisible(visible);
    if (visible)
        _selectionRect->setZValue(zValue() + 0.5);
}

void Line::toggleSelectionRect()
{
    _selectionRectVisible = !_selectionRectVisible;
    updateSelectionRect();
}

void Line::updateHandlesZValue()
{
    for (DragCircle* circle : _circles)
    {
        circle->setZValue(this->zValue() + 1);
    }
}

void Line::raiseHandlesToTop()
{
    for (DragCircle* circle : _circles)
    {
        circle->raiseToTop();
        circle->setVisible(true);
    }
}

void Line::moveToBack()
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

void Line::setModificationCallback(std::function<void()> callback)
{
    _modificationCallback = callback;
}

void Line::replaceScenePoints(const QVector<QPointF>& scenePts, bool closed)
{
    for (DragCircle* c : _circles)
    {
        if (!c) continue;
        c->setParentItem(nullptr);
        delete c;
    }
    _circles.clear();

    qDeleteAll(pointNumbers);
    qDeleteAll(numberBackgrounds);
    pointNumbers.clear();
    numberBackgrounds.clear();

    if (QGraphicsScene* sc = scene())
    {
        for (const QPointF& p : scenePts)
            addPoint(p, sc);
    }

    updatePath();
    updatePointNumbers();
    if (_modificationCallback)
        _modificationCallback();
}


void Line::updateConnections()
{

}

void Line::updateClosedState()
{

}

void Line::updatePointNumbersAfterReorder()
{
    // Удаляем старые номера и фоны
    qDeleteAll(pointNumbers);
    qDeleteAll(numberBackgrounds);
    pointNumbers.clear();
    numberBackgrounds.clear();

    // Создаем новые номера с обновленными индексами
    updatePointNumbers();
}

void Line::updatePointNumbers()
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

        qreal handleRadius = 5.0;
        if (!_circles.isEmpty() && _circles[i])
        {
            handleRadius = _circles[i]->rect().width() / 2.0;
        }

        const qreal fontHeight = (_numberFontSize > 0 ? _numberFontSize : 10.0);
        const qreal offsetDistance = handleRadius + fontHeight * 0.8;
        QPointF numberPos = circlePos + direction * offsetDistance;

        // Создаем номер
        //QGraphicsSimpleTextItem* number = new QGraphicsSimpleTextItem(QString::number(i), this);
        const int labelIndex = _numberingFromLast ? (_circles.size() - 1 - i) : i;
        QGraphicsSimpleTextItem* number = new QGraphicsSimpleTextItem(QString::number(labelIndex), this);
        number->setBrush(_numberColor);
        number->setZValue(1001);
        number->setPen(Qt::NoPen);
        number->setFlag(QGraphicsItem::ItemIgnoresParentOpacity, true);
        number->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        number->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

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
        bg->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        bg->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

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

QPointF Line::findBestDirection(const QPointF& pointPos, const QPointF& initialDirection, int pointIndex)
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

int Line::countIntersections(const QLineF& testLine, int excludePointIndex)
{
    int intersectionCount = 0;

    // Проверяем пересечения со всеми сегментами линии
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
        if (segment.intersects(testLine, &intersectionPoint) == QLineF::BoundedIntersection)
        {
            ++intersectionCount;
        }
    }

    // Проверяем последний сегмент для завершенной линии
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
            if (segment.intersects(testLine, &intersectionPoint) == QLineF::BoundedIntersection)
            {
                intersectionCount++;
            }
        }
    }

    return intersectionCount;
}

void Line::applyNumberStyle(qreal fontSize, const QColor& textColor, const QColor& bgColor)
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

void Line::rotatePointsClockwise()
{
    if (_circles.size() < 2) return;

    // Сдвигаем точки по часовой стрелке
    DragCircle* first = _circles.takeFirst();
    _circles.append(first);

    updatePath();
    updatePointNumbers();
}

void Line::rotatePointsCounterClockwise()
{
    if (_circles.size() < 2) return;

    // Сдвигаем точки против часовой стрелки
    DragCircle* last = _circles.takeLast();
    _circles.prepend(last);

    updatePath();
    updatePointNumbers();
}

void Line::handleKeyPressEvent(QKeyEvent* event)
{
    keyPressEvent(event);
}

void Line::resumeFromHandle(DragCircle* h)
{
    if (!h) return;

    const int idx = _circles.indexOf(h);
    if (idx < 0) return;

    if (_isClosed)
    {
        QVector<DragCircle*> rotated;
        rotated.reserve(_circles.size());

        for (int i = idx + 1; i < _circles.size(); ++i)
            rotated.append(_circles[i]);
        // Выбранная точка - последняя
        for (int i = 0; i <= idx; ++i)
            rotated.append(_circles[i]);

        _circles = rotated;
    }
    else
    {
        for (int i = _circles.size() - 1; i > idx; --i)
        {
            DragCircle* c = _circles[i];
            c->invalidate();
            c->disconnect();
            _circles.removeAt(i);

            if (scene())
                scene()->removeItem(c);

            delete c;
        }
    }

    _isClosed = false;

    updateConnections();
    updatePath();
    updatePointNumbers();
    updateSelectionRect();
}

void Line::togglePointNumbers()
{
    _pointNumbersVisible = !_pointNumbersVisible;
    updatePointNumbers();
}

} // namespace qgraph
