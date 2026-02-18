#pragma once

#include "user_type.h"
#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsRectItem>
#include <QObject>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>
#include <QAction>


namespace qgraph {

class DragCircle;

class Rectangle : public ShapeT<QGraphicsRectItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Rectangle)};
    int type() const override {return Type;}

    Rectangle(QGraphicsScene*);
    ~Rectangle();

    void setFrameScale(float) override;
    void dragCircleMove(DragCircle*) override;
    void dragCircleRelease(DragCircle*) override;

    // Возвращает координаты расположения прямоугольника на сцене с учетом
    // масштабного коэффициента frameScale()
    QRectF realSceneRect() const;
    void setRealSceneRect(const QRectF&);
    void updateHandlePosition();
    void updatePointNumbers();
    void applyNumberStyle(qreal fontSize, const QColor& textColor, const QColor& bgColor);
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void rotatePointsClockwise();
    void rotatePointsCounterClockwise();
    void handleKeyPressEvent(QKeyEvent* event);

    void togglePointNumbers();
    bool isPointNumbersVisible() const { return _pointNumbersVisible; }

    void updateHandlesZValue();
    void raiseHandlesToTop() override;
    void moveToBack();
    void recalcNumberingFromHandle(DragCircle* handle);

public slots:
    void deleteItem();

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void updateHandleVisibility();
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

private slots:
    void handleHandleHoverEnter();
    void handleHandleHoverLeave();

private:
    DragCircle* _circleTL; // Левый верхний угол
    DragCircle* _circleTR; // Правый верхний
    DragCircle* _circleBR; // Правый нижний
    DragCircle* _circleBL; // Левый нижний

    // Добавлено для предотвращения полного схлопывания прямоугольника
    const qreal _minSize = 10.0;

    bool _isDrawing = false;
    QPointF _startPoint;

    QColor _highlightColor = Qt::transparent; // Цвет выделения
    QList<QGraphicsSimpleTextItem*> pointNumbers;
    QList<QGraphicsRectItem*> numberBackgrounds;
    int _numberingOffset = 0; // Смещение для нумерации точек
    qreal _numberFontSize = 10.0; // Размер шрифта по умолчанию
    QColor _numberColor = Qt::white;
    QColor _numberBgColor = QColor(0, 0, 0, 180);

    bool _pointNumbersVisible = true; // Видимости нумерации
};

} // namespace qgraph
