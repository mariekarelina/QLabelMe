#include "rectangle.h"
#include "drag_circle.h"
#include <QtGui>
#include <QMenu>
#include <QMetaObject>
#include <QGraphicsView>
#include <QVariant>

namespace qgraph {

Rectangle::Rectangle(QGraphicsScene* scene)
    :_isDrawing(false) // Изначально режим рисования выключен
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
    setBrush(Qt::NoBrush);

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


    qDeleteAll(pointNumbers); // Удаляем номера
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
    setRect(rect); // Обновляем размеры
    setPos(pos); // Устанавливаем позицию

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
            if (QObject* receiver = scene->property("shapeDeleteReceiver").value<QObject*>())
            {
                QMetaObject::invokeMethod(receiver,
                                          "onSceneItemRemoved",
                                          Qt::DirectConnection,
                                          Q_ARG(QGraphicsItem*, static_cast<QGraphicsItem*>(this)));
            }
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
    QAction* changeClassAction = menu.addAction("Изменить класс");

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
        auto sc = this->scene();
        if (sc)
        {
            if (QObject* receiver = sc->property("shapeDeleteReceiver").value<QObject*>())
            {
                QMetaObject::invokeMethod(receiver,
                                          "onSceneItemRemoved",
                                          Qt::DirectConnection,
                                          Q_ARG(QGraphicsItem*, static_cast<QGraphicsItem*>(this)));
            }

            sc->removeItem(this);
            delete this;
        }
    });

    QObject::connect(changeClassAction, &QAction::triggered, [this]() {
        auto sc = this->scene();
        if (!sc)
            return;

        QObject* receiver = sc->property("classChangeReceiver").value<QObject*>();
        if (!receiver)
            return;

        const qulonglong uid = this->data(0x1337ABCD).toULongLong();
        if (uid == 0)
            return;

        QMetaObject::invokeMethod(receiver,
                                  "changeClassByUid",
                                  Qt::DirectConnection,
                                  Q_ARG(qulonglong, uid));
    });

    menu.exec(event->screenPos());
    event->accept();
}

void Rectangle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsRectItem::hoverEnterEvent(event);

    _hovered = true;

    bool fillEnabled = true;
    if (scene())
    {
        const QVariant v = scene()->property("fillShapeWhenSelected");
        if (v.isValid())
            fillEnabled = v.toBool();
    }
    if (fillEnabled)
    {
        QColor fill = pen().color();
        fill.setAlpha(60);
        setBrush(QBrush(fill));
    }
    else
    {
        setBrush(Qt::NoBrush);
    }
    raiseHandlesToTop();
    update();
}

void Rectangle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsRectItem::hoverLeaveEvent(event);

    _hovered = false;

    bool fillEnabled = true;
    if (scene())
    {
        const QVariant v = scene()->property("fillShapeWhenSelected");
        if (v.isValid())
            fillEnabled = v.toBool();
    }

    if (isSelected() && fillEnabled)
    {
        QColor fill = pen().color();
        fill.setAlpha(60);
        setBrush(QBrush(fill));
    }
    else
    {
        setBrush(Qt::NoBrush);
    }
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

    painter->save();
    painter->setBrush(brush());
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

    if (!(_globalPointNumbersVisible || _pointNumbersVisible))
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
    const qreal margin = baseFontSize * 0.25;
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

        if (_numberFontSize > 0)
        {
            QFont f = num->font();
            f.setPointSizeF(_numberFontSize);
            num->setFont(f);
        }

        const QRectF textRect = num->boundingRect();
        QPointF finalNumberPos = pts[i] + QPointF(8.0, -8.0);

        if (scene() && !scene()->views().isEmpty())
        {
            QGraphicsView* view = scene()->views().first();
            if (view)
            {
                const QPointF handleCenterScene = circles[i]->scenePos();
                const QPointF handleCenterViewF = view->mapFromScene(handleCenterScene);

                qreal handleRadiusPx = qMax<qreal>(circles[i]->boundingRect().width() / 2.0, 6.0);

                QPointF direction;
                switch (i)
                {
                case 0: direction = QPointF(-1.0, 0.0); break;
                case 1: direction = QPointF( 1.0, 0.0); break;
                case 2: direction = QPointF( 1.0, 0.0); break;
                case 3: direction = QPointF(-1.0, 0.0); break;
                default: direction = QPointF(1.0, 0.0); break;
                }

                const qreal dirX = std::abs(direction.x());
                const qreal dirY = std::abs(direction.y());

                const qreal textExtentPx =
                        dirX * (textRect.width()  / 2.0) +
                        dirY * (textRect.height() / 2.0);

                const qreal gapPx = qMax<qreal>(margin, 3.0);
                const qreal offsetPx = handleRadiusPx + textExtentPx + gapPx;

                const QPointF numberCenterViewF =
                        handleCenterViewF + direction * offsetPx;

                const QPointF topLeftViewF(
                        numberCenterViewF.x() - textRect.width()  / 2.0,
                        numberCenterViewF.y() - textRect.height() / 2.0);

                const QPointF topLeftScene = view->mapToScene(
                        QPoint(int(std::round(topLeftViewF.x())),
                               int(std::round(topLeftViewF.y())))
                );

                finalNumberPos = mapFromScene(topLeftScene);
            }
        }

        num->setPos(finalNumberPos);

        auto* bg = new QGraphicsRectItem(textRect, this);
        bg->setPos(finalNumberPos);
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
        updatePointNumbers();
    }
    else if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        const bool selectedNow = value.toBool();

        bool fillEnabled = true;
        if (scene())
        {
            const QVariant v = scene()->property("fillShapeWhenSelected");
            if (v.isValid())
                fillEnabled = v.toBool();
        }

        if ((selectedNow || _hovered) && fillEnabled)
        {
            QColor fill = pen().color();
            fill.setAlpha(60);
            setBrush(QBrush(fill));
        }
        else
        {
            setBrush(Qt::NoBrush);
        }

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

void Rectangle::setGlobalPointNumbersVisible(bool visible)
{
    if (_globalPointNumbersVisible == visible)
        return;

    _globalPointNumbersVisible = visible;
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

void Rectangle::recalcNumberingFromHandle(DragCircle* handle)
{
    if (!handle)
        return;

    int i = -1;
    if (handle == _circleTL) i = 0;
    else if (handle == _circleTR) i = 1;
    else if (handle == _circleBR) i = 2;
    else if (handle == _circleBL) i = 3;

    if (i < 0)
        return;

    _numberingOffset = (4 - i) % 4;
    updatePointNumbers();
}

} // namespace qgraph
