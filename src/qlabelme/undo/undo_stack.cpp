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
namespace {

double distanceSquaredToSegment(const QPointF& point,
                                const QPointF& first,
                                const QPointF& second)
{
    const double vx = second.x() - first.x();
    const double vy = second.y() - first.y();
    const double wx = point.x() - first.x();
    const double wy = point.y() - first.y();

    const double segmentLengthSquared = vx * vx + vy * vy;

    double t = 0.0;

    if (segmentLengthSquared > 1e-12)
        t = (wx * vx + wy * vy) / segmentLengthSquared;

    if (t < 0.0)
        t = 0.0;

    if (t > 1.0)
        t = 1.0;

    const double px = first.x() + t * vx;
    const double py = first.y() + t * vy;

    const double dx = point.x() - px;
    const double dy = point.y() - py;

    return dx * dx + dy * dy;
}

QVector<QPointF> reorderedPolylinePoints(qgraph::Polyline* polyline,
                                         const QPointF& scenePos)
{
    QVector<QPointF> points = polyline->points();

    const int pointsCount = points.size();

    if (!polyline->isClosed() || pointsCount < 3)
        return points;

    int bestIndex = 0;
    double bestDistance = -1.0;

    for (int i = 0; i < pointsCount; ++i)
    {
        const QPointF& first = points[i];
        const QPointF& second = points[(i + 1) % pointsCount];

        const double distance = distanceSquaredToSegment(scenePos, first, second);

        if (bestDistance < 0.0 || distance < bestDistance)
        {
            bestDistance = distance;
            bestIndex = i;
        }
    }

    QVector<QPointF> reordered;
    reordered.reserve(pointsCount);

    const int startIndex = (bestIndex + 1) % pointsCount;

    for (int i = 0; i < pointsCount; ++i)
        reordered.append(points[(startIndex + i) % pointsCount]);

    return reordered;
}

void copyCommonShapeState(QGraphicsItem* sourceShape,
                          QGraphicsItem* targetShape)
{
    if (!sourceShape || !targetShape)
        return;

    // Класс фигуры в проекте хранится в data(0).
    // Служебные роли MainWindow здесь не трогаем.
    targetShape->setData(0, sourceShape->data(0));

    targetShape->setZValue(sourceShape->zValue());
    targetShape->setVisible(sourceShape->isVisible());
    targetShape->setFlags(sourceShape->flags());

    if (qgraph::Polyline* sourcePolyline = dynamic_cast<qgraph::Polyline*>(sourceShape))
    {
        if (qgraph::Line* targetLine = dynamic_cast<qgraph::Line*>(targetShape))
        {
            targetLine->setPen(sourcePolyline->pen());
        }
    }
    else if (qgraph::Line* sourceLine = dynamic_cast<qgraph::Line*>(sourceShape))
    {
        if (qgraph::Polyline* targetPolyline = dynamic_cast<qgraph::Polyline*>(targetShape))
        {
            targetPolyline->setPen(sourceLine->pen());
        }
    }
}

bool hasVisiblePointNumbers(QGraphicsItem* shape)
{
    if (!shape)
        return false;

    const QList<QGraphicsItem*> children = shape->childItems();

    for (QGraphicsItem* child : children)
    {
        QGraphicsSimpleTextItem* text = dynamic_cast<QGraphicsSimpleTextItem*>(child);

        if (text && text->isVisible() && !text->text().isEmpty())
            return true;
    }

    return false;
}

void setPointNumbersVisible(qgraph::Line* line, bool visible)
{
    if (!line)
        return;

    line->updatePointNumbers();

    if (hasVisiblePointNumbers(line) != visible)
    {
        line->togglePointNumbers();
        line->updatePointNumbers();
    }
}

void setPointNumbersVisible(qgraph::Polyline* polyline, bool visible)
{
    if (!polyline)
        return;

    polyline->updatePointNumbers();

    if (hasVisiblePointNumbers(polyline) != visible)
    {
        polyline->togglePointNumbers();
        polyline->updatePointNumbers();
    }
}

} // namespace


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

FinishDrawing::FinishDrawing(Document* doc,
                             QGraphicsItem* shape,
                             Type type,
                             const QString& text)
{
    _doc = doc;
    _shape = shape;
    _type = type;

    if (_doc)
        _row = _doc->polygonList.items.indexOf(_shape);

    QUndoCommand::setText(text);
}

void FinishDrawing::undo()
{
    if (!_doc || !_shape)
        return;

    setClosed(false);
    removeFromList();

    if (_doc->scene)
        _doc->scene->update();
}

