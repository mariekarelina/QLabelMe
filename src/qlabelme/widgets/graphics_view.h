#pragma once

#include <QGraphicsView>

class GraphicsView : public QGraphicsView
{
public:
    explicit GraphicsView(QWidget* parent = nullptr);

    //virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* mouseEvent);
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    Q_OBJECT

};
