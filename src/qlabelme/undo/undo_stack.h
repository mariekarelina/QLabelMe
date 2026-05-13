#pragma once

#include "shared/qt/quuidex.h"
#include "qgraphics2/shape.h"

#include <QGraphicsScene>
#include <QUndoCommand>

// Тип фигур
enum class ShapeKind {Rectangle, Circle, Polyline, Line, Point};

struct ShapeBackup2
{
    ShapeKind kind = {ShapeKind::Rectangle};
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

class UndoAction : public QUndoCommand
{
public:
    enum class Type {Restore, Delete, Move, Edit};
    // Callback нужен, чтобы MainWindow мог обновить ui
    using ApplyCallback = std::function<void(qgraph::Shape*)>;

    UndoAction(QGraphicsScene* scene, Type type, const QString& text,
               const ShapeBackup2& before, const ShapeBackup2& after,
               ApplyCallback onApplied);

    // Применяет снимок к уже существующей фигуре
    bool applyBackup(const ShapeBackup2& backup);
    // Восстанавливает состояние фигуры
    void restoreFromBackup(qgraph::Shape* item, const ShapeBackup2& backup);
    // Ищет фигуру на сцене по qgraph::Shape::id()
    qgraph::Shape* findItem(const QUuidEx id) const;


protected:
    QGraphicsScene* _scene = {nullptr};
    Type _type = {Type::Restore};
    QString _text;
    ShapeBackup2 _before;
    ShapeBackup2 _after;

    ApplyCallback _onApplied;
};

class MoveShapeAction : public UndoAction
{
public:
    MoveShapeAction(QGraphicsScene* scene, const QString& text,
                    const ShapeBackup2& before, const ShapeBackup2& after,
                    ApplyCallback onApplied = {});

    void redo() override;
    void undo() override;
};

class UndoRestore : public QUndoCommand
{

};
