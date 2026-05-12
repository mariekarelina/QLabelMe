#pragma once
#include <QUndoCommand>
#include <QDebug>
#include <functional>
#include <optional>

class LambdaCommand : public QUndoCommand
{
public:
    using Func = std::function<void()>;

    LambdaCommand(Func redoFunc, Func undoFunc, const QString& text = {});

    void redo() override;
    void undo() override;

private:
    Func _redoFunc;
    Func _undoFunc;
    QString _text;

    bool _applied = false;
};
