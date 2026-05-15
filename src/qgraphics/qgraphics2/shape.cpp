#include "shape.h"
#include <atomic>

quint64 qgraph::shapeId()
{
    static std::atomic_uint64_t shapeId_ {0};
    return ++shapeId_;
}
