#include "video_rect.h"
#include "shape.h"

namespace qgraph {

VideoRect::VideoRect(QGraphicsScene* scene)
{
    setZValue(0);
    scene->addItem(this);
}

void VideoRect::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    for (QGraphicsItem* item : this->scene()->items())
        if (Shape* shape = dynamic_cast<Shape*>(item))
            shape->setActive(false);

    QGraphicsItem::mousePressEvent(event);
    update();
}

} // namespace qgraph
