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

	// пересоздание фреймбуфера
	void recreateFrameBuffer(int width, int height);

	// загрузка и отображение списка доступных шрифтов
	void scanFonts();

private:

	// Делегат для запрещения редактирования названия колонок
	class PreviewItemDelegate : public QItemDelegate
	//class PreviewItemDelegate : public QAbstractItemDelegate
	{
	public:

		PreviewItemDelegate(QObject *parent = NULL);

		// произвольная отрисовка
		virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

		// возврат размеров элемента
		virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	};

	QGLFramebufferObject  *mFrameBuffer;   // фреймбуфер для отрисовки иконки предпросмотра шрифта
	QIcon                 mIconDrawText;   // иконка в браузере текста
};

#endif // FONT_BROWSER_H
