#include "document.h"

Document::Document()
{
    scene = simple_ptr<QGraphicsScene>::create();
}

Document::~Document()
{
    _undoStack.reset();
}

Document::Ptr Document::create(const QString& path)
{
    Ptr doc = Ptr::create();
    doc->filePath = path;
    doc->polygonList.model.setColumnCount(1);

    return doc;
}

bool Document::loadImage()
{
    pixmap = QPixmap(filePath);
    if (pixmap.isNull())
    {
        return false;
    }
    if (!videoRect)
    {
        videoRect = new qgraph::VideoRect(scene);
    }
    videoRect->setPixmap(pixmap);

    videoRect->setPos(0, 0);
    scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(pixmap.size())));
    return true;
}
