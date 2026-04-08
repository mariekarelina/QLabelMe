#pragma once
#include <QUndoCommand>
#include <QDebug>
#include <functional>
#include <optional>

class LambdaCommand : public QUndoCommand
{
public:
    using Func = std::function<void()>;
    using MergeFunc = std::function<bool(const QUndoCommand* other)>;

    LambdaCommand(Func redoFunc, Func undoFunc, const QString& text = {})
        : _redoFunc(std::move(redoFunc))
        , _undoFunc(std::move(undoFunc))
        , _text(text)
    {
        setText(_text);
    }

    void redo() override;
    void undo() override;

private:
    Func _redoFunc;
    Func _undoFunc;
    QString _text;

    bool _applied = false;
};
