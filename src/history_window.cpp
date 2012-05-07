#include "pch.h"
#include "history_window.h"

HistoryWindow::HistoryWindow(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	// настраиваем виджет отмен
	mUndoView->setEmptyLabel("Начало работы");
	mUndoView->setCleanIcon(QIcon(":/images/file_save.png"));
}

void HistoryWindow::setUndoStack(QUndoStack *undoStack)
{
	mUndoView->setStack(undoStack);
	mUndoView->setEnabled(undoStack != NULL);
}
