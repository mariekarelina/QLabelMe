#pragma once

#include "shared/simple_signal.h"
#include "drag_circle.h"

namespace qgraph {

template<typename> class ShapeT;

/**
  Shape и ShapeT - сервисные классы, они позволяют добавить некоторые свойства
  в классы для отрисовки геометрических фигур VirtualLine и RoiRect
*/
class Shape
{
public:
    typedef Closure<void (QGraphicsItem*)> ItemSignal;

    // Признак активирования/выделения геометрической фигуры
    bool active() const {return _active;}
    virtual void setActive(bool) = 0;

    // Скрывает или отображает окружности используемые для перемещения
    // геометрической фигуры
    virtual void circleVisible(bool) = 0;

    // Скрывает или отображает геометрическую фигуру
    virtual void shapeVisible(bool) = 0;

    // Коэффициент масштабирования геометрической фигуры
    float frameScale() const {return _frameScale;}
    virtual void setFrameScale(float) = 0;

    // Вызывается когда происходит перемещение "окружности перетаскивания"
    virtual void dragCircleMove(DragCircle*) = 0;

    // Вызывается когда "окружностm перетаскивания" отпускается
    virtual void dragCircleRelease(DragCircle*) = 0;

    // Устанавливает zValue уровень для фигуры
    virtual void setZLevel(float z) = 0;

    virtual QColor penColor() const = 0;
    virtual void setPenColor(const QColor&) = 0;

    virtual int penWidth() const = 0;
    virtual void setPenWidth(int) = 0;

    int tag() const {return _tag;}
    void setTag(int val) {_tag = val;}

    SimpleSignal<ItemSignal> circleMoveSignal;
    SimpleSignal<ItemSignal> circleReleaseSignal;
    SimpleSignal<ItemSignal> shapeMoveSignal;

protected:
    bool _active = {false};
    float _frameScale = {1};
    int _tag = {0};

    template<typename> friend class ShapeT;
};

template<typename T>
struct ShapeT : public Shape, public T
{
    ~ShapeT() {}

    void setActive(bool) override;
    void circleVisible(bool) override;
    void shapeVisible(bool)  override;
    void setZLevel(float) override;

    QColor penColor() const override;
    void setPenColor(const QColor&) override;

    int penWidth() const override;
    void setPenWidth(int) override;

    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;
};

//---------------------------------- ShapeT ----------------------------------

template<typename T>
void ShapeT<T>::setActive(bool val)
{
    for (QGraphicsItem* item : this->scene()->items())
    {
        if (item == this)
        {
            //setZVal((val) ? zValueActive : zValuePassive);
            circleVisible(val);
            continue;
        }
        if (Shape* shape = dynamic_cast<Shape*>(item))
            if (shape->_active)
            {
                //setZVal(this->zValuePassive);
                shape->circleVisible(false);
                shape->_active = false;
            }
    }
    _active = val;
}

template<typename T>
void ShapeT<T>::circleVisible(bool val)
{
    for (QGraphicsItem* item : this->childItems())
        if (DragCircle* circle = qgraphicsitem_cast<DragCircle*>(item))
        {
            circle->setVisible(val);
            //circle->setZValue(this->zValue() + 1);
        }
}

template<typename T>
void ShapeT<T>::shapeVisible(bool val)
{
    if (val == false)
        circleVisible(false);

    this->setVisible(val);
}

template<typename T>
void ShapeT<T>::setZLevel(float z)
{
    this->setZValue(z);
    for (QGraphicsItem* item : this->childItems())
        if (DragCircle* circle = qgraphicsitem_cast<DragCircle*>(item))
            circle->setZValue(z + 1);
}

template<typename T>
QColor ShapeT<T>::penColor() const
{
    return this->pen().color();
}

template<typename T>
void ShapeT<T>::setPenColor(const QColor& val)
{
    QPen pen = this->pen();
    pen.setColor(val);
    this->setPen(pen);
}

template<typename T>
int ShapeT<T>::penWidth() const
{
    return this->pen().width();
}

template<typename T>
void ShapeT<T>::setPenWidth(int val)
{
    QPen pen = this->pen();
    pen.setWidth(val);
    this->setPen(pen);
}

template<typename T>
void ShapeT<T>::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    setActive(true);
    QGraphicsItem::mousePressEvent(event);
    this->update();
}

template<typename T>
void ShapeT<T>::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseMoveEvent(event);
    shapeMoveSignal.emit_(this);
}

template<typename T>
void ShapeT<T>::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsItem::mouseReleaseEvent(event);
}

} // namespace qgraph
