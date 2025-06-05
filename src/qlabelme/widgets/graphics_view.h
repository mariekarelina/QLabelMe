#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QGraphicsItem>
#include <QWheelEvent>
#include <QSlider>

class MainWindow;

class GraphicsView : public QGraphicsView
{
public:
    explicit GraphicsView(QWidget* parent = nullptr);

    bool init(MainWindow*);

    //virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

    qreal getCurrentZoom() const
    {
        return _currentZoom;
    }

    void setZoom(qreal factor)
    {
        QTransform t;
        t.scale(factor, factor);
        setTransform(t);
        _currentZoom = factor;
    }

private:
    Q_OBJECT


private:
    QGraphicsItem* _draggingItem; // Указатель на перетаскиваемый объект
    QPoint _lastPos; // Хранит последнюю позицию мыши

    QSlider* _zoomSlider;
    MainWindow* _mw = {nullptr};

    qreal _currentZoom = 1.0;

};
