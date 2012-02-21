#ifndef FONT_BROWSER_H
#define FONT_BROWSER_H

#include "ui_font_browser.h"

class FontBrowser : public QDockWidget, private Ui::FontBrowser
{
	Q_OBJECT
	
public:

	// Конструктор
	explicit FontBrowser(QWidget *parent = 0);

	// Деструктор
	virtual ~FontBrowser();

private:

	// возврат из опций текущей коренной директории
	QString getRootPath() const;

	// пересозжание фреймбуфера
	void recreateFrameBuffer(int width, int height);

	// загрузка и отображение списка доступных шрифтов
	void scanFonts();

private:

	// иконка в браузере текста
	QIcon mIconDrawText;

	// FIXME: удалить
	// временный список шрифтов для генерации предпросмотра
//	QVector< QSharedPointer<FTFont> > fontVector;

	QGLFramebufferObject *mFrameBuffer;
};

#endif // FONT_BROWSER_H
