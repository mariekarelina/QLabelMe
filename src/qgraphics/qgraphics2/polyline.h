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

    QVector<QPointF> points() const;
    void updatePointNumbers();
    QPointF findBestDirection(const QPointF& pointPos, const QPointF& initialDirection, int pointIndex);
    int countIntersections(const QLineF& testLine, int excludePointIndex);
    void applyNumberStyle(qreal fontSize, const QColor& textColor, const QColor& bgColor);
    void rotatePointsClockwise();
    void rotatePointsCounterClockwise();
    void handleKeyPressEvent(QKeyEvent* event);

    void togglePointNumbers();
    bool isPointNumbersVisible() const { return _pointNumbersVisible; }

    void updateHandlesZValue();
    void raiseHandlesToTop() override;
    void moveToBack();

    // callback для уведомлений об изменениях
    void setModificationCallback(std::function<void()> callback);

    const QVector<DragCircle*>& circles() const { return _circles; }

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    int findClosestSegment(const QPointF& pos) const;
    void insertPointAtSegment(int segmentIndex, const QPointF& pos);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

signals:
    void lineChanged(Polyline* line);
    void polylineModified();

private:
    void updateConnections(); // Для обновления связей между точками
    void updateClosedState();
    void updatePointNumbersAfterReorder(); // Обновление номеров после изменения порядка

    void dragCircleMove(DragCircle* circle) override;
    void dragCircleRelease(DragCircle* circle) override;
    QColor selectionFillColor() const;

public:
    QVector<DragCircle*> _circles;
    bool _isClosed;

    float _frameScale = 1.0;
    const qreal _minSegmentLength = 10.0; // Минимальная длина отрезка

    QColor _highlightColor = Qt::transparent; // Цвет выделения
    QList<QGraphicsSimpleTextItem*> pointNumbers;
    QList<QGraphicsRectItem*> numberBackgrounds;
    qreal _numberFontSize = 10.0; // Размер шрифта по умолчанию
    QColor _numberColor = Qt::white;
    QColor _numberBgColor = QColor(0, 0, 0, 180);

    bool _pointNumbersVisible = true; // Видимости нумерации
    std::function<void()> _modificationCallback; // callback для уведомлений
};

} // namespace qgraph

