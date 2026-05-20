#include "undo_stack.h"

#include "shared/list.h"
#include "shared/break_point.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/line.h"
#include "qgraphics2/point.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/rectangle.h"
#include "qgraphics2/drag_circle.h"

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

BaseUndo::~BaseUndo()
{
    for (QGraphicsItem* shape : _shapesCollector)
    {
        // if (shape && shape->scene())
        // {
        //     // Отладить TODO
        //     //break_point

        // }

        // if (shape && !shape->scene())
        //     delete shape;

        if (!shape)
            continue;

        // Если фигура находится на сцене, ее владельцем считается сцена
        // Здесь удаляем только те фигуры, которые были вынуты из сцены
        // и больше никем не будут уничтожены
        if (!shape->scene())
            delete shape;
    }
}

bool BaseUndo::firstRedo()
{
    if (_firstRedo)
    {
        _firstRedo = false;
        return true;
    }
    return false;
}

// qgraph::Shape* BaseUndo::findItem(const QUuidEx id) const
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

// Create::Create(QGraphicsScene* doc, const QUuidEx& shape, const QString& text,
//                ShapeData::Ptr data)
Create::Create(Document* doc, QGraphicsItem* shape, const QString& text)
{
    _doc = doc;
    _shape = shape;
    QUndoCommand::setText(text);
}

Create::~Create()
{
}

