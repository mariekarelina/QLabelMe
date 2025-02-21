#pragma once

#include "user_type.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>

namespace qgraph {

class DragCircle : public QGraphicsEllipseItem
{
public:
    enum {Type = toInt(qgraph::UserType::DragCircle)};
    int type() const override {return Type;}

    DragCircle(QGraphicsScene* scene);
    void setParent(QGraphicsItem* newParent);

private:
    void mouseMoveEvent(QGraphicsSceneMouseEvent*) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

    int _radius = {8};
};

} // namespace qgraph
