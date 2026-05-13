#include "undo_stack.h"

#include "qgraphics2/circle.h"
#include "qgraphics2/line.h"
#include "qgraphics2/point.h"
#include "qgraphics2/polyline.h"
#include "qgraphics2/rectangle.h"

UndoAction::UndoAction(QGraphicsScene* scene, Type type, const QString& text,
                       const ShapeBackup2& before, const ShapeBackup2& after,
                       ApplyCallback onApplied)
{
    _scene = scene;
    _type = type;
    _text = text;
    _before = before;
    _after = after;
    _onApplied = onApplied;
}

bool UndoAction::applyBackup(const ShapeBackup2& backup)
{
    qgraph::Shape* shape = findItem(backup.id);
    if (!shape)
        return false;

    restoreFromBackup(shape, backup);

    if (_onApplied)
        _onApplied(shape);

    return true;
}

void UndoAction::restoreFromBackup(qgraph::Shape* item, const ShapeBackup2& backup)
{
    QGraphicsItem* graphicsItem = dynamic_cast<QGraphicsItem*>(item);

    if (!graphicsItem)
        return;

    // shapeNumber здесь пока не трогаем:
    // _roleShapeNumber находится в MainWindow, а undo_stack.cpp его не видит.

    graphicsItem->setData(0, backup.className);
    item->setZLevel(backup.zValue);
    item->shapeVisible(backup.visible);

    switch (backup.kind)
    {
        case ShapeKind2::Rectangle:
            if (qgraph::Rectangle* rect = dynamic_cast<qgraph::Rectangle*>(item))
            {
                rect->setRealSceneRect(backup.rect);

                rect->updateHandlePosition();
                rect->updatePointNumbers();
            }
            break;

        case ShapeKind2::Circle:
            if (qgraph::Circle* circle = dynamic_cast<qgraph::Circle*>(item))
            {
                circle->setRealRadius(backup.circleRadius);

                const QPointF currentCenter = circle->sceneBoundingRect().center();
                const QPointF targetCenter = backup.circleCenter;
                const QPointF delta = targetCenter - currentCenter;

                if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
                    circle->moveBy(delta.x(), delta.y());

                circle->updateHandlePosition();
            }
            break;

        case ShapeKind2::Polyline:
            if (qgraph::Polyline* polyline = dynamic_cast<qgraph::Polyline*>(item))
            {
                polyline->replaceScenePoints(backup.points, backup.closed);

                polyline->updateHandlePosition();
                polyline->updatePointNumbers();
            }
            break;

        case ShapeKind2::Line:
            if (qgraph::Line* line = dynamic_cast<qgraph::Line*>(item))
            {
                line->replaceScenePoints(backup.points, backup.closed);
                line->setNumberingFromLast(backup.numberingFromLast);

                line->updateHandlePosition();
                line->updatePointNumbers();
            }
            break;

        case ShapeKind2::Point:
            if (qgraph::Point* point = dynamic_cast<qgraph::Point*>(item))
            {
                    point->setCenter(backup.pointCenter);

                point->updateHandlePosition();
            }
            break;
    }
    // TODO - отображение в ui

    // if (ui && ui->polygonList)
    // {
    //     int currentRow = -1;
    //     QListWidgetItem* currentItem = nullptr;

    //     for (int i = 0; i < ui->polygonList->count(); ++i)
    //     {
    //         QListWidgetItem* li = ui->polygonList->item(i);
    //         if (!li)
    //             continue;

    //         if (li->data(Qt::UserRole).value<QGraphicsItem*>() == item)
    //         {
    //             currentRow = i;
    //             currentItem = li;
    //             break;
    //         }
    //     }

    //     if (!currentItem && !backup.className.isEmpty())
    //     {
    //         linkSceneItemToList(item, backup.listRow);
    //     }
    //     else if (currentItem)
    //     {
    //         const int targetRow = (backup.listRow < 0) ? currentRow : backup.listRow;
    //         if (targetRow != currentRow &&
    //             targetRow >= 0 &&
    //             targetRow < ui->polygonList->count())
    //         {
    //             QListWidgetItem* moved = ui->polygonList->takeItem(currentRow);
    //             ui->polygonList->insertItem(targetRow, moved);
    //         }
    //         renumberPolygonList();
    //     }
    // }
}

qgraph::Shape* UndoAction::findItem(const QUuidEx id) const
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

MoveShapeAction::MoveShapeAction(QGraphicsScene* scene,
                                 const QString& text,
                                 const ShapeBackup2& before,
                                 const ShapeBackup2& after,
                                 ApplyCallback onApplied)
    : UndoAction(scene, Type::Move, text, before, after, onApplied)
{
}

// redo() применяет состояние после перемещения
void MoveShapeAction::redo()
{
    applyBackup(_after);
}

// undo() возвращает состояние до перемещения
void MoveShapeAction::undo()
{
    applyBackup(_before);
}


namespace undo {

// Create::Create(QGraphicsScene* scene, const QUuidEx& shapeId, const QString& text,
//                ShapeData::Ptr data)
Create::Create(QGraphicsScene* scene, qgraph::Shape* shape, const QString& text,
       ShapeData::Ptr data)
{
    _scene = scene;
    _shape = shape;
    if (_shape)
            _shapeId = _shape->id();
    _text = text;
    _data = data;
    //_before = before;
    //_after = after;
    //_onApplied = onApplied;
    setText(text);
    //_shape = findShape(_scene, _shapeId);
}

void Create::undo()
{
    if (RectangleData::Ptr rect = _data.dynamic_cast_to<RectangleData::Ptr>())
    {
        qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape);

        if (rectangle && rectangle->scene())
            rectangle->scene()->removeItem(rectangle);

        return;
    }
    else if (auto circle = _data.dynamic_cast_to<CircleData::Ptr>())
    {

    }
    else if (auto line = _data.dynamic_cast_to<LineData::Ptr>())
    {

    }
    else if (auto pline = _data.dynamic_cast_to<PolylineData::Ptr>())
    {

    }
    else if (auto point = _data.dynamic_cast_to<PointData::Ptr>())
    {

    }

}

void Create::redo()
{
    if (!_scene || !_data)
        return;

    // QUndoStack::push() сразу вызывает redo()
    // Но при создании прямоугольника объект уже находится на сцене,
    // поэтому первый redo() должен ничего не делать
    if (_skipFirstRedo)
    {
        _skipFirstRedo = false;
        return;
    }

    if (RectangleData::Ptr rect = _data.dynamic_cast_to<RectangleData::Ptr>())
    {
        qgraph::Rectangle* rectangle = dynamic_cast<qgraph::Rectangle*>(_shape);

        if (!rectangle)
        {
           rectangle = new qgraph::Rectangle(_scene);
           rectangle->setId(rect->id);

           _shape = rectangle;
           _shapeId = rectangle->id();
        }
        else if (!rectangle->scene())
        {
           _scene->addItem(rectangle);
        }

        rectangle->setData(0, rect->className);
        rectangle->setZLevel(rect->zLevel);
        rectangle->shapeVisible(rect->visible);

        rectangle->setRealSceneRect(rect->rect);
        rectangle->updateHandlePosition();
        rectangle->updatePointNumbers();
    }
    else if (auto circle = _data.dynamic_cast_to<CircleData::Ptr>())
    {

    }
    else if (auto line = _data.dynamic_cast_to<LineData::Ptr>())
    {

    }
    else if (auto pline = _data.dynamic_cast_to<PolylineData::Ptr>())
    {

    }
    else if (auto point = _data.dynamic_cast_to<PointData::Ptr>())
    {

    }
}


} // namespace undo
