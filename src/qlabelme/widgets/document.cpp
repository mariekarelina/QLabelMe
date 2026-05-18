#include "document.h"

Document::Document()
{
    //scene = new QGraphicsScene();
    scene = simple_ptr<QGraphicsScene>::create();
}

Document::~Document()
{
    //delete scene;
    undoStack2.clear();
    _undoStack.reset();
}

Document::Ptr Document::create(const QString& path)
{
    //Ptr doc {new Document};
    Ptr doc = Ptr::create();
    doc->filePath = path;

    //doc->polygonList.model.reset(new QStandardItemModel);
    //doc->polygonList.model = std::unique_ptr(new QStandardItemModel);
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
    //if (!scene)
    //{
    //    scene = new QGraphicsScene();
    //}
    if (!videoRect)
    {
        videoRect = new qgraph::VideoRect(scene);
    }
    videoRect->setPixmap(pixmap);

    videoRect->setPos(0, 0);
    scene->setSceneRect(QRectF(QPointF(0, 0), QSizeF(pixmap.size())));
    return true;
}