void Create::undo()
{
    //_scene->removeItem(_shape);
    _doc->scene->removeItem(_shape);
    _shapesCollector.insert(_shape);

    // //QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(_shape);
    // for (const QUuidEx& shapeId : _shapeIds)
    //     if (qgraph::Shape* shape = findItem(shapeId))
    //     {
    //         QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
    //         _scene->removeItem(item);
    //         //_shape = shape;
    //         _shapes.append(shape);
    //     }


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
    //if (!_scene /*|| !_data*/)
    //    return;

    if (firstRedo())
        return;

    _doc->scene->addItem(_shape);
    _shapesCollector.remove(_shape);

    // for (qgraph::Shape* shape : _shapes)
    //     if (shape)
    //     {
    //         QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
    //         _scene->addItem(item);
    //         // _shape = nullptr;
    //         _shapes.removeAll(shape);
    //     }

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

// Delete::Delete(QGraphicsScene* scene,
//                const QSet<QGraphicsItem*>& shapes,
//                QListWidget* listWidget,
//                const QList<ListItemState>& listItems,
//                const QString& text)
// {
//     _scene = scene;
//     _shapes = shapes;
//     _shapesCollector = shapes;
//     _listWidget = listWidget;
//     _listItems = listItems;
//     //_text = text;

//     // for (qgraph::Shape* shape : shapes)
//     // {
//     //     if (shape && !shape->id().isNull())
//     //         _shapeIds.append(shape->id());
//     // }

//     QUndoCommand::setText(text);
// }
Delete::Delete(Document* doc, const QSet<QGraphicsItem*>& shapes,
               const QString& text)
{
    _doc = doc;

    QUndoCommand::setText(text);

    // Проверяем состояние документа
    //if (!_doc || !_doc->scene || !_doc->polygonList.model)
    //    return;

    for (QGraphicsItem* shape : shapes)
    {
        if (!shape)
            continue;

        RowState state;
        state.shape = shape;

        // Запоминаем строку до удаления
        state.row = _doc->polygonList.items.indexOf(shape);

        if (state.row < 0)
            continue;

        _rows.append(state);
    }
    // Храним строки по возрастанию
    std::sort(_rows.begin(), _rows.end(),
              [](const RowState& left, const RowState& right)
    {
        return left.row < right.row;
    });
}

Delete::~Delete()
{
    // for (qgraph::Shape* shape : _shapes)
    //     if (shape)
    //         delete shape;

    // for (const ListItemState& state : _listItems)
    //     if (state.item && !state.item->listWidget())
    //         delete state.item;
    for (RowState& state : _rows)
    {
        for (QStandardItem* item : state.modelItems)
        {
            if (item && !item->model())
                delete item;
            state.modelItems.clear();
        }
    }
}

// void Delete::undo()
// {
//     if (!_scene)
//         return;

//     for (QGraphicsItem* shape : _shapes)
//     {
//         _scene->addItem(shape);
//         _shapesCollector.remove(shape);

//         QVariant vdata = shape->data(SHAPE_UNDO_DELETE_DATA);
//         if (vdata.canConvert<undo::Delete::Data>())
//         {
//             undo::Delete::Data data = vdata.value<undo::Delete::Data>();
//             // ...

//         }
//     }

//     if (!_listWidget)
//         return;

//     QList<ListItemState> listItems = _listItems;

//     for (const ListItemState& state : listItems)
//     {
//         if (QListWidgetItem* listItem = state.item)
//         {
//             int row = state.row;

//             //if (row < 0 || row > _listWidget->count())
//             if (!lst::inRange(row, 0, _listWidget->count()))
//                 row = _listWidget->count();

//             _listWidget->insertItem(row, listItem);
//             listItem->setSelected(true);
//         }
//     }
// }
void Delete::undo()
{
    //if (!_doc || !_doc->scene || !_doc->polygonList.model)
    //if (!_doc || !_doc->scene)
    //    return;

    // Восстанавливаем по возрастанию строк
    for (RowState& state : _rows)
    {
        if (!state.shape)
            continue;

        if (!state.shape->scene())
            _doc->scene->addItem(state.shape);

        // После восстановления фигура снова принадлежит сцене
        _shapesCollector.remove(state.shape);

        int row = state.row;

        // Если исходная строка стала некорректной
        if (row < 0 || row > _doc->polygonList.items.size())
            row = _doc->polygonList.items.size();

        // Восстанавливаем связь строки списка с фигурой
        if (!_doc->polygonList.items.contains(state.shape))
            _doc->polygonList.items.insert(row, state.shape);

        // Возвращаем строку модели обратно в QListView
        if (!state.modelItems.isEmpty())
        {
            _doc->polygonList.model.insertRow(row, state.modelItems);
            state.modelItems.clear();
        }
    }

    _doc->isModified = true;
}

// void Delete::redo()
// {
//     if (!_scene)
//         return;

//     if (firstRedo())
//         return;

//     for (QGraphicsItem* shape : _shapes)
//     {
//         _scene->removeItem(shape);
//         _shapesCollector.insert(shape);
//     }

//     // for (const QUuidEx& shapeId : _shapeIds)
//     // {
//     //     if (qgraph::Shape* shape = findItem(shapeId))
//     //     {
//     //         QGraphicsItem* item = dynamic_cast<QGraphicsItem*>(shape);
//     //         _scene->removeItem(item);

//     //         // После removeItem() фигура больше не принадлежит сцене
//     //         _shapes.append(shape);
//     //     }
//     // }
//     if (!_listWidget)
//         return;

//     for (int i = _listItems.size() - 1; i >= 0; --i)
//     {
//         QListWidgetItem* listItem = _listItems[i].item;

//         const int row = _listWidget->row(listItem);

//         if (row >= 0)
//             _listWidget->takeItem(row);
//     }
// }
void Delete::redo()
{
    //if (!_doc || !_doc->scene || !_doc->polygonList.model)
    //    return;

    // Удаляем с конца, чтобы индексы строк не смещались
    for (int i = _rows.size() - 1; i >= 0; --i)
    {
        RowState& state = _rows[i];

        if (!state.shape)
            continue;

        if (state.shape->scene())
            _doc->scene->removeItem(state.shape);

        // Фигура удалена со сцены, передана команде
        _shapesCollector.insert(state.shape);

        // Удаляем связь строки списка с фигурой
        const int currentRow = _doc->polygonList.items.indexOf(state.shape);
        if (currentRow >= 0)
            _doc->polygonList.items.removeAt(currentRow);

        // Забираем строку из модели QListView
        if (state.row >= 0 && state.row < _doc->polygonList.model.rowCount())
            state.modelItems = _doc->polygonList.model.takeRow(state.row);
    }

    _doc->isModified = true;
}

Move::Move(Document* doc, QSet<QGraphicsItem*> shapes, const QString& text,
           const QPointF& delta)
{
    _doc = doc;
    _shapes = shapes;
    _delta = delta;
    QUndoCommand::setText(text);
}

void Move::undo()
{    
    // if (!_scene || _shapeId.isNull())
    //     return;

    //qgraph::Shape* shape = nullptr;

    for (QGraphicsItem* shape : _shapes)
    {
        shape->moveBy(-_delta.x(), -_delta.y());

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

    if (firstRedo())
        return;

    for (QGraphicsItem* shape : _shapes)
    {
        shape->moveBy(_delta.x(), _delta.y());

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

Edit::Edit(Document* doc, QGraphicsItem* shape,
           const Data& data, const QString& text)
{
    _doc = doc;
    _shape = shape;
    _data = data;
    QUndoCommand::setText(text);
}

void Edit::undo()
{
    switch (_data.type)
    {
    case Type::MovePoint:
    {
        if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(_shape))
        {
            point->setCenter(point->center() - _data.delta);
            point->updateHandlePosition();
        }

        break;
    }

    case Type::MoveLineNode:
    {
        if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape))
        {
            const QVector<qgraph::DragCircle*> circles = line->circles();

            if (_data.pointIndex < 0 || _data.pointIndex >= circles.size())
                break;

            qgraph::DragCircle* circle = circles[_data.pointIndex];

            if (!circle)
                break;

            const QPointF newScenePos = circle->scenePos() - _data.delta;
            circle->setPos(line->mapFromScene(newScenePos));

            line->updatePath();
            line->updatePointNumbers();
        }

        break;
    }

    case Type::MovePolylineNode:
    {
        if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape))
        {
            const QVector<qgraph::DragCircle*> circles = polyline->circles();

            if (_data.pointIndex < 0 || _data.pointIndex >= circles.size())
                break;

            qgraph::DragCircle* circle = circles[_data.pointIndex];

            if (!circle)
                break;

            const QPointF newScenePos = circle->scenePos() - _data.delta;
            circle->setPos(polyline->mapFromScene(newScenePos));

            polyline->updatePath();
            polyline->updatePointNumbers();
        }

        break;
    }

    case Type::EditRectangle:
    {
        if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape))
        {
            rectangle->setRealSceneRect(_data.rectBefore);
            rectangle->updateHandlePosition();
            rectangle->updatePointNumbers();
        }

        break;
    }

    case Type::EditCircle:
    {
        if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(_shape))
        {
            circle->setRealRadius(_data.circleRadiusBefore);
            circle->updateHandlePosition();
            circle->updateCrossLines();
        }

        break;
    }
    }
}