void FinishDrawing::redo()
{
    if (firstRedo())
        return;

    if (!_doc || !_shape)
        return;

    setClosed(true);
    restoreToList();

    if (_doc->scene)
        _doc->scene->update();
}

void FinishDrawing::setClosed(bool closed)
{
    if (_type == Type::Line)
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (!line)
            return;

        line->setClosed(closed, false);
        line->setFlag(QGraphicsItem::ItemIsMovable, closed);
        line->setSelected(true);
        line->setFocus();

        line->setNumberingFromLast(false);
        line->updatePath();
        line->updateHandlePosition();
        line->updatePointNumbers();

        return;
    }

    if (_type == Type::Polyline)
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (!polyline)
            return;

        polyline->setClosed(closed, false);
        polyline->setFlag(QGraphicsItem::ItemIsMovable, closed);
        polyline->setSelected(true);
        polyline->setFocus();

        polyline->updatePath();
        polyline->updateHandlePosition();
        polyline->updatePointNumbers();
    }
}

void FinishDrawing::removeFromList()
{
    if (!_doc || !_shape)
        return;

    const int currentRow = _doc->polygonList.items.indexOf(_shape);

    if (currentRow >= 0)
    {
        _row = currentRow;
        _doc->polygonList.items.removeAt(currentRow);
    }
}

void FinishDrawing::restoreToList()
{
    if (!_doc || !_shape)
        return;

    if (_doc->polygonList.items.contains(_shape))
        return;

    int row = _row;

    if (row < 0 || row > _doc->polygonList.items.size())
        row = _doc->polygonList.items.size();

    _doc->polygonList.items.insert(row, _shape);
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

Paste::Paste(Document* doc,
             const QList<QGraphicsItem*>& shapes,
             const QString& text)
{
    _doc = doc;

    QUndoCommand::setText(text);

    if (!_doc)
        return;

    for (QGraphicsItem* shape : shapes)
    {
        if (!shape)
            continue;

        Data state;
        state.shape = shape;
        state.row = _doc->polygonList.items.indexOf(shape);

        if (state.row < 0)
            continue;

        _rows.append(state);
    }

    std::sort(_rows.begin(), _rows.end(),
              [](const Data& left, const Data& right)
    {
        return left.row < right.row;
    });
}

Paste::~Paste()
{
    for (Data& state : _rows)
    {
        for (QStandardItem* item : state.modelItems)
        {
            if (item && !item->model())
                delete item;
        }

        state.modelItems.clear();
    }
}

void Paste::undo()
{
    if (!_doc || !_doc->scene)
        return;

    // Удаляем с конца, чтобы строки списка не смещались
    for (int i = _rows.size() - 1; i >= 0; --i)
    {
        Data& state = _rows[i];

        if (!state.shape)
            continue;

        const int currentRow = _doc->polygonList.items.indexOf(state.shape);

        if (currentRow >= 0)
        {
            state.row = currentRow;
            _doc->polygonList.items.removeAt(currentRow);

            if (currentRow < _doc->polygonList.model.rowCount())
                state.modelItems = _doc->polygonList.model.takeRow(currentRow);
        }

        if (state.shape->scene())
            _doc->scene->removeItem(state.shape);

        // После undo фигура временно принадлежит команде
        _shapesCollector.insert(state.shape);
    }

    if (_doc->scene)
        _doc->scene->update();
}

void Paste::redo()
{
    if (firstRedo())
        return;

    if (!_doc || !_doc->scene)
        return;

    // Восстанавливаем по возрастанию строк.
    for (Data& state : _rows)
    {
        if (!state.shape)
            continue;

        if (!state.shape->scene())
            _doc->scene->addItem(state.shape);

        _shapesCollector.remove(state.shape);

        int row = state.row;

        if (row < 0 || row > _doc->polygonList.items.size())
            row = _doc->polygonList.items.size();

        if (!_doc->polygonList.items.contains(state.shape))
            _doc->polygonList.items.insert(row, state.shape);

        if (!state.modelItems.isEmpty())
        {
            _doc->polygonList.model.insertRow(row, state.modelItems);
            state.modelItems.clear();
        }
    }

    if (_doc->scene)
        _doc->scene->update();
}

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

        Data state;
        state.shape = shape;

        // Запоминаем строку до удаления
        state.row = _doc->polygonList.items.indexOf(shape);

        if (state.row < 0)
            continue;

        _rows.append(state);
    }
    // Храним строки по возрастанию
    std::sort(_rows.begin(), _rows.end(),
              [](const Data& left, const Data& right)
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
    for (Data& state : _rows)
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
    for (Data& state : _rows)
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
        Data& state = _rows[i];

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

Replace::Replace(Document* doc,
                 QGraphicsItem* shape,
                 const Data& data,
                 const QString& text)
{
    _doc = doc;
    _beforeShape = shape;
    _data = data;

    if (_doc)
        _row = _doc->polygonList.items.indexOf(_beforeShape);

    _afterShape = createReplacement();

    if (_afterShape)
        _shapesCollector.insert(_afterShape);

    QUndoCommand::setText(text);
}

bool Replace::isValid() const
{
    return _doc && _doc->scene && _beforeShape && _afterShape && _row >= 0;
}

QGraphicsItem* Replace::createReplacement() const
{
    if (!_doc || !_doc->scene || !_beforeShape)
        return nullptr;

    if (_data.type == Type::PolylineToLine)
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_beforeShape);

        if (!polyline)
            return nullptr;

        const bool pointNumbersVisible = hasVisiblePointNumbers(polyline);

        const QVector<QPointF> points = reorderedPolylinePoints(polyline, _data.scenePos);

        if (points.size() < 2)
            return nullptr;

        qgraph::Line* line = new qgraph::Line(_doc->scene, points.first());

        line->beginBulkLoad();

        for (int i = 1; i < points.size(); ++i)
            line->addPoint(points[i], _doc->scene);

        line->endBulkLoad();
        line->closeLine();
        line->setNumberingFromLast(false);

        copyCommonShapeState(polyline, line);

        line->updateHandlePosition();
        setPointNumbersVisible(line, pointNumbersVisible);

        if (line->scene())
            line->scene()->removeItem(line);

        line->setSelected(false);

        return line;
    }

    if (_data.type == Type::LineToPolyline)
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_beforeShape);

        if (!line)
            return nullptr;

        const bool pointNumbersVisible = hasVisiblePointNumbers(line);

        const QVector<QPointF> points = line->points();

        if (points.size() < 2)
            return nullptr;

        qgraph::Polyline* polyline = new qgraph::Polyline(_doc->scene, points.first());

        polyline->beginBulkLoad();

        for (int i = 1; i < points.size(); ++i)
            polyline->addPoint(points[i], _doc->scene);

        polyline->endBulkLoad();
        polyline->closePolyline();

        copyCommonShapeState(line, polyline);

        polyline->updateHandlePosition();
        setPointNumbersVisible(polyline, pointNumbersVisible);

        if (polyline->scene())
            polyline->scene()->removeItem(polyline);

        polyline->setSelected(false);

        return polyline;
    }

    return nullptr;
}

