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

	// Возвращает указатель на виджет со списком шрифтов
	QWidget *getFontWidget() const;

private slots:

	void onDirectoryLoaded(const QString &path);
	void onFileRenamed(const QString &path, const QString &oldName, const QString &newName);
	void onRootPathChanged(const QString &newPath);

	void on_mFontListView_activated(const QModelIndex &index);

private:

	class PreviewItemDelegate : public QItemDelegate
	{
	public:

		PreviewItemDelegate(QObject *parent = NULL);

		// произвольная отрисовка
		virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const;

		// возврат размеров элемента
		virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	};

	// возврат поддиректории для шрифтов
	QString getFontPath() const;

	// возврат коренной директории
	QString getRootPath() const;

	// пересоздание фреймбуфера
	void recreateFrameBuffer(int width, int height);

	// загрузка и отображение списка доступных шрифтов
	void scanFonts(const QString &path);

	QFileSystemModel       *mFileModel;     // модель файловой системы для шрифтов
	QGLFramebufferObject   *mFrameBuffer;   // фреймбуфер для отрисовки иконки предпросмотра шрифта
//	QIcon                  mIconDrawText;   // иконка в браузере текста
	QMap<QString, QImage>  mImages;         // иконки предпросмотра текста

};

#endif // FONT_BROWSER_H
