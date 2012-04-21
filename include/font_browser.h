#ifndef FONT_BROWSER_H
#define FONT_BROWSER_H

#include "ui_font_browser.h"

class FontBrowser : public QDockWidget, private Ui::FontBrowser
{
	Q_OBJECT

public:

	struct Images
	{
		QImage mNormal;	              // невыделенный текст
		QImage mHighlighted;          // выделенный текст с фокусом
		QImage mHighlightedInactive;  // выделенный текст без фокуса
	};

	// Конструктор
	explicit FontBrowser(QWidget *parent = 0);

	// Деструктор
	virtual ~FontBrowser();

	// Возвращает указатель на виджет со списком шрифтов
	QWidget *getFontWidget() const;

	// определение загруженности файла
	bool isImageLoaded(const QModelIndex &index) const;

private slots:

	void onDirectoryLoaded(const QString &);
	void onFileRenamed(const QString &path, const QString &oldName, const QString &newName);
	void onRootPathChanged(const QString &newPath);

	void on_mFontListView_activated(const QModelIndex &index);

private:

	class FontFileSystemModel : public QFileSystemModel
	{
	public:

		explicit FontFileSystemModel(FontBrowser *parent);

		Qt::ItemFlags flags(const QModelIndex &index) const;

	private:

		FontBrowser *mFontBrowser;
	};

	class PreviewItemDelegate : public QItemDelegate
	{
	public:

		PreviewItemDelegate(QObject *parent, FontFileSystemModel *fileModel);

		~PreviewItemDelegate();

		virtual void drawDisplay(QPainter *painter,
			const QStyleOptionViewItem &option, const QRect &rect, const QString &text) const;

		// возврат размеров элемента
		virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

		// пересоздание фреймбуфера
		void recreateFrameBuffer(int width, int height) const;

		// создание предпросмотра шрифта
		void createImage(const QStyleOptionViewItem &option, const QString &text) const;

		// ф-ция возврата нужного image предпросмотра шрифта
		QImage getImage(const QStyleOptionViewItem &option, const QString &text) const;

		// очистка сгенерированных предпросмотров шрифтов
		void clearAllImages();

		const QMap<QString, Images> &getImages() const;

	private:

		FontFileSystemModel            *mFileModel;   // указатель на модель файловой системы
		mutable QGLFramebufferObject   *mFrameBuffer; // фреймбуфер для отрисовки иконок предпросмотра шрифтов
		mutable QMap<QString, Images>  mImages;       // иконки предпросмотра
	};

	// возврат поддиректории для шрифтов
	QString getFontPath() const;

	// возврат коренной директории
	QString getRootPath() const;

	FontFileSystemModel    *mFileModel;           // модель файловой системы для шрифтов
	PreviewItemDelegate    *mPreviewItemDelegate; // делегат
};

#endif // FONT_BROWSER_H
