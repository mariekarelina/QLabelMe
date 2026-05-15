#include "undo_stack.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/line.h"
#include "qgraphics2/point.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/rectangle.h"

// UndoAction::UndoAction(QGraphicsScene* scene, Type type, const QString& text,
//                        const ShapeBackup2& before, const ShapeBackup2& after,
//                        ApplyCallback onApplied)
// {
//     _scene = scene;
//     _type = type;
//     _text = text;
//     _before = before;
//     _after = after;
//     _onApplied = onApplied;
// }

// bool UndoAction::applyBackup(const ShapeBackup2& backup)
// {
//     qgraph::Shape* shape = findItem(backup.id);
//     if (!shape)
//         return false;

//     restoreFromBackup(shape, backup);

//     if (_onApplied)
//         _onApplied(shape);

//     return true;
// }

// void UndoAction::restoreFromBackup(qgraph::Shape* item, const ShapeBackup2& backup)
// {
//     QGraphicsItem* graphicsItem = dynamic_cast<QGraphicsItem*>(item);

//     if (!graphicsItem)
//         return;

//     // shapeNumber здесь пока не трогаем:
//     // _roleShapeNumber находится в MainWindow, а undo_stack.cpp его не видит.

//     graphicsItem->setData(0, backup.className);
//     item->setZLevel(backup.zValue);
//     item->shapeVisible(backup.visible);

//     switch (backup.kind)
//     {
//         case ShapeKind2::Rectangle:
//             if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
//             {
//                 rect->setRealSceneRect(backup.rect);

//                 rect->updateHandlePosition();
//                 rect->updatePointNumbers();
//             }
//             break;

//         case ShapeKind2::Circle:
//             if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
//             {
//                 circle->setRealRadius(backup.circleRadius);

//                 const QPointF currentCenter = circle->sceneBoundingRect().center();
//                 const QPointF targetCenter = backup.circleCenter;
//                 const QPointF delta = targetCenter - currentCenter;

//                 if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
//                     circle->moveBy(delta.x(), delta.y());

//                 circle->updateHandlePosition();
//             }
//             break;

//         case ShapeKind2::Polyline:
//             if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
//             {
//                 polyline->replaceScenePoints(backup.points, backup.closed);

//                 polyline->updateHandlePosition();
//                 polyline->updatePointNumbers();
//             }
//             break;

//         case ShapeKind2::Line:
//             if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
//             {
//                 line->replaceScenePoints(backup.points, backup.closed);
//                 line->setNumberingFromLast(backup.numberingFromLast);

//                 line->updateHandlePosition();
//                 line->updatePointNumbers();
//             }
//             break;

//         case ShapeKind2::Point:
//             if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
//             {
//                     point->setCenter(backup.pointCenter);

//                 point->updateHandlePosition();
//             }
//             break;
//     }
//     // TODO - отображение в ui

//     // if (ui && ui->polygonList)
//     // {
//     //     int currentRow = -1;
//     //     QListWidgetItem* currentItem = nullptr;

//     //     for (int i = 0; i < ui->polygonList->count(); ++i)
//     //     {
//     //         QListWidgetItem* li = ui->polygonList->item(i);
//     //         if (!li)
//     //             continue;

//     //         if (li->data(Qt::UserRole).value<QGraphicsItem*>() == item)
//     //         {
//     //             currentRow = i;
//     //             currentItem = li;
//     //             break;
//     //         }
//     //     }

//     //     if (!currentItem && !backup.className.isEmpty())
//     //     {
//     //         linkSceneItemToList(item, backup.listRow);
//     //     }
//     //     else if (currentItem)
//     //     {
//     //         const int targetRow = (backup.listRow < 0) ? currentRow : backup.listRow;
//     //         if (targetRow != currentRow &&
//     //             targetRow >= 0 &&
//     //             targetRow < ui->polygonList->count())
//     //         {
//     //             QListWidgetItem* moved = ui->polygonList->takeItem(currentRow);
//     //             ui->polygonList->insertItem(targetRow, moved);
//     //         }
//     //         renumberPolygonList();
//     //     }
//     // }
// }

// qgraph::Shape* UndoAction::findItem(const QUuidEx id) const
// {
//     if (!_scene || id.isNull())
//         return nullptr;

//     for (QGraphicsItem* item : _scene->items())
//     {
//         if (!item)
//             continue;

//         if (qgraph::Shape* shape = dynamic_cast<qgraph::Shape*>(item))
//         {
//             if (shape->id() == id)
//                 return shape;
//         }
//     }
//     return nullptr;
// }

