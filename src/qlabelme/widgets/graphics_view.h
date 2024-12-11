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

private:
    Q_OBJECT


private:
    QGraphicsItem* _draggingItem; // Указатель на перетаскиваемый объект
    QPoint _lastPos; // Хранит последнюю позицию мыши

    QSlider* _zoomSlider;
    MainWindow* _mw;

};
