#ifndef OPTIONS_DIALOG_H
#define OPTIONS_DIALOG_H

#include "ui_options_dialog.h"

// Диалоговое окно настроек приложения
class OptionsDialog : public QDialog, private Ui::OptionsDialog
{
	Q_OBJECT

public:

	// Конструктор
	OptionsDialog(QWidget *parent = NULL);

public slots:

	// Вызывается при принятии диалога
	virtual void accept();

private slots:

	// Обработчик нажатия одной из кнопок в группе кнопок Ok/Cancel/Apply
	void on_mButtonBox_clicked(QAbstractButton *button);

private:

	// Применяет выбранные настройки
	void applySettings();
};

#endif // OPTIONS_DIALOG_H
