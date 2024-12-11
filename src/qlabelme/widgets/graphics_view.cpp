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
//    if (_draggingItem)
//    {
//        // Вычисляем смещение
//        QPointF newPos = mapToScene(mouseEvent->pos());
//        QPointF offset = newPos - mapToScene(_lastPos);
//        // Перемещаем элемент в пределах границ сцены
//        _draggingItem->moveBy(offset.x(), offset.y());
//        _lastPos = mouseEvent->pos();
//    }
//    QGraphicsView::mouseMoveEvent(mouseEvent);


    this->setDragMode(QGraphicsView::RubberBandDrag
                              /*: QGraphicsView::ScrollHandDrag*/);
    // Это свойство определяет, позволяет ли представление
    // взаимодействовать со сценой.
    //graphicsView->setInteractive(selectModeButton->isChecked());
    this->setInteractive(true);
    QGraphicsView::mouseMoveEvent(mouseEvent);
}

void GraphicsView::mousePressEvent(QMouseEvent* mouseEvent)
{
    _mw->graphicsView_mousePressEvent(mouseEvent, this);
//    this->setCursor(QCursor(Qt::ClosedHandCursor));
//    if (mouseEvent->button() == Qt::LeftButton
//        && (mouseEvent->modifiers() & Qt::ShiftModifier))
//    {
//        // Проверяем, есть ли элемент под курсором мыши
//        _draggingItem = itemAt(mouseEvent->pos());
//        // Возвращает указатель
//        qgraph::VideoRect* videoRect = dynamic_cast<qgraph::VideoRect*>(_draggingItem);
//        if (videoRect)
//        {
//            // Запоминаем начальную позицию курсора
//            _lastPos = mouseEvent->pos();
//        }
//    }
////    else
////    {
////        QGraphicsView::mousePressEvent(mouseEvent);
////    }
//    // Если нажали кнопку на main_window
//    if (_mw->_btnRectFlag)
//    {
//        QGraphicsScene* scene = this->scene();
//        if (scene)
//        {
//            QPointF _scenePos;
//            double size = 50; // Размер стороны квадрата
//            scene->addRect(QRectF(_scenePos.x() - size / 2, _scenePos.y() - size / 2, size, size),
//                           QPen(Qt::red));
//        }
//    }

//    if (_mw->_btnLineFlag)
//    {
//        QGraphicsScene* scene = this->scene();
//        if (scene)
//        {
//            QPointF _scenePos;
//            scene->addLine(QLine(_scenePos.x() - 50 / 2, _scenePos.y() - 50 / 2, 100, 30),
//                           QPen(Qt::green));
//        }
//    }

//    QGraphicsView::mousePressEvent(mouseEvent);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
    this->setCursor(QCursor(Qt::ArrowCursor));
    if (mouseEvent->button() == Qt::LeftButton)
    {
        // Сбрасываем указатель на перетаскиваемый элемент
        _draggingItem = nullptr;
    }
    QGraphicsView::mouseReleaseEvent(mouseEvent);
}

void GraphicsView::wheelEvent(QWheelEvent* wheelEvent)
{
    if (wheelEvent->modifiers() & Qt::ControlModifier)
    {
        // Получаем позицию курсора в координатах сцены
        QPointF scenePos = mapToScene(wheelEvent->pos());
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