void Edit::redo()
{
    // Первый redo вызывается автоматически при push()
    if (firstRedo())
        return;

    switch (_data.type)
    {
    case Type::MovePoint:
    {
        if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(_shape))
        {
            point->setCenter(point->center() + _data.delta);
            point->updateHandlePosition();
        }

        break;
    }

    case Type::MoveLineNode:
    {
        if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape))
        {
            const QVector<qgraph::DragCircle*> circles = line->circles();

            if (_data.pointIndex < 0 || _data.pointIndex >= circles.size())
                break;

            qgraph::DragCircle* circle = circles[_data.pointIndex];

            if (!circle)
                break;

            const QPointF newScenePos = circle->scenePos() + _data.delta;
            circle->setPos(line->mapFromScene(newScenePos));

            line->updatePath();
            line->updatePointNumbers();
        }

        break;
    }

    case Type::MovePolylineNode:
    {
        if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape))
        {
            const QVector<qgraph::DragCircle*> circles = polyline->circles();

            if (_data.pointIndex < 0 || _data.pointIndex >= circles.size())
                break;

            qgraph::DragCircle* circle = circles[_data.pointIndex];

            if (!circle)
                break;

            const QPointF newScenePos = circle->scenePos() + _data.delta;
            circle->setPos(polyline->mapFromScene(newScenePos));

            polyline->updatePath();
            polyline->updatePointNumbers();
        }

        break;
    }

    case Type::EditRectangle:
    {
        if (qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape))
        {
            rectangle->setRealSceneRect(_data.rectAfter);
            rectangle->updateHandlePosition();
            rectangle->updatePointNumbers();
        }

        break;
    }

    case Type::EditCircle:
    {
        if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(_shape))
        {
            circle->setRealRadius(_data.circleRadiusAfter);
            circle->updateHandlePosition();
            circle->updateCrossLines();
        }

        break;
    }
    }
}

NodeEdit::NodeEdit(Document* doc, QGraphicsItem* shape,
                   const Data& data, const QString& text)
{
    _doc = doc;
    _shape = shape;
    _data = data;

    QUndoCommand::setText(text);
}

void NodeEdit::undo()
{
    switch (_data.type)
    {
    case Type::InsertLineNode:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (line)
            line->removePointAtIndex(_data.pointIndex);

        break;
    }

    case Type::RemoveLineNode:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (line)
            line->insertPointAtIndex(_data.pointIndex, _data.point);

        break;
    }

    case Type::InsertPolylineNode:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (polyline)
            polyline->removePointAtIndex(_data.pointIndex);

        break;
    }

    case Type::RemovePolylineNode:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (polyline)
            polyline->insertPointAtIndex(_data.pointIndex, _data.point);

        break;
    }
    }
}

