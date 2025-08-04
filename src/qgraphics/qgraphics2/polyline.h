#pragma once

#include "user_type.h"
#include "shape.h"

#include "drag_circle.h"
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QObject>
#include <QVector>
#include <QMenu>
#include <QLineF>
#include <QtMath>


namespace qgraph {

// class Polyline : public QObject, public ShapeT<QGraphicsPathItem> //public QGraphicsPathItem
// {
//     Q_OBJECT
class Polyline : public ShapeT<QGraphicsPathItem> //public QGraphicsPathItem
{
public:
    enum {Type = toInt(qgraph::UserType::Rectangle)};
    int type() const override {return Type;}

    Polyline(QGraphicsScene*, const QPointF&);
    ~Polyline();

    void setFrameScale(float newScale) override;
    void setRealSceneRect(const QRectF&);
    void updateHandlePosition();

    void addPoint(const QPointF&, QGraphicsScene*);
    void removePoint(QPointF position);
    void insertPoint(QPointF position);
    void closePolyline();
    void updatePath(); // Метод для обновления пути на основе позиций кругов
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void handlePointDeletion(DragCircle* circle);

    QVector<QPointF> points() const
    {
        QVector<QPointF> result;
        for (DragCircle* circle : _circles)
        {
            result.append(circle->scenePos());
        }
        return result;
    }
    void updatePointNumbers();
    void applyNumberStyle(qreal fontSize);
    void rotatePointsClockwise();
    void rotatePointsCounterClockwise();
    void handleKeyPressEvent(QKeyEvent* event);

    void togglePointNumbers();
    bool isPointNumbersVisible() const { return _pointNumbersVisible; }

    void updateHandlesZValue();
    void raiseHandlesToTop() override;
    void moveToBack();


protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    int findClosestSegment(const QPointF& pos) const;
    void insertPointAtSegment(int segmentIndex, const QPointF& pos);

signals:
    void lineChanged(Polyline* line);

//private slots:
    // void dragCircleMove(DragCircle* circle) override;
    // void dragCircleRelease(DragCircle* circle) override;

    // void handleHandleHoverEnter();
    // void handleHandleHoverLeave();

private:
    void updateConnections(); // Для обновления связей между точками
    void updateClosedState();
    void updatePointNumbersAfterReorder(); // Обновление номеров после изменения порядка

    void dragCircleMove(DragCircle* circle) override;
    void dragCircleRelease(DragCircle* circle) override;

public:
    QVector<DragCircle*> _circles;
    bool _isClosed;

    float _frameScale = 1.0;
    const qreal _minSegmentLength = 10.0; // Минимальная длина отрезка

    QColor _highlightColor = Qt::transparent; // Цвет выделения
    QList<QGraphicsSimpleTextItem*> pointNumbers;
    QList<QGraphicsRectItem*> numberBackgrounds;
    qreal _numberFontSize = 10.0; // Размер шрифта по умолчанию

    bool _pointNumbersVisible = true; // Видимости нумерации
};

} // namespace qgraph

