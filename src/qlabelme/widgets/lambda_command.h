#pragma once
#include <QUndoCommand>
#include <QDebug>
#include <functional>
#include <optional>

class LambdaCommand : public QUndoCommand
{
public:
    // Fn -> Func
    using Func = std::function<void()>;
    using MergeFn = std::function<bool(const QUndoCommand* other)>;

    LambdaCommand(Func redoFn, Func undoFn, const QString& text = {})
        : _redoFn(std::move(redoFn))
        , _undoFn(std::move(undoFn))
        , _text(text)
    {
        setText(_text);
        //qDebug() << "[CMD] create:" << text;
    }

    void redo() override;
    void undo() override;

private:
    Func _redoFn;
    Func _undoFn;
    QString _text;

    bool _applied = false;
};
