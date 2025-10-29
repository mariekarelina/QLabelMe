#pragma once
#include <QUndoCommand>
#include <functional>
#include <optional>

class LambdaCommand : public QUndoCommand
{
public:
    using Fn = std::function<void()>;
    using MergeFn = std::function<bool(const QUndoCommand* other)>;

    LambdaCommand(Fn redoFn, Fn undoFn, const QString& text = {}, int id = 0, MergeFn merge = {})
        : _redoFn(std::move(redoFn))
        , _undoFn(std::move(undoFn))
        , _text(text)
        , _id(id)
        , _merge(std::move(merge))
    {
        setText(_text);
    }

    int id() const override { return _id; }

    bool mergeWith(const QUndoCommand* other) override;
    void redo() override;
    void undo() override;

private:
    Fn _redoFn;
    Fn _undoFn;
    QString _text;
    int _id = 0;              // Одинаковые id → кандидаты на merge
    MergeFn _merge;           // Логика склейки перетаскиваний
};
