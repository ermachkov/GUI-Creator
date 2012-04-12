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

	void onDirectoryLoaded(const QString &);
	void onFileRenamed(const QString &path, const QString &oldName, const QString &newName);
	void onRootPathChanged(const QString &newPath);

	void on_mFontListView_activated(const QModelIndex &index);

private:

	struct Images
	{
		QImage mNormal;	              // невыделенный текст
		QImage mHighlighted;          // выделенный текст с фокусом
		QImage mHighlightedInactive;  // выделенный текст без фокуса
	};

	class PreviewItemDelegate : public QItemDelegate
	{
	public:

		PreviewItemDelegate(QObject *parent, QFileSystemModel *fileModel);

		~PreviewItemDelegate();

		virtual void drawDisplay(QPainter *painter,
			const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;

		// возврат размеров элемента
		virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

		// пересоздание фреймбуфера
		void recreateFrameBuffer(int width, int height) const;

		// создание
		void createImage(const QStyleOptionViewItem &option, const QString &text) const;

		// ф-ция возврата нужного image
		QImage getImage(const QStyleOptionViewItem &option, const QString &text) const;

		// очистка сгенерированных предпросмотров шрифтов
		void clearAllImages();

	private:

		QFileSystemModel               *mFileModel;   // указатель на модель файловой системы
		mutable QGLFramebufferObject   *mFrameBuffer; // фреймбуфер для отрисовки иконок предпросмотра шрифтов
		mutable QMap<QString, Images>  mImages;       // иконки предпросмотра
	};

	// возврат поддиректории для шрифтов
	QString getFontPath() const;

	// возврат коренной директории
	QString getRootPath() const;

	QFileSystemModel       *mFileModel;           // модель файловой системы для шрифтов
	PreviewItemDelegate    *mPreviewItemDelegate; // делегат
};

#endif // FONT_BROWSER_H
