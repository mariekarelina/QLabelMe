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

class Polyline : public QObject, public ShapeT<QGraphicsPathItem> //public QGraphicsPathItem
{
    Q_OBJECT
public:
    Polyline(QGraphicsScene*, const QPointF&);

    void setFrameScale(float newScale);
    void setRealSceneRect(const QRectF&);
    void updateHandlePosition();

    void addPoint(const QPointF&, QGraphicsScene*);
    void removePoint(QPointF position);
    void insertPoint(QPointF position);
    void closePolyline();
    void updatePath(); // Метод для обновления пути на основе позиций кругов
    QVariant itemChange(GraphicsItemChange change, const QVariant& value);

    void handlePointDeletion(DragCircle* circle);

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    int findClosestSegment(const QPointF& pos) const;
    void insertPointAtSegment(int segmentIndex, const QPointF& pos);

signals:
    void lineChanged(Polyline* line);

private slots:
    void dragCircleMove(DragCircle* circle);
    void dragCircleRelease(DragCircle* circle);

private:
    void updateConnections(); // Для обновления связей между точками
    void updateClosedState();

public:
    QVector<DragCircle*> _circles;
    bool _isClosed;

//    DragCircle* _circle1; // Начало линии
//    DragCircle* _circle2; // Переходная точка
//    DragCircle* _circle3; // Вторая переходная точка
//    DragCircle* _circle4; // Конец линии


    float _frameScale = 1.0;
    const qreal _minSegmentLength = 10.0; // Минимальная длина отрезка

    QColor _highlightColor = Qt::transparent; // Цвет выделения
};

} // namespace qgraph

