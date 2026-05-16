#pragma once

#include "shared/clife_base.h"
#include "shared/clife_ptr.h"
#include "shared/qt/quuidex.h"
#include "qgraphics2/shape.h"
#include "widgets/document.h"

#include <QGraphicsScene>
#include <QUndoCommand>
//#include <QListWidget>
//#include <QListWidgetItem>

// Тип фигур
enum class ShapeKind2 {Rectangle, Circle, Polyline, Line, Point};

struct ShapeBackup2
{
    ShapeKind2 kind = {ShapeKind2::Rectangle};
    QUuidEx   id; // Идентификатор объекта, см класс Shape
    QString   className;
    qreal     zValue = 0;
    bool      visible = {true};

    // Геометрия по типам
    QRectF rect;
    QPointF circleCenter;
    qreal circleRadius = 0;
    QVector<QPointF> points;
    QPointF pointCenter;

    bool closed = false;
    bool numberingFromLast = false;

    int listRow = -1;  // Номер в правой панели
    int sceneRow = -1; // Порядок на сцене
    int shapeNumber = -1; // Номер в списке
};

// class UndoAction : public QUndoCommand
// {
// public:
//     enum class Type {Restore, Delete, Move, Edit};
//     // Callback нужен, чтобы MainWindow мог обновить ui
//     using ApplyCallback = std::function<void(qgraph::Shape*)>;

//     UndoAction(QGraphicsScene* scene, Type type, const QString& text,
//                const ShapeBackup2& before, const ShapeBackup2& after,
//                ApplyCallback onApplied);

//     // Применяет снимок к уже существующей фигуре
//     bool applyBackup(const ShapeBackup2& backup);
//     // Восстанавливает состояние фигуры
//     void restoreFromBackup(qgraph::Shape* item, const ShapeBackup2& backup);
//     // Ищет фигуру на сцене по qgraph::Shape::id()
//     qgraph::Shape* findItem(const QUuidEx id) const;


// protected:
//     QGraphicsScene* _scene = {nullptr};
//     Type _type = {Type::Restore};
//     QString _text;
//     ShapeBackup2 _before;
//     ShapeBackup2 _after;

//     ApplyCallback _onApplied;
// };

// class MoveShapeAction : public UndoAction
// {
// public:
//     MoveShapeAction(QGraphicsScene* scene, const QString& text,
//                     const ShapeBackup2& before, const ShapeBackup2& after,
//                     ApplyCallback onApplied = {});

//     void redo() override;
//     void undo() override;
// };

// class UndoRestore : public QUndoCommand
// {

// };

namespace undo {

// struct ShapeData : clife_base
// {
//     typedef clife_ptr<ShapeData> Ptr;

//     //ShapeKind2 kind = {ShapeKind2::Rectangle};
//     QUuidEx   id; // Идентификатор объекта, см класс Shape
//     QString   className;
//     int       zLevel = {0};
//     bool      visible = {true};

//     int listRow = -1;     // Номер в правой панели
//     int sceneRow = -1;    // Порядок на сцене
//     int shapeNumber = -1; // Номер в списке
// };

// struct RectangleData : ShapeData
// {
//     typedef clife_ptr<RectangleData> Ptr;

//     QRectF rect;
// };

// struct CircleData : ShapeData
// {
//     typedef clife_ptr<CircleData> Ptr;

//     QPointF circleCenter;
//     qreal   circleRadius = 0;
// };

// struct LineData : ShapeData
// {
//     typedef clife_ptr<LineData> Ptr;

//     QVector<QPointF> points;
//     bool numberingFromLast = false;
// };

// struct PolylineData : ShapeData
// {
//     typedef clife_ptr<PolylineData> Ptr;

//     QVector<QPointF> points;
//     bool closed = {false};
// };

// struct PointData : ShapeData
// {
//     typedef clife_ptr<PointData> Ptr;

//     QPointF pointCenter;
// };

class BaseUndo : public QUndoCommand //, public clife_base
{
public:
    // enum class Type {Restore, Delete, Move, Edit};
    // // Callback нужен, чтобы MainWindow мог обновить ui
    // using ApplyCallback = std::function<void(qgraph::Shape*)>;

    BaseUndo() : QUndoCommand(nullptr) {}
    ~BaseUndo();

protected:
    //QGraphicsScene* _scene = {nullptr};
    Document* _doc = {nullptr};
    QSet<QGraphicsItem*> _shapesCollector;

    // ApplyCallback _onApplied;
    bool firstRedo();
    bool _firstRedo = {true};
};

class Create : public BaseUndo
{
public:

    Create(Document* doc, QGraphicsItem* shape, const QString& text);
    ~Create();

    void undo() override;
    void redo() override;

private:
    QGraphicsItem* _shape = {nullptr};
    //quint64 shapeId = {0};
    //qgraph::Shape* _shape = {nullptr};
    //QList<qgraph::Shape*> _shapes;
};


class Delete : public BaseUndo
{
public:
    // struct ListItemState
    // {
    //     QListWidgetItem* item = {nullptr};
    //     int row = -1;
    // };

    // struct Data
    // {
    //     QListWidgetItem* item = {nullptr};
    //     int row = -1;
    // };

    struct RowState
    {
        QGraphicsItem* shape = {nullptr};

        // Строка фигуры в списке до удаления
        int row = {-1};

        // Строка модели
        QList<QStandardItem*> modelItems;
    };

    Delete(Document* doc, const QSet<QGraphicsItem*>& shapes,
           const QString& text);
    ~Delete();

    void undo() override;
    void redo() override;

private:
    QVector<RowState> _rows;
};

class Move : public BaseUndo
{
public:
    Move(Document* doc, QSet<QGraphicsItem*> shapes, const QString& text,
         const QPointF& delta);

    void undo() override;
    void redo() override;

private:
    QSet<QGraphicsItem*> _shapes;
    QPointF _delta;
};


class Edit : public BaseUndo
{

};


class ChangeClass : public BaseUndo
{

};

} // namespace undo

//Q_DECLARE_METATYPE(undo::Delete::Data)
