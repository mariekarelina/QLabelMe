#pragma once

#include "user_type.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QObject>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>

namespace qgraph {

class DragCircle : public QObject, public QGraphicsEllipseItem
{
    Q_OBJECT
public:
    enum {Type = toInt(qgraph::UserType::DragCircle)};
    int type() const override {return Type;}

    DragCircle(QGraphicsScene* scene);
    void setParent(QGraphicsItem* newParent);

signals:
    void moved(DragCircle* circle);      // Сигнал, вызываемый при перемещении
    void released(DragCircle* circle);  // Сигнал, вызываемый при отпускании мыши
    void deleteRequested(DragCircle* circle); // Сигнал для удаления

protected:
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

private:
    int _radius = {8};
    QMenu* _contextMenu;
};

} // namespace qgraph
