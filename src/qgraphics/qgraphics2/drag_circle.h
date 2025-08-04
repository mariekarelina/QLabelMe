#pragma once

#include "user_type.h"
//#include "shape.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QObject>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>

namespace qgraph {

class DragCircle : public QObject, public QGraphicsRectItem
{
    Q_OBJECT
public:
     enum {Type = toInt(qgraph::UserType::DragCircle)};
     int type() const override {return Type;}

    DragCircle(QGraphicsScene* scene);
    //explicit DragCircle(QGraphicsItem* parent = nullptr, bool isGhost = false);
    ~DragCircle();
    void setParent(QGraphicsItem* newParent);
    void raiseToTop();

    // Добавляем сеттеры для базовых размеров
    void setBaseSize(qreal size);

    void setSmallSize();
    void setLargeSize();

    // Переопределяем метод для определения столкновений
    bool collidesWithItem(const QGraphicsItem* other,
                          Qt::ItemSelectionMode mode = Qt::IntersectsItemShape) const override;
    // Увеличиваем область взаимодействия
    QPainterPath shape() const override;
    void setGhostDriven(bool on) { _ghostDriven = on; }
    static void applyHoverStyle(QGraphicsRectItem* item, bool active);
    // Сохранить текущий вид item как «базовый»
    static void rememberCurrentAsBase(QGraphicsRectItem* item);

    void setHoverStyle(bool hover);
    void restoreBaseStyle();

    QRectF baseRect() const { return _baseRect; }
    QPen basePen() const { return _basePen; }
    QBrush baseBrush() const { return _baseBrush; }

    void setHover(bool on);
    void setHoverSizingEnabled(bool on);
    bool hoverSizingEnabled() const { return _hoverSizingEnabled; }

    bool containsPoint(const QPointF &point) const;   // Проверка попадания в круг
    void setCenter(const QPointF &center);           // Перемещение центра

    int index() const { return m_index; }
    void setIndex(int idx) { m_index = idx; }


signals:
    void moved(DragCircle* circle);      // Сигнал, вызываемый при перемещении
    void released(DragCircle* circle);  // Сигнал, вызываемый при отпускании мыши
    void deleteRequested(DragCircle* circle); // Сигнал для удаления

    void hoverEntered();
    void hoverLeft();

    // эти сигналы будут генерироваться из MainWindow (или из самой ручки,
    // если вы захотите переместить emit внутрь DragCircle)
    void dragCircleMove(int index, const QPointF &scenePos);
    void dragCircleRelease(int index, const QPointF &scenePos);


protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    // void hoverEnterEvent(QGraphicsSceneHoverEvent* event) override;
    // void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;

    // void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    // void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    // void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;


private:
    bool isGhost() const { return _isGhost; }


public:
    QMenu* _contextMenu = nullptr;
    qreal _smallSize = 8.0;
    qreal _largeSize = 12.0;
    qreal _currentSize = 8.0;
    QRectF _baseRect;
    QPen   _basePen;
    QBrush _baseBrush;
    qreal _interactionRadius = 30.0; // Радиус области взаимодействия

    QGraphicsItem* _parentShape = nullptr; // Ссылка на родительскую фигуру
    bool _ghostDriven = false;
    bool _isGhost = false;

    // Роли в QVariant для хранения «базового» вида у любых QGraphicsRectItem
    static constexpr int kRoleBaseRect  = Qt::UserRole + 4200;
    static constexpr int kRoleBasePen   = Qt::UserRole + 4201;
    static constexpr int kRoleBaseBrush = Qt::UserRole + 4202;

    bool _hoverSizingEnabled = true;

    bool _isBeingDragged = false; // Добавить этот флаг
    int m_index = -1;


public:
    QPointF m_center;
    float m_radius;

    // int _radius = {8};
    // int _smallSize = {6};  // Маленький размер (видимый)
    // int _largeSize = {16}; // Большой размер (для захвата)
    // int _currentSize = _smallSize;
};

} // namespace qgraph
