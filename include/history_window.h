#ifndef HISTORY_WINDOW_H
#define HISTORY_WINDOW_H

#include "ui_history_window.h"

// Класс окна истории правок
class HistoryWindow : public QDockWidget, private Ui::HistoryWindow
{
	Q_OBJECT

public:

	// Конструктор
	HistoryWindow(QWidget *parent = NULL);

	// Устанавливает текущий стек отмен
	void setUndoStack(QUndoStack *undoStack);
};

#endif // HISTORY_WINDOW_H