void Replace::undo()
{
    replace(_afterShape, _beforeShape);
}

void Replace::redo()
{
    replace(_beforeShape, _afterShape);
}

void Replace::replace(QGraphicsItem* leavingShape, QGraphicsItem* enteringShape)
{
    if (!isValid() || !leavingShape || !enteringShape)
        return;

    int row = _doc->polygonList.items.indexOf(leavingShape);

    if (row < 0)
        row = _row;

    if (row < 0 || row >= _doc->polygonList.items.size())
        return;

    leavingShape->setSelected(false);

    if (leavingShape->scene())
        _doc->scene->removeItem(leavingShape);

    if (!enteringShape->scene())
        _doc->scene->addItem(enteringShape);

    enteringShape->setSelected(true);
    enteringShape->setFocus();

    _doc->polygonList.items[row] = enteringShape;

    _shapesCollector.insert(leavingShape);
    _shapesCollector.remove(enteringShape);

    if (_doc->scene)
        _doc->scene->update();
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

ResumeEdit::ResumeEdit(Document* doc, QGraphicsItem* shape,
                       const Data& data, const QString& text)
{
    _doc = doc;
    _shape = shape;
    _data = data;

    QUndoCommand::setText(text);
}

void ResumeEdit::undo()
{
    switch (_data.type)
    {
    case Type::Line:
    {
        qgraph::Line* line = dynamic_cast<qgraph::Line*>(_shape);

        if (!line)
            break;

        line->setClosed(false, false);

        if (_data.resumeIndex >= 0)
        {
            const int insertIndex = _data.resumeIndex + 1;

            for (int i = 0; i < _data.removedPoints.size(); ++i)
                line->insertPointAtIndex(insertIndex + i, _data.removedPoints[i]);

            const int removeIndex = insertIndex + _data.removedPoints.size();

            for (int i = 0; i < _data.addedPoints.size(); ++i)
                line->removePointAtIndex(removeIndex);
        }

        line->setClosed(_data.closedBefore, false);
        line->setFlag(QGraphicsItem::ItemIsMovable, _data.closedBefore);

        line->updatePath();
        line->updatePointNumbers();

        break;
    }
    case Type::Polyline:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (!polyline)
            break;

        polyline->setClosed(false, false);

        if (_data.resumeIndex >= 0)
        {
            const int insertIndex = _data.resumeIndex + 1;

            for (int i = 0; i < _data.removedPoints.size(); ++i)
                polyline->insertPointAtIndex(insertIndex + i, _data.removedPoints[i]);

            const int removeIndex = insertIndex + _data.removedPoints.size();

            for (int i = 0; i < _data.addedPoints.size(); ++i)
                polyline->removePointAtIndex(removeIndex);
        }

        polyline->setClosed(_data.closedBefore, false);
        polyline->setFlag(QGraphicsItem::ItemIsMovable, _data.closedBefore);

        polyline->updatePath();
        polyline->updatePointNumbers();

        break;
    }
    }

    if (_doc && _doc->scene)
        _doc->scene->update();
}

