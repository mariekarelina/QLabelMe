#pragma once

#include <QGraphicsView>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPointer>
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

    void setSegmentPreview(QGraphicsItem* shape, const QLineF& line, const QPen& pen);
    void clearSegmentPreview();

private:
    Q_OBJECT

private:
    MainWindow* _mw = {nullptr};

    class SegmentPreviewItem;
    QPointer<SegmentPreviewItem> _segmentPreview;
};
