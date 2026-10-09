#pragma once

#include "qgraphics2/video_rect.h"
#include "shared/container_ptr.h"
#include "shared/simple_ptr.h"
#include "shared/defmac.h"

#include <QStandardItemModel>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QUndoStack>
#include <QPixmap>
#include <QPointF>
#include <QString>
#include <QVector>

#include <memory>

struct PolygonListData
{
    QVector<QGraphicsItem*> items; // Порядок фигур в правой панели
    QStandardItemModel model;      // Модель, которую отображает QListView
};

struct Document
{
    typedef container_ptr<Document> Ptr;

    Document();
    ~Document();

    DISABLE_DEFAULT_COPY(Document)

    QString filePath;                            // Путь к файлу изображения
    simple_ptr<QGraphicsScene> scene;
    qgraph::VideoRect* videoRect = {nullptr};
    QPixmap pixmap;                              // Само изображение
    bool isModified = {false};                   // Есть ли несохраненные изменения

    PolygonListData polygonList; // Данные списка фигур для текущего документа

    std::unique_ptr<QUndoStack> _undoStack;

    struct
    {
        int hScroll = {0};
        int vScroll = {0};
        qreal zoom  = {0};
        QPointF center;
    } viewState;

    static Ptr create(const QString& path);
    bool loadImage();
};

Q_DECLARE_METATYPE(Document::Ptr)