void ResumeEdit::redo()
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

        line->setClosed(false, false);

        if (_data.resumeIndex >= 0)
        {
            const int insertIndex = _data.resumeIndex + 1;

            for (int i = 0; i < _data.addedPoints.size(); ++i)
                line->insertPointAtIndex(insertIndex + i, _data.addedPoints[i]);

            const int removeIndex = insertIndex + _data.addedPoints.size();

            for (int i = 0; i < _data.removedPoints.size(); ++i)
                line->removePointAtIndex(removeIndex);
        }

        line->setClosed(_data.closedAfter, false);
        line->setFlag(QGraphicsItem::ItemIsMovable, _data.closedAfter);

        line->updatePath();
        line->updatePointNumbers();

        break;
    }
    case Type::Polyline:
    {
        qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(_shape);

        if (!polyline)
            break;

        polyline->setClosed(false, false);

        if (_data.resumeIndex >= 0)
        {
            const int insertIndex = _data.resumeIndex + 1;

            for (int i = 0; i < _data.addedPoints.size(); ++i)
                polyline->insertPointAtIndex(insertIndex + i, _data.addedPoints[i]);

            const int removeIndex = insertIndex + _data.addedPoints.size();

            for (int i = 0; i < _data.removedPoints.size(); ++i)
                polyline->removePointAtIndex(removeIndex);
        }

        polyline->setClosed(_data.closedAfter, false);
        polyline->setFlag(QGraphicsItem::ItemIsMovable, _data.closedAfter);

        polyline->updatePath();
        polyline->updatePointNumbers();

        break;
    }
    }

    if (_doc && _doc->scene)
        _doc->scene->update();
}

ListOrder::ListOrder(Document* doc,
                     QGraphicsItem* shape,
                     int fromRow,
                     int toRow,
                     const QString& text)
{
    _doc = doc;
    _shape = shape;
    _fromRow = fromRow;
    _toRow = toRow;

    QUndoCommand::setText(text);
}

void ListOrder::undo()
{
    if (!_doc || !_shape)
        return;

    QVector<QGraphicsItem*>& items = _doc->polygonList.items;
    QStandardItemModel& model = _doc->polygonList.model;

    const int currentRow = items.indexOf(_shape);

    if (currentRow < 0)
        return;

    if (_fromRow < 0 || _fromRow >= items.size())
        return;

    if (currentRow == _fromRow)
        return;

    if (currentRow >= model.rowCount())
        return;

    if (_fromRow >= model.rowCount())
        return;

    QList<QStandardItem*> modelRow = model.takeRow(currentRow);

    if (modelRow.isEmpty())
        return;

    QGraphicsItem* shape = items.takeAt(currentRow);
    items.insert(_fromRow, shape);

    model.insertRow(_fromRow, modelRow);

    if (_doc->scene)
        _doc->scene->update();
}

void ListOrder::redo()
{
    if (!_doc || !_shape)
        return;

    QVector<QGraphicsItem*>& items = _doc->polygonList.items;
    QStandardItemModel& model = _doc->polygonList.model;

    const int currentRow = items.indexOf(_shape);

    if (currentRow < 0)
        return;

    if (_toRow < 0 || _toRow >= items.size())
        return;

    if (currentRow == _toRow)
        return;

    if (currentRow >= model.rowCount())
        return;

    if (_toRow >= model.rowCount())
        return;

    QList<QStandardItem*> modelRow = model.takeRow(currentRow);

    if (modelRow.isEmpty())
        return;

    QGraphicsItem* shape = items.takeAt(currentRow);
    items.insert(_toRow, shape);

    model.insertRow(_toRow, modelRow);

    if (_doc->scene)
        _doc->scene->update();
}

} // namespace undo
