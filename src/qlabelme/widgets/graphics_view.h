#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>

class MainWindow;

class GraphicsView : public QGraphicsView
{
public:
    explicit GraphicsView(QWidget* parent = nullptr);

    bool init(MainWindow*);

    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;

private:
    Q_OBJECT

private:
    MainWindow* _mw = {nullptr};
};
