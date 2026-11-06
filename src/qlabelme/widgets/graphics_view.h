#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QColor>
#include <QLineF>
#include <QPen>

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

    void setSegmentPreview(const QLineF& line, const QPen& pen = QPen());

protected:
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private:
    Q_OBJECT

private:
    MainWindow* _mw = {nullptr};

    QLineF _segmentPreview;
    QPen _segmentPreviewPen;
};
