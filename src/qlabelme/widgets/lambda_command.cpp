#include "lambda_command.h"

void LambdaCommand::redo()
{
    //qDebug() << "[CMD] redo:" << text();

    if (_redoFn)
        _redoFn();

    _applied = true;
}

void LambdaCommand::undo()
{
    //qDebug() << "[CMD] undo:" << text();

    if (_undoFn)
        _undoFn();

   _applied = false;
}
