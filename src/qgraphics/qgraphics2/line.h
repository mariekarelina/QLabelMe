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

class Line : public ShapeT<QGraphicsPathItem>
{
public:
    enum {Type = toInt(qgraph::UserType::Line)};
    int type() const override {return Type;}

    Line(QGraphicsScene*, const QPointF&);
    ~Line();

    void setFrameScale(float newScale) override;
    void setRealSceneRect(const QRectF&);
    void updateHandlePosition();

    void addPoint(const QPointF&, QGraphicsScene*);

    void beginBulkLoad() { _loadingMode = true; }
    void endBulkLoad();

    void removePoint(QPointF position);
    void insertPoint(QPointF position);
    void closeLine();
    bool isClickOnAnyPoint(const QPointF& scenePos, int* idx = nullptr) const;
    void updatePath(); // Метод для обновления пути на основе позиций кругов
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    // Undo/Redo узлов в режиме рисования
    bool removeLastPointForce(bool callCallback = true);
    void setClosed(bool closed, bool callCallback = true);

    void handlePointDeletion(DragCircle* circle);
    bool isNumberingFromLast() const { return _numberingFromLast; }
    void setNumberingFromLast(bool v);

    enum class CloseMode
    {
        DoubleClick,      // 1) По двойному клику ЛКМ
        CtrlModifier,     // 3) Через Ctrl
        KeyC              // 4) Нажатие 'C'
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
    void resumeFromHandle(DragCircle* h);

    void togglePointNumbers();
    void setGlobalPointNumbersVisible(bool visible);

    bool isPointNumbersVisible() const { return _pointNumbersVisible; }

    void toggleSelectionRect();
    bool isSelectionRectVisible() const { return _selectionRectVisible; }

    void setGlobalSelectionRectVisible(bool visible);

    void updateHandlesZValue();
    void raiseHandlesToTop() override;
    void moveToBack();

    // Callback для уведомлений об изменениях
    void setModificationCallback(std::function<void()> callback);

    const QVector<DragCircle*>& circles() const { return _circles; }

    void replaceScenePoints(const QVector<QPointF>& scenePts, bool closed);

protected:
    // Переопределяем обработчик событий клавиатуры
    void keyPressEvent(QKeyEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    int findClosestSegment(const QPointF& pos) const;
    void insertPointAtSegment(int segmentIndex, const QPointF& pos);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
              QWidget* widget = nullptr) override;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;

    void updateConnections(); // Для обновления связей между точками
    void updateClosedState();
    void updatePointNumbersAfterReorder(); // Обновление номеров после изменения порядка

signals:
    void lineChanged(Line* line);
    void LineModified();

private:
    void dragCircleMove(DragCircle* circle) override;
    void dragCircleRelease(DragCircle* circle) override;
    QColor selectionFillColor() const;
    void updateSelectionRect();

public:
    QVector<DragCircle*> _circles;
    bool _isClosed;

    bool _loadingMode = false;

    float _frameScale = 1.0;
    const qreal _minSegmentLength = 10.0; // Минимальная длина отрезка

    static CloseMode s_closeMode;

    QColor _highlightColor = Qt::transparent; // Цвет выделения
    bool _hovered = false;
    QList<QGraphicsSimpleTextItem*> pointNumbers;
    QList<QGraphicsRectItem*> numberBackgrounds;
    qreal _numberFontSize = 10.0; // Размер шрифта по умолчанию
    QColor _numberColor = Qt::white;
    QColor _numberBgColor = QColor(0, 0, 0, 180);

    bool _pointNumbersVisible = false; // Видимости нумерации
    bool _globalPointNumbersVisible = false;
    QGraphicsRectItem* _selectionRect = nullptr;
    bool _selectionRectVisible = true;
    bool _globalSelectionRectVisible = true;

    std::function<void()> _modificationCallback; // Callback для уведомлений

private:
    bool _numberingFromLast = false;
};

} // namespace qgraph

