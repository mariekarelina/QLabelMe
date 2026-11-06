#include "graphics_view.h"
#include "main_window.h"

#include <QRegion>

GraphicsView::GraphicsView(QWidget* parent) : QGraphicsView(parent)
{
}

bool GraphicsView::init(MainWindow* mw)
{
    _mw = mw;
    return true;
}

void GraphicsView::mouseMoveEvent(QMouseEvent* mouseEvent)
{
    if (_mw)
    {
        _mw->graphicsView_mouseMoveEvent(mouseEvent, this);
        if (mouseEvent->isAccepted())
            return;
    }
    QGraphicsView::mouseMoveEvent(mouseEvent);
}

void GraphicsView::mousePressEvent(QMouseEvent* mouseEvent)
{
    if (_mw)
    {
        _mw->graphicsView_mousePressEvent(mouseEvent, this);
        if (mouseEvent->isAccepted())
            return;
    }
    QGraphicsView::mousePressEvent(mouseEvent);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    if (_mw)
    {
        _mw->graphicsView_mouseReleaseEvent(mouseEvent, this);
        if (mouseEvent->isAccepted())
            return;
    }
    QGraphicsView::mouseReleaseEvent(mouseEvent);
}

void GraphicsView::wheelEvent(QWheelEvent* wheelEvent)
{
    if (wheelEvent->modifiers() & Qt::ControlModifier)
    {
        // Получаем позицию курсора в координатах сцены
        QPointF scenePos = mapToScene(wheelEvent->position().toPoint());
        if (wheelEvent->angleDelta().y() > 0)
        {
            scale(1.1, 1.1);
        }
        else
        {
            scale(1.0 / 1.1, 1.0 / 1.1);
        }
        // Сбрасываем вид, чтобы курсор оставался на одной и той же позиции в сцене
        this->centerOn(scenePos);
        // Указываем, что событие обработано
        wheelEvent->accept();
    }
    QGraphicsView::wheelEvent(wheelEvent);
}

void GraphicsView::setSegmentPreview(const QLineF& line, const QPen& pen)
{
    if (_segmentPreview == line && _segmentPreviewPen == pen)
        return;

    // Учитываем толщину обоих отрезков, чтобы полностью стереть старый
    const int oldMargin = qCeil(_segmentPreviewPen.widthF()) + 2;
    const int newMargin = qCeil(pen.widthF()) + 2;

    QRegion dirty;
    if (!_segmentPreview.isNull())
    {
        const QRect oldRect(mapFromScene(_segmentPreview.p1()),
                            mapFromScene(_segmentPreview.p2()));
        dirty += oldRect.normalized().adjusted(
            -oldMargin, -oldMargin, oldMargin, oldMargin);
    }

    if (!line.isNull())
    {
        const QRect newRect(mapFromScene(line.p1()), mapFromScene(line.p2()));
        dirty += newRect.normalized().adjusted(
            -newMargin, -newMargin, newMargin, newMargin);
    }

    _segmentPreview = line;
    _segmentPreviewPen = pen;
    viewport()->update(dirty);
}

void GraphicsView::drawForeground(QPainter* painter, const QRectF& rect)
{
    QGraphicsView::drawForeground(painter, rect);

    if (_segmentPreview.isNull())
        return;

    painter->save();

    // Сохраняем толщину отрезка при изменении масштаба
    painter->setPen(_segmentPreviewPen);
    painter->setPen(_segmentPreviewPen);
    painter->drawLine(_segmentPreview);

    painter->restore();
}
