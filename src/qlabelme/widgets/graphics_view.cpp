#include "graphics_view.h"
#include "main_window.h"

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
    _mw->graphicsView_mouseMoveEvent(mouseEvent, this);

    QGraphicsView::mouseMoveEvent(mouseEvent);

}

void GraphicsView::mousePressEvent(QMouseEvent* mouseEvent)
{
    _mw->graphicsView_mousePressEvent(mouseEvent, this);

    QGraphicsView::mousePressEvent(mouseEvent);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    _mw->graphicsView_mouseReleaseEvent(mouseEvent, this);

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
        //setZoom(getcurrentZoom() * 1.1);
        // Указываем, что событие обработано
        wheelEvent->accept();
    }
    QGraphicsView::wheelEvent(wheelEvent);
}
