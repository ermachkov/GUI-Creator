#ifndef FONT_BROWSER_H
#define FONT_BROWSER_H

#include "ui_font_browser.h"

class FontBrowser : public QDockWidget, private Ui::FontBrowser
{
	Q_OBJECT
	
public:

	// Конструктор
	explicit FontBrowser(QWidget *parent = 0);

private:

	// возврат из опций текущей коренной директории
	QString getRootPath() const;

	// загрузка и отображение списка доступных шрифтов
	void scanFonts();
};

#endif // FONT_BROWSER_H