void NodeEdit::redo()
{
    if (firstRedo())
        return;

    switch (_data.type)
    {
    case Type::InsertLineNode:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (line)
            line->insertPointAtIndex(_data.pointIndex, _data.point);

        break;
    }

    case Type::RemoveLineNode:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (line)
            line->removePointAtIndex(_data.pointIndex);

        break;
    }

    case Type::InsertPolylineNode:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (polyline)
            polyline->insertPointAtIndex(_data.pointIndex, _data.point);

        break;
    }

    case Type::RemovePolylineNode:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (polyline)
            polyline->removePointAtIndex(_data.pointIndex);

        break;
    }
    }
}

Visibility::Visibility(Document* doc,
                       const QVector<QGraphicsItem*>& shapes,
                       bool visible,
                       const QString& text)
{
    _doc = doc;
    _shapes = shapes;
    _visible = visible;

    QUndoCommand::setText(text);
}

void Visibility::undo()
{
    for (QGraphicsItem* shape : _shapes)
    {
        if (!shape)
            continue;

        shape->setVisible(!_visible);

        if (_visible)
            shape->setSelected(false);
    }

    if (_doc && _doc->scene)
        _doc->scene->update();
}

void Visibility::redo()
{
    if (firstRedo())
        return;

    for (QGraphicsItem* shape : _shapes)
    {
        if (!shape)
            continue;

        shape->setVisible(_visible);

        if (!_visible)
            shape->setSelected(false);
    }

    if (_doc && _doc->scene)
        _doc->scene->update();
}

ChangeClass::ChangeClass(Document* doc, QGraphicsItem* shape,
                         const Data& data, const QString& text)
{
    _doc = doc;
    _shape = shape;
    _data = data;

    QUndoCommand::setText(text);
}

void ChangeClass::undo()
{
    if (!_shape)
        return;

    _shape->setData(0, _data.classBefore);

    if (_shape->scene())
        _shape->scene()->update();
}

void ChangeClass::redo()
{
    if (firstRedo())
        return;

    if (!_shape)
        return;

    _shape->setData(0, _data.classAfter);

    if (_shape->scene())
        _shape->scene()->update();
}

NumberingEdit::NumberingEdit(Document* doc, QGraphicsItem* shape,
                             const Data& data, const QString& text)
{
    _doc = doc;
    _shape = shape;
    _data = data;

    QUndoCommand::setText(text);
}

void NumberingEdit::undo()
{
    switch (_data.type)
    {
    case Type::Line:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (!line)
            break;

        line->setNumberingFromLast(_data.lineNumberingFromLastBefore);
        line->updatePointNumbers();

        break;
    }
    case Type::Polyline:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (!polyline)
            break;

        qgraph::DragCircle* start = nullptr;
        const QVector<qgraph::DragCircle*>& circles = polyline->circles();

        for (qgraph::DragCircle* circle : circles)
        {
            if (!circle)
                continue;

            if (QLineF(circle->scenePos(), _data.polylineFirstPointBefore).length() < 0.001)
            {
                start = circle;
                break;
            }
        }

        if (!start)
            break;

        if (_data.polylineClockwiseBefore)
            polyline->renumberFromHandleClockwise(start);
        else
            polyline->renumberFromHandleCounterClockwise(start);

        break;
    }
    case Type::Rectangle:
    {
        qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape);

        if (!rectangle)
            break;

        rectangle->setNumberingOffset(_data.rectangleNumberingOffsetBefore);

        break;
    }
    }

    if (_doc && _doc->scene)
        _doc->scene->update();
}

void NumberingEdit::redo()
{
    if (firstRedo())
        return;

    switch (_data.type)
    {
    case Type::Line:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (!line)
            break;

        line->setNumberingFromLast(_data.lineNumberingFromLastAfter);
        line->updatePointNumbers();

        break;
    }
    case Type::Polyline:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (!polyline)
            break;

        qgraph::DragCircle* start = nullptr;
        const QVector<qgraph::DragCircle*>& circles = polyline->circles();

        for (qgraph::DragCircle* circle : circles)
        {
            if (!circle)
                continue;

            if (QLineF(circle->scenePos(), _data.polylineFirstPointAfter).length() < 0.001)
            {
                start = circle;
                break;
            }
        }

        if (!start)
            break;

        if (_data.polylineClockwiseAfter)
            polyline->renumberFromHandleClockwise(start);
        else
            polyline->renumberFromHandleCounterClockwise(start);

        break;
    }
    case Type::Rectangle:
    {
        qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape);

        if (!rectangle)
            break;

        rectangle->setNumberingOffset(_data.rectangleNumberingOffsetAfter);

        break;
    }
    }

    if (_doc && _doc->scene)
        _doc->scene->update();
}

} // namespace undo
