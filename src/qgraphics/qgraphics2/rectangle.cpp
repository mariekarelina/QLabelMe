#include "rectangle.h"
#include "drag_circle.h"
#include <QtGui>

namespace qgraph {

Rectangle::Rectangle(QGraphicsScene* scene)
    : _isDrawing(false) // Изначально режим рисования выключен
{
    setFlags(ItemIsMovable | ItemIsFocusable);
    setAcceptHoverEvents(true); // Включаем обработку событий наведения

    setRect({0, 0, 50, 100});

    QPen pen = this->pen();
    pen.setColor(QColor(0, 255, 0));
    pen.setWidth(2);
    pen.setCosmetic(true);
    setPen(pen);

    setZLevel(1);
    scene->addItem(this);

    _circleTL = new DragCircle(scene);
    _circleTL->setParent(this);
    _circleTL->setVisible(false);

    _circleTR = new DragCircle(scene);
    _circleTR->setParent(this);
    _circleTR->setVisible(false);

    _circleBR = new DragCircle(scene);
    _circleBR->setParent(this);
    _circleBR->setVisible(false);

    _circleBL = new DragCircle(scene);
    _circleBL->setParent(this);
    _circleBL->setVisible(false);
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
}

void Rectangle::dragCircleMove(DragCircle* circle)
{
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
        //r.setTopLeft(QPoint(circle->x(), circle->y()));

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
        //r.setWidth(circle->x());
        //r.setHeight(circle->y());
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
        //r.setWidth(circle->x());
        //r.setHeight(circle->y());
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

    circleMoveSignal.emit_(this);
}

void Rectangle::dragCircleRelease(DragCircle* circle)
{
    if (circle->parentItem() == this)
    {
        //circle->setPos(rect().width(), rect().height());
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
    float s = frameScale();
    // Если масштаб равен нулю
    if (qFuzzyCompare(s, 0))
    {
        s = 1.0f; // Устанавливаем значение по умолчанию
    }

    QPointF pos = r.topLeft(); // Верхний левый угол
    QRectF rect(0, 0, r.width(), r.height()); // Размеры прямоугольника без учета масштаба

    prepareGeometryChange();
    setRect(rect);   // Обновляем размеры
    setPos(pos);     // Устанавливаем позицию

    // Убедитесь, что ручки видимы
    _circleTL->setVisible(true);
    _circleTR->setVisible(true);
    _circleBL->setVisible(true);
    _circleBR->setVisible(true);


    updateHandlePosition(); // Обновляем позиции ручек

    //_circle->setPos(rect.width(), rect.height());
}

void Rectangle::updateHandlePosition()
{
    // Пересчитываем позиции ручек на основе текущих углов прямоугольника
    _circleTL->setPos(this->rect().topLeft());
    _circleTR->setPos(this->rect().topRight());
    _circleBL->setPos(this->rect().bottomLeft());
    _circleBR->setPos(this->rect().bottomRight());
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
    else
    {
        // Передаем событие дальше, если это не `Del`
        QGraphicsRectItem::keyPressEvent(event);
    }
}

void Rectangle::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    // Устанавливаем бледный цвет заливки при наведении
    setBrush(QColor(200, 255, 200, 150)); // Бледно-красный с прозрачностью
    update(); // Обновляем отображение
}

void Rectangle::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    Q_UNUSED(event);
    // Возвращаем прозрачную заливку при уходе курсора
    setBrush(Qt::transparent);
    update(); // Обновляем отображение
}

} // namespace qgraph
