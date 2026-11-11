#include "graphics_view.h"
#include "main_window.h"

#include <QGraphicsLineItem>

class GraphicsView::SegmentPreviewItem : public QObject, public QGraphicsLineItem
{
public:
    explicit SegmentPreviewItem(QGraphicsItem* parent)
        : QGraphicsLineItem(parent)
    {
        setAcceptedMouseButtons(Qt::NoButton);

        // Рисуем отрезок ниже дочерних узлов фигуры
        setZValue(-1.0);
    }

    QRectF boundingRect() const override
    {
        // Сохраняем область перерисовки с учетом толщины пера
        return QGraphicsLineItem::shape().controlPointRect();
    }

    QPainterPath shape() const override
    {
        // Исключаем предпросмотр из поиска элементов под курсором
        return QPainterPath();
    }
};

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

void GraphicsView::setSegmentPreview(QGraphicsItem* shape,
                                     const QLineF& line,
                                     const QPen& pen)
{
    if (line.isNull())
    {
        clearSegmentPreview();
        return;
    }

    if (!_segmentPreview || _segmentPreview->parentItem() != shape)
    {
        clearSegmentPreview();
        _segmentPreview = new SegmentPreviewItem(shape);
    }

    _segmentPreview->setPen(pen);
    _segmentPreview->setLine(line);
}

void GraphicsView::clearSegmentPreview()
{
    delete _segmentPreview.data();
}
