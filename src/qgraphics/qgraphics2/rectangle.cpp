#include "rectangle.h"
#include "drag_circle.h"
#include <QtGui>
#include <QMenu>

namespace qgraph {

Rectangle::Rectangle(QGraphicsScene* scene)
    :_isDrawing(false), _pointNumbersVisible(true) // Изначально режим рисования выключен
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true); // Включаем обработку событий наведения
    setFlag(QGraphicsItem::ItemIsSelectable, true);

    setRect({0, 0, 50, 100});

    QPen pen = this->pen();
    pen.setColor(QColor(244, 164, 96));
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);

    setZLevel(1);
    scene->addItem(this);

    _circleTL = new DragCircle(scene);
    _circleTL->setParentItem(this);
    _circleTL->setVisible(true);
    _circleTL->setZValue(100000);

    _circleTR = new DragCircle(scene);
    _circleTR->setParentItem(this);
    _circleTR->setVisible(true);
    _circleTR->setZValue(100000);

    _circleBR = new DragCircle(scene);
    _circleBR->setParentItem(this);
    _circleBR->setVisible(true);
    _circleBR->setZValue(100000);

    _circleBL = new DragCircle(scene);
    _circleBL->setParentItem(this);
    _circleBL->setVisible(true);
    _circleBL->setZValue(100000);

    _circleTL->show();
    _circleTR->show();
    _circleBR->show();
    _circleBL->show();
}

Rectangle::~Rectangle()
{
    // Удаляем ручки из сцены при удалении прямоугольника
    if (scene()) {
        scene()->removeItem(_circleTL);
        scene()->removeItem(_circleTR);
        scene()->removeItem(_circleBR);
        scene()->removeItem(_circleBL);
    }

    delete _circleTL;
    delete _circleTR;
    delete _circleBR;
    delete _circleBL;


    qDeleteAll(pointNumbers);    // Удаляем номера
    qDeleteAll(numberBackgrounds); // Удаляем фоны
    pointNumbers.clear();
    numberBackgrounds.clear();
}

void Rectangle::setFrameScale(float newScale)
{
    float s = newScale / frameScale();
    QRectF r;
    r.setLeft(0);
    r.setTop (0);
    r.setWidth (rect().width()  * s);
    r.setHeight(rect().height() * s);

    prepareGeometryChange();
    setRect(r);
    setPos(pos().x() * s, pos().y() * s);

    // Обновляем позиции ручек в зависимости от новых координат прямоугольника
    _circleTL->setPos(r.topLeft());
    _circleTR->setPos(r.topRight());
    _circleBR->setPos(r.bottomRight());
    _circleBL->setPos(r.bottomLeft());

    _frameScale = newScale;
    //changeSignal.emit_(this);
    updatePointNumbers();
    raiseHandlesToTop();
}

void Rectangle::dragCircleMove(DragCircle* circle)
{
    qgraph::Shape::HandleBlocker guard(this);
    QRectF r = rect();

    if (_circleTL == circle)
    {
        qreal newX = circle->x();
        qreal newY = circle->y();

        // Ограничение: левая сторона не может пересекаться с правой
        if (newX >= r.right() - _minSize)
        {
            newX = r.right() - _minSize;
        }

        // Ограничение: верхняя сторона не может пересекаться с нижней
        if (newY >= r.bottom() - _minSize)
        {
            newY = r.bottom() - _minSize;
        }

        // Устанавливаем новый верхний левый угол прямоугольника
        r.setTopLeft(QPointF(newX, newY));

        // Обновление позиций кругов, чтобы они были прикреплены к углам
        _circleTL->setPos(r.topLeft());
        _circleTR->setPos(r.topRight());
        _circleBL->setPos(r.bottomLeft());
        _circleBR->setPos(r.bottomRight());
    }
    else if (_circleTR == circle)
    {
        qreal newX = circle->x();
        qreal newY = circle->y();

        // Ограничение: правая сторона не может пересекаться с левой
        if (newX <= r.left() + _minSize)
        {
            newX = r.left() + _minSize;
        }

        // Ограничение: верхняя сторона не может пересекаться с нижней
        if (newY >= r.bottom() - _minSize)
        {
            newY = r.bottom() - _minSize;
        }

        r.setTopRight(QPointF(newX, newY));

        _circleTR->setPos(r.topRight());

        _circleTL->setPos(r.topLeft());
        _circleBR->setPos(r.bottomRight());
        _circleBL->setPos(r.bottomLeft());
    }
    else if (_circleBR == circle)
    {
        qreal newX = circle->x();
        qreal newY = circle->y();

        // Ограничение: правая сторона не может пересекаться с левой
        if (newX <= r.left() + _minSize)
        {
            newX = r.left() + _minSize;
        }

        // Ограничение: нижняя сторона не может пересекаться с верхней
        if (newY <= r.top() + _minSize)
        {
            newY = r.top() + _minSize;
        }

        r.setBottomRight(QPointF(newX, newY));

        _circleBR->setPos(r.bottomRight());

        _circleTR->setPos(r.topRight());
        _circleBL->setPos(r.bottomLeft());
        _circleTL->setPos(r.topLeft());
    }

    else if (_circleBL == circle)
    {
        qreal newX = circle->x();
        qreal newY = circle->y();

        // Ограничение: левая сторона не может пересекаться с правой
        if (newX >= r.right() - _minSize)
        {
            newX = r.right() - _minSize;
        }

        // Ограничение: нижняя сторона не может пересекаться с верхней
        if (newY <= r.top() + _minSize)
        {
            newY = r.top() + _minSize;
        }

        r.setBottomLeft(QPointF(newX, newY));

        _circleBL->setPos(r.bottomLeft());

        _circleTL->setPos(r.topLeft());
        _circleTR->setPos(r.topRight());
        _circleBR->setPos(r.bottomRight());
    }

    prepareGeometryChange();
    setRect(r);
    updatePointNumbers();
    circleMoveSignal.emit_(this);
    updateHandlePosition();
    raiseHandlesToTop();
}

