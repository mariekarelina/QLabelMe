#include "lambda_command.h"

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
