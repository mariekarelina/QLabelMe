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
    bool isClickOnFirstPoint(const QPointF& scenePos) const; // точка 0
    bool isClickOnAnyPoint(const QPointF& scenePos, int* idx = nullptr) const;
    void updatePath(); // Метод для обновления пути на основе позиций кругов
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    // Undo/Redo узлов в режиме рисования
    bool removeLastPointForce(bool callCallback = true);
    void setClosed(bool closed, bool callCallback = true);

    void handlePointDeletion(DragCircle* circle);

    enum class CloseMode
    {
        DoubleClick,      // 1) По двойному клику ЛКМ
        SingleClickOnFirstPoint,    // 2) По одинарному клику ЛКМ на нулевой точке
        CtrlModifier,               // 3) Через Ctrl
        KeyC                        // 4) Нажатие 'C'
    };

    bool isClosed() const { return _isClosed; }
    static void setGlobalCloseMode(CloseMode m) { s_closeMode = m; }
    static CloseMode globalCloseMode() { return s_closeMode; }

    QVector<QPointF> points() const;
    void updatePointNumbers();
    QPointF findBestDirection(const QPointF& pointPos, const QPointF& initialDirection, int pointIndex);
    int countIntersections(const QLineF& testLine, int excludePointIndex);
    void applyNumberStyle(qreal fontSize, const QColor& textColor, const QColor& bgColor);
    void rotatePointsClockwise();
    void rotatePointsCounterClockwise();
    void handleKeyPressEvent(QKeyEvent* event);
    void resumeFromHandle(qgraph::DragCircle* h);

    void togglePointNumbers();
    bool isPointNumbersVisible() const { return _pointNumbersVisible; }
    void toggleSelectionRect();
    bool isSelectionRectVisible() const { return _selectionRectVisible; }

    void updateHandlesZValue();
    void raiseHandlesToTop() override;
    void moveToBack();

    // Сallback для уведомлений об изменениях
    void setModificationCallback(std::function<void()> callback);

    const QVector<DragCircle*>& circles() const { return _circles; }

    void replaceScenePoints(const QVector<QPointF>& scenePts, bool closed);


protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    int findClosestSegment(const QPointF& pos) const;
    void insertPointAtSegment(int segmentIndex, const QPointF& pos);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
               QWidget* widget = nullptr) override;

    void updateConnections(); // Для обновления связей между точками
    void updateClosedState();
    void updatePointNumbersAfterReorder(); // Обновление номеров после изменения порядка

signals:
    void lineChanged(Polyline* line);
    void polylineModified();

private:
    void dragCircleMove(DragCircle* circle) override;
    void dragCircleRelease(DragCircle* circle) override;
    QColor selectionFillColor() const;
    void updateSelectionRect();

public:
    QVector<DragCircle*> _circles;
    bool _isClosed;

    float _frameScale = 1.0;
    const qreal _minSegmentLength = 10.0; // Минимальная длина отрезка

    static CloseMode s_closeMode;

    QColor _highlightColor = Qt::transparent; // Цвет выделения
    QList<QGraphicsSimpleTextItem*> pointNumbers;
    QList<QGraphicsRectItem*> numberBackgrounds;
    qreal _numberFontSize = 10.0; // Размер шрифта по умолчанию
    QColor _numberColor = Qt::white;
    QColor _numberBgColor = QColor(0, 0, 0, 180);

    bool _pointNumbersVisible = true; // Видимости нумерации
    QGraphicsRectItem* _selectionRect = nullptr;
    bool _selectionRectVisible = true;

    bool _closeCallbackScheduled = false;

    std::function<void()> _modificationCallback; // Сallback для уведомлений
};

} // namespace qgraph

