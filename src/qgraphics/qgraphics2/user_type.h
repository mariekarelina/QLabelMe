#pragma once
#include <QGraphicsItem>

namespace qgraph {

enum class UserType
{
    DragCircle = QGraphicsItem::UserType + 1,
    Circle     = QGraphicsItem::UserType + 2,
    Square     = QGraphicsItem::UserType + 3,
    Rectangle  = QGraphicsItem::UserType + 4,
    VideoRect  = QGraphicsItem::UserType + 5,
};

constexpr int toInt(UserType ut) {return static_cast<int>(ut);}

} // namespace qgraph
