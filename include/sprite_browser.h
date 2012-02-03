#ifndef SPRITE_BROWSER_H
#define SPRITE_BROWSER_H

#include "ui_sprite_browser.h"

class ThumbnailLoader;

class SpriteBrowser : public QDockWidget, private Ui::SpriteBrowser
{
	Q_OBJECT

public:

	SpriteBrowser(QWidget *parent = NULL);

private slots:

	// Обработчик двойного щелчка или Enter по элементу
	void on_mListWidget_itemActivated(QListWidgetItem *item);

	// слот о загруженности иконки - обновление иконки
	void onThumbnailLoaded(QString absoluteFileName, QImage image, QFileInfo fileInfo);

	// слот об ошибке при загрузке иконки
	void onThumbnailNotLoaded(QString absoluteFileName);

	// слот об изменении списка файлов внутри текущей директории
	void onDirectoryChanged(const QString &path);

	// слот об изменении файла внутри текущей директории
	void onFileChanged(const QString &absoluteFileName);

private:

	// Создает и инициализирует все виджеты на плавающем окне
	void createWidgets();

	// Пересоздание объекта слежения за каталогом
	void recreateWatcher();

	// Обновление отображения текущего каталога
	void update();

	// возврат из опций текущей коренной директории
	QString getRootPath() const;

private:

	struct IconWithInfo
	{
		IconWithInfo(const QIcon &icon, qint64 fileSize, const QDateTime &fileDate)
		: mIcon(icon), mFileSize(fileSize), mFileDate(fileDate)
		{
		}

		QIcon mIcon;
		qint64 mFileSize;
		QDateTime mFileDate;
	};

	// путь относительно корневой директории
	QString mRelativePath;

	// слежение за текущей директорией
	QFileSystemWatcher *mWatcher;

	// иконки для отображения директорий и файлов
	QIcon mIconFolderUp;
	QIcon mIconFolder;
	QIcon mIconSpriteLoading;
	QIcon mIconSpriteError;

	ThumbnailLoader *mThumbnailLoader;           // нить для подгрузки спрайтов в фоне
	QMap<QString, IconWithInfo> mThumbnailCache; // кеш для хранения загруженых иконок
};

#endif // SPRITE_BROWSER_H
