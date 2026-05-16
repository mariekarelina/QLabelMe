#pragma once

#include "shared/container_ptr.h"
#include "qgraphics2/video_rect.h"

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QPixmap>
#include <QPointF>
#include <QStandardItemModel>
#include <QString>
#include <QUndoStack>
#include <QVector>

#include <memory>

struct PolygonListData
{
    QVector<QGraphicsItem*> items; // Порядок фигур в правой панели
    std::unique_ptr<QStandardItemModel> model; // Модель, которую отображает QListView
};

struct Document
{
    typedef container_ptr<Document> Ptr;

    QString filePath;                            // Путь к файлу изображения
    QGraphicsScene* scene = {nullptr};           // Сцена с изображением и разметкой
    qgraph::VideoRect* videoRect = {nullptr};
    QPixmap pixmap;                              // Само изображение
    QGraphicsPixmapItem* pixmapItem = {nullptr}; // Элемент на сцене
    bool isModified = {false};                   // Есть ли несохраненные изменения

    std::unique_ptr<QUndoStack> _undoStack;
    QUndoStack undoStack2;

    PolygonListData polygonList; // Данные списка фигур для текущего документа

    struct
    {
        int hScroll = 0;
        int vScroll = 0;
        qreal zoom = 0.0;
        QPointF center;
    } viewState;

    static Ptr create(const QString& path);
    bool loadImage();
};

Q_DECLARE_METATYPE(Document::Ptr)