void Rectangle::dragCircleRelease(DragCircle* circle)
{
    if (circle->parentItem() == this)
    {
        circleReleaseSignal.emit_(this);
    }
}

QRectF Rectangle::realSceneRect() const
{
    float s = frameScale();
    QRectF r;
    r.setLeft(pos().x() / s);
    r.setTop (pos().y() / s);
    r.setWidth (rect().width()  / s);
    r.setHeight(rect().height() / s);
    return r;
}

void Rectangle::setRealSceneRect(const QRectF& r)
{
    QPointF pos = r.topLeft(); // Верхний левый угол
    QRectF rect(0, 0, r.width(), r.height()); // Размеры прямоугольника без учета масштаба

    prepareGeometryChange();
    setRect(rect);   // Обновляем размеры
    setPos(pos);     // Устанавливаем позицию

    _circleTL->setVisible(true);
    _circleTR->setVisible(true);
    _circleBL->setVisible(true);
    _circleBR->setVisible(true);


    updateHandlePosition(); // Обновляем позиции ручек
    updatePointNumbers();
    raiseHandlesToTop();
}

void Rectangle::updateHandlePosition()
{
    // Пересчитываем позиции ручек на основе текущих углов прямоугольника
    _circleTL->setPos(this->rect().topLeft());
    _circleTR->setPos(this->rect().topRight());
    _circleBL->setPos(this->rect().bottomLeft());
    _circleBR->setPos(this->rect().bottomRight());

    raiseHandlesToTop();
    updatePointNumbers();
}

void Rectangle::keyPressEvent(QKeyEvent* event)
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
        QGraphicsRectItem::keyPressEvent(event);
    }
}

void Rectangle::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
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

void Rectangle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsRectItem::hoverEnterEvent(event);
    QColor baseColor = pen().color();
    QColor highlightColor = baseColor;
    highlightColor.setAlpha(100);
    setBrush(highlightColor);

    raiseHandlesToTop();
    update();
}

void Rectangle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsRectItem::hoverLeaveEvent(event);
    setBrush(Qt::transparent);

    updateHandlesZValue();
    update();
}

void Rectangle::updateHandleVisibility()
{
    _circleTL->setVisible(true);
    _circleTR->setVisible(true);
    _circleBL->setVisible(true);
    _circleBR->setVisible(true);

    raiseHandlesToTop();
}

void Rectangle::paint(QPainter* painter,
                     const QStyleOptionGraphicsItem* option,
                     QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    const QRectF r = rect();

    if (isSelected() || isUnderMouse())
    {
        QColor fill = pen().color();
        fill.setAlpha(80);
        painter->save();
        painter->setBrush(fill);
        painter->setPen(Qt::NoPen);
        painter->drawRect(r);
        painter->restore();
    }

    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(pen());
    painter->drawRect(r);
    painter->restore();
}
void Rectangle::handleHandleHoverEnter()
{
    // Поднимаем прямоугольник на верхний z-уровень временно
    setZValue(1001);
}

void Rectangle::handleHandleHoverLeave()
{
    // Возвращаем исходный z-уровень
    setZValue(1);
    // Возвращаем видимость ручек в исходное состояние
    //updateHandleVisibility();
}

