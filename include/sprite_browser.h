#ifndef SPRITE_BROWSER_H
#define SPRITE_BROWSER_H

#include "ui_sprite_browser.h"

class ThumbnailLoader;

class SpriteBrowser : public QDockWidget, private Ui::SpriteBrowser
{
	Q_OBJECT

public:

	// Конструктор
	SpriteBrowser(QWidget *parent = NULL);

	// Деструктор
	virtual ~SpriteBrowser();

signals:

	// Сигнал добавления спрайта в очередь на загрузку в фоновом потоке
	void thumbnailQueued(QString absoluteFileName, int count);

protected:

	// Вызывается по срабатыванию таймера
	virtual void timerEvent(QTimerEvent *event);

private slots:

	// Обработчик двойного щелчка или Enter по элементу
	void on_mListWidget_itemActivated(QListWidgetItem *item);

	// слот о загруженности иконки - обновление иконки
	void onThumbnailLoaded(QString absoluteFileName, QImage image);

	// слот об изменении списка файлов внутри текущей директории
	void onDirectoryChanged(const QString &path);

	// слот об изменении файла внутри текущей директории
	void onFileChanged(const QString &absoluteFileName);

private:

	// Создает и инициализирует все виджеты на плавающем окне
	void createWidgets();

	// возврат из опций текущей коренной директории
	QString getRootPath() const;

	// Пересоздание объекта слежения за каталогом
	void recreateWatcher();

	// Обновление отображения текущего каталога
	void update(QString oldPath, QString newPath);

private:

	class IconWithInfo
	{
	public:
		//IconWithInfo(const QIcon &icon, qint64 fileSize, const QDateTime &fileDate)
		//: mIcon(icon), mFileSize(fileSize), mFileDate(fileDate), mChanged(false)
		IconWithInfo(const QIcon &icon, bool isIconLoaded)
		: mIcon(icon), mIconLoaded(isIconLoaded), mFileSize(-1), mChanged(false)
		{
		}

		// проверка что иконка загружена
		bool isIconLoaded() const;

		// установка иконки
		void setIcon(const QIcon &icon, bool isIconLoaded);

		QIcon getIcon() const;

		void setFileSize(qint64 fileSize);

		qint64 getFileSize() const;

		void setFileDate(const QDateTime &fileDate);

		QDateTime getFileDate() const;

		// установка отложеной загрузки
		// ...

		// установка немедленной загрузки
		// ...

		// установка флага изменения иконки
		void setChanged(bool isChanged);

		bool isChanged() const;

		bool isTimerFinished() const;

	private:

		QIcon             mIcon;       // Готовая иконка
		bool              mIconLoaded; // Флаг о загруженности иконки
		qint64            mFileSize;   // Размер файла
		QDateTime         mFileDate;   // Дата изменения файла
		bool              mChanged;    // Флаг изменения иконки
		QElapsedTimer     mTimer;      // Таймер для отсчета времени с момента последнего изменения файла
	};

	// Тип для кэша иконок
	typedef QMap<QString, IconWithInfo> ThumbnailCache;

	QString             mRelativePath;          // Путь относительно корневой директории
	QFileSystemWatcher  *mWatcher;              // Объект слежения за текущей директорией
	QThread             *mBackgroundThread;     // Фоновый поток
	ThumbnailLoader     *mThumbnailLoader;      // Загрузчик миниатюр
	int                 mThumbnailCount;        // Счетчик загруженных миниатюр
	ThumbnailCache      mThumbnailCache;        // Кэш для хранения загруженных иконок

	// Иконки для отображения директорий и файлов
	QIcon mIconFolderUp;
	QIcon mIconFolder;
	QIcon mIconSpriteLoading;
	QIcon mIconSpriteError;
};

#endif // SPRITE_BROWSER_H
