#pragma once

#include "user_type.h"

#include <QtCore>
#include <QGraphicsItem>
#include <QGraphicsScene>

namespace qgraph {

class VideoRect : public QGraphicsPixmapItem
{
public:
    enum {Type = toInt(qgraph::UserType::VideoRect)};
    int type() const override {return Type;}

    VideoRect(QGraphicsScene*);

private:
    void mousePressEvent(QGraphicsSceneMouseEvent*) override;
};

} // namespace qgraph
