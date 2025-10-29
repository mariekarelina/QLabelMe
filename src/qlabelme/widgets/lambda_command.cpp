#include "lambda_command.h"

bool LambdaCommand::mergeWith(const QUndoCommand* other)
{
    if (!_merge)
        return false;
    return _merge(other);
}

void LambdaCommand::redo()
{
    if (_redoFn)
        _redoFn();
}

void LambdaCommand::undo()
{
    if (_undoFn)
        _undoFn();
}