void Rectangle::updatePointNumbers()
{
    if (!scene())
        return;

    qDeleteAll(pointNumbers);
    qDeleteAll(numberBackgrounds);
    pointNumbers.clear();
    numberBackgrounds.clear();

    if (!_pointNumbersVisible)
        return;

    // Массив узлов
    DragCircle* circles[4] = { _circleTL, _circleTR, _circleBR, _circleBL };

    auto centerOf = [](DragCircle* c) -> QPointF {
        return c ? c->pos() : QPointF(0, 0);
    };

    QPointF pts[4] =
    {
        centerOf(circles[0]),
        centerOf(circles[1]),
        centerOf(circles[2]),
        centerOf(circles[3])
    };

    const qreal baseFontSize = (_numberFontSize > 0 ? _numberFontSize : 10.0);
    const qreal margin       = baseFontSize * 0.25;

    for (int i = 0; i < 4; ++i)
    {
        if (!circles[i])
            continue;

        int idx = (i + _numberingOffset) % 4;

        auto* num = new QGraphicsSimpleTextItem(QString::number(idx), this);
        num->setBrush(_numberColor);
        num->setPen(Qt::NoPen);
        num->setZValue(1001);
        num->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        if (_numberFontSize > 0) {
            QFont f = num->font();
            f.setPointSizeF(_numberFontSize);
            num->setFont(f);
        }

        QRectF r = num->boundingRect();
        qreal halfW = r.width()  / 2.0;
        qreal halfH = r.height() / 2.0;

        QPointF center = pts[i];

        // Радиус свой для каждого узла
        qreal handleR = circles[i]->boundingRect().width() / 2.0;

        switch (i)
        {
        case 0:
            center.setX(center.x() - (handleR + halfW + margin));
            break;
        case 1:
            center.setX(center.x() + (handleR + halfW + margin));
            break;
        case 2:
            center.setX(center.x() + (handleR + halfW + margin));
            break;
        case 3:
            center.setX(center.x() - (handleR + halfW + margin));
            break;
        }

        num->setPos(center.x() - halfW,
                    center.y() - halfH);

        auto* bg = new QGraphicsRectItem(num->boundingRect(), this);
        bg->setPos(num->pos());
        bg->setBrush(_numberBgColor);
        bg->setPen(Qt::NoPen);
        bg->setZValue(1000);
        bg->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        pointNumbers.append(num);
        numberBackgrounds.append(bg);
    }
}

void Rectangle::applyNumberStyle(qreal fontSize, const QColor& textColor, const QColor& bgColor)
{
    _numberFontSize = fontSize; // Сохраняем размер шрифта
    _numberColor = textColor;   // Сохраняем цвет текста
    _numberBgColor = bgColor;   // Сохраняем цвет фона

    updatePointNumbers();
}

QVariant Rectangle::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == QGraphicsItem::ItemPositionHasChanged)
    {
        // При любом перемещении прямоугольника подтягиваем номера
        updatePointNumbers();
    }
    else if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        update();
    }
    return QGraphicsRectItem::itemChange(change, value);
}

void Rectangle::rotatePointsClockwise()
{
    _numberingOffset = (_numberingOffset - 1 + 4) % 4;
    updatePointNumbers();
}

void Rectangle::rotatePointsCounterClockwise()
{
    _numberingOffset = (_numberingOffset + 1) % 4;
    updatePointNumbers();
}

void Rectangle::deleteItem()
{
    auto scene = this->scene();
    if (scene)
    {
        scene->removeItem(this);
        delete this;
    }
}

void Rectangle::handleKeyPressEvent(QKeyEvent* event)
{
    keyPressEvent(event);
}

void Rectangle::togglePointNumbers()
{
    _pointNumbersVisible = !_pointNumbersVisible;
    updatePointNumbers();
}

void Rectangle::updateHandlesZValue()
{
    _circleTL->setZValue(this->zValue() + 1000);
    _circleTR->setZValue(this->zValue() + 1000);
    _circleBR->setZValue(this->zValue() + 1000);
    _circleBL->setZValue(this->zValue() + 1000);

    raiseHandlesToTop();
}

void Rectangle::raiseHandlesToTop()
{
    // Временное повышение z-value ручек
    _circleTL->setZValue(this->zValue() + 1000);
    _circleTR->setZValue(this->zValue() + 1000);
    _circleBR->setZValue(this->zValue() + 1000);
    _circleBL->setZValue(this->zValue() + 1000);

    // Принудительное обновление сцены
    if (scene())
    {
        scene()->update();
    }
}

void Rectangle::moveToBack()
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

} // namespace qgraph
