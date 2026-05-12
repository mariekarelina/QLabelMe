#include "lambda_command.h"

LambdaCommand::LambdaCommand(Func redoFunc, Func undoFunc, const QString& text)
    : _redoFunc(std::move(redoFunc))
    , _undoFunc(std::move(undoFunc))
    , _text(text)
{
    setText(_text);
}

void LambdaCommand::redo()
{
    if (_redoFunc)
        _redoFunc();

    _applied = true;
}

void LambdaCommand::undo()
{
    if (_undoFunc)
        _undoFunc();

   _applied = false;
}