// MoveShapeAction::MoveShapeAction(QGraphicsScene* scene,
//                                  const QString& text,
//                                  const ShapeBackup2& before,
//                                  const ShapeBackup2& after,
//                                  ApplyCallback onApplied)
//     : UndoAction(scene, Type::Move, text, before, after, onApplied)
// {
// }

// // redo() применяет состояние после перемещения
// void MoveShapeAction::redo()
// {
//     applyBackup(_after);
// }

// // undo() возвращает состояние до перемещения
// void MoveShapeAction::undo()
// {
//     applyBackup(_before);
// }


namespace undo {

qgraph::Shape* BaseUndo::findItem(const QUuidEx id) const
{
    if (!_scene || id.isNull())
        return nullptr;

    for (QGraphicsItem* item : _scene->items())
    {
        if (!item)
            continue;

        if (qgraph::Shape* shape = dynamic_cast<qgraph::Shape*>(item))
        {
            if (shape->id() == id)
                return shape;
        }
    }
    return nullptr;
}

// Create::Create(QGraphicsScene* scene, const QUuidEx& shapeId, const QString& text,
//                ShapeData::Ptr data)
Create::Create(QGraphicsScene* scene, const QUuidEx& shapeId, const QString& text,
       ShapeData::Ptr data)
{
    _scene = scene;
    //_shape = shape;
    _shapeIds.append(shapeId);
    _text = text;
    _data = data;

    setText(text);
    //_shape = findShape(_scene, _shapeId);
}

Create::~Create()
{
    for (qgraph::Shape* shape : _shapes)
        if (shape)
            delete shape;
}

void Create::undo()
{
    // if (_shape)
    //         return;

    //QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(_shape);
    for (const QUuidEx& shapeId : _shapeIds)
        if (qgraph::Shape* shape = findItem(shapeId))
        {
            QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
            _scene->removeItem(item);
            //_shape = shape;
            _shapes.append(shape);
        }


    // if (!item || !item->scene())
    //     return;

    // item->scene()->removeItem(item);
    // if (RectangleData::Ptr rect = _data.dynamic_cast_to<RectangleData::Ptr>())
    // {
    //     qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape);

    //     if (rectangle && rectangle->scene())
    //         rectangle->scene()->removeItem(rectangle);

    //     return;
    // }
    // else if (CircleData::Ptr circle = _data.dynamic_cast_to<CircleData::Ptr>())
    // {

    // }
    // else if (LineData::Ptr line = _data.dynamic_cast_to<LineData::Ptr>())
    // {

    // }
    // else if (PolylineData::Ptr pline = _data.dynamic_cast_to<PolylineData::Ptr>())
    // {

    // }
    // else if (PointData::Ptr point = _data.dynamic_cast_to<PointData::Ptr>())
    // {

    // }
}

void Create::redo()
{
    if (!_scene || !_data)
        return;

    if (_skipFirstRedo)
    {
        _skipFirstRedo = false;
        return;
    }
    for (qgraph::Shape* shape : _shapes)
        if (shape)
        {
            QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
            _scene->addItem(item);
            // _shape = nullptr;
            _shapes.removeAll(shape);
        }

    // QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(_shape);

    // if (!item)
    //     return;

    // if (!item->scene())
    //     _scene->addItem(item);

    // if (RectangleData::Ptr rect = _data.dynamic_cast_to<RectangleData::Ptr>())
    // {
    //     qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape);

    //     if (!rectangle)
    //     {
    //        rectangle = new qgraph::Rectangle(_scene);
    //        rectangle->setId(rect->id);

    //        _shape = rectangle;
    //        _shapeId = rectangle->id();
    //     }
    //     else if (!rectangle->scene())
    //     {
    //        _scene->addItem(rectangle);
    //     }

    //     rectangle->setData(0, rect->className);
    //     rectangle->setZLevel(rect->zLevel);
    //     rectangle->shapeVisible(rect->visible);

    //     rectangle->setRealSceneRect(rect->rect);
    //     rectangle->updateHandlePosition();
    //     rectangle->updatePointNumbers();
    // }
    // else if (CircleData::Ptr circle = _data.dynamic_cast_to<CircleData::Ptr>())
    // {

    // }
    // else if (LineData::Ptr line = _data.dynamic_cast_to<LineData::Ptr>())
    // {

    // }
    // else if (PolylineData::Ptr pline = _data.dynamic_cast_to<PolylineData::Ptr>())
    // {

    // }
    // else if (PointData::Ptr point = _data.dynamic_cast_to<PointData::Ptr>())
    // {

    // }
}

Delete::Delete(QGraphicsScene* scene,
               const QList<qgraph::Shape*>& shapes,
               QListWidget* listWidget,
               const QList<ListItemState>& listItems,
               const QString& text)
{
    _scene = scene;
    _listWidget = listWidget;
    _listItems = listItems;
    _text = text;

    for (qgraph::Shape* shape : shapes)
    {
        if (shape && !shape->id().isNull())
            _shapeIds.append(shape->id());
    }

    setText(text);
}

Delete::~Delete()
{
    for (qgraph::Shape* shape : _shapes)
        if (shape)
            delete shape;

    for (const ListItemState& state : _listItems)
        if (state.item && !state.item->listWidget())
            delete state.item;
}

void Delete::undo()
{
    if (!_scene)
        return;

    for (qgraph::Shape* shape : _shapes)
    {
        if (shape)
        {
            QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
            _scene->addItem(item);
            // Фигура снова на сцене, команда больше не должна владеть ею
            _shapes.removeAll(shape);
        }
    }

    if (!_listWidget)
        return;

    QList<ListItemState> listItems = _listItems;

    for (const ListItemState& state : listItems)
    {
        if (QListWidgetItem* listItem = state.item)
        {
            int row = state.row;

            if (row < 0 || row > _listWidget->count())
                row = _listWidget->count();

            _listWidget->insertItem(row, listItem);
            listItem->setSelected(true);
        }
    }
}

void Delete::redo()
{
    if (!_scene)
        return;

    for (const QUuidEx& shapeId : _shapeIds)
    {
        if (qgraph::Shape* shape = findItem(shapeId))
        {
            QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
            _scene->removeItem(item);

            // После removeItem() фигура больше не принадлежит сцене
            _shapes.append(shape);
        }
    }
    if (!_listWidget)
        return;

    for (int i = _listItems.size() - 1; i >= 0; --i)
    {
        QListWidgetItem* listItem = _listItems[i].item;

        const int row = _listWidget->row(listItem);

        if (row >= 0)
            _listWidget->takeItem(row);
    }
}

Move::Move(QGraphicsScene* scene, const QList<QUuidEx> shapeIds, const QString& text,
           const QPointF& delta)
{
    _scene = scene;
    //_shapeId = shapeId;
    _shapeIds = shapeIds;
    _text = text;
    _delta = delta;

    setText(text);
}

void Move::undo()
{    
    // if (!_scene || _shapeId.isNull())
    //     return;

    //qgraph::Shape* shape = nullptr;

    for (const QUuidEx& shapeId : _shapeIds)
        if (qgraph::Shape* shape = findItem(shapeId))
        {
            QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
            item->moveBy(-_delta.x(), -_delta.y());

            if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(shape))
            {
                rectangle->updateHandlePosition();
                rectangle->updatePointNumbers();
            }
            if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(shape))
            {
                circle->updateHandlePosition();
            }
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(shape))
            {
                line->updateHandlePosition();
                line->updatePointNumbers();
            }
            if (qgraph::Polyline* pline = dynamic_cast<qgraph::Polyline*>(shape))
            {
                pline->updateHandlePosition();
                pline->updatePointNumbers();
            }
            if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(shape))
            {
                point->updateHandlePosition();
            }
        }
}

void Move::redo()
{
    // if (!_scene || _shapeId.isNull())
    //     return;

    if (_skipFirstRedo)
    {
        _skipFirstRedo = false;
        return;
    }

    //qgraph::Shape* shape = nullptr;

    for (const QUuidEx& shapeId : _shapeIds)
        if (qgraph::Shape* shape = findItem(shapeId))
        {
            QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
            item->moveBy(_delta.x(), _delta.y());

            if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(shape))
            {
                rectangle->updateHandlePosition();
                rectangle->updatePointNumbers();
            }
            if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(shape))
            {
                circle->updateHandlePosition();
            }
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(shape))
            {
                line->updateHandlePosition();
                line->updatePointNumbers();
            }
            if (qgraph::Polyline* pline = dynamic_cast<qgraph::Polyline*>(shape))
            {
                pline->updateHandlePosition();
                pline->updatePointNumbers();
            }
            if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(shape))
            {
                point->updateHandlePosition();
            }
        }
}

} // namespace undo
