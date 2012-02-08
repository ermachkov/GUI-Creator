#include "pch.h"
#include "sprite_browser.h"
#include "options.h"
#include "thumbnail_loader.h"
#include "utils.h"

SpriteBrowser::SpriteBrowser(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	// запрещаем перетаскивание внутри виджета спрайтов
	mListWidget->setAcceptDrops(false);

	mThumbnailLoader = new ThumbnailLoader(this);

	qRegisterMetaType<QFileInfo>("QFileInfo");

	// связывание сигнала фоновой нити со слотом основной нити
	connect(mThumbnailLoader, SIGNAL(thumbnailLoaded(QString, QImage)),
		this, SLOT(onThumbnailLoaded(QString, QImage)));
	connect(mThumbnailLoader, SIGNAL(thumbnailNotLoaded(QString)),
		this, SLOT(onThumbnailNotLoaded(QString)));

	// создание объекта для слежения за изменениями файлов внутри текущей директории
	mWatcher = NULL;
	recreateWatcher();

	createWidgets();

	// запускаем таймер для отслеживания изменений файлов
	startTimer(250);
}

void SpriteBrowser::timerEvent(QTimerEvent *event)
{
	for (ThumbnailCache::iterator it = mThumbnailCache.begin(); it != mThumbnailCache.end(); ++it)
	{
		if (it.value().isChanged() && it.value().isTimerFinished())
		{
			qDebug() << "addFileForLoading:" << it.key();

			// отправка на немедленную подгрузку иконки в фоне
			mThumbnailLoader->addFileForLoading(it.key());

			it.value().setChanged(false);
		}

	}
}

void SpriteBrowser::on_mListWidget_itemActivated(QListWidgetItem *item)
{
	// qDebug() << "on_mListWidget_itemActivated" << item->text();

	// определение - файл или каталог
	QFileInfo info(getRootPath() + mRelativePath + item->text());
	if (info.isFile())
		return;

	// сохранение старого пути для передачи в update();
	QString oldPath = mRelativePath;

	// формирование текущего пути
	if (item->text() == "..")
	{
		// удаление последнего слеша у относительного пути
		mRelativePath.resize(mRelativePath.size() - 1);

		// поиск последнего слеша у относительного пути
		int i = mRelativePath.lastIndexOf('/');

		if (i == -1)
		{
			// если слеша нет - то удалить строку целиком
			mRelativePath.clear();
		}
		else
		{
			// удаление содержимого строки с последнего слеша не включительно
			mRelativePath.resize(i + 1);
		}
	}
	else
	{
		// дополнение относительного пути
		mRelativePath += item->text() + "/";
	}


	// пересоздание объекта слежения
	recreateWatcher();

	// обновление иконок
	update(oldPath, mRelativePath);
}

// слот о загруженности иконки - обновление иконки
void SpriteBrowser::onThumbnailLoaded(QString absoluteFileName, QImage image)
{
	qDebug() << "loaded:" << absoluteFileName;

	QFileInfo fileInfo(absoluteFileName);

	// выделение имени файла
	QString fileName = fileInfo.fileName();
	// выделение имени директории
	QString absolutePath = fileInfo.absolutePath();

	// сравнение текущей директории с путем загруженым
	if (absolutePath + '/' == getRootPath() + mRelativePath)
	{
		// преобразование QImage в QIcon
		QIcon icon = QIcon(QPixmap::fromImage(image));

		ThumbnailCache::iterator iterTC = mThumbnailCache.find(absoluteFileName);

		// иконка должна быть в буфере
		Q_ASSERT(iterTC != mThumbnailCache.end());

		// установка иконки в кеш
		iterTC.value().setIcon(icon, true);
		iterTC.value().setChanged(false);
		iterTC.value().setFileDate(fileInfo.lastModified());
		iterTC.value().setFileSize(fileInfo.size());

		QList<QListWidgetItem *> items = mListWidget->findItems(fileName, Qt::MatchCaseSensitive);
		if (items.size() == 1)
		{
			// установка только что загруженой иконки
			items.front()->setIcon(iterTC.value().getIcon());
		}
	}
}

void SpriteBrowser::onThumbnailNotLoaded(QString absoluteFileName)
{
	qDebug() << "not loaded:" << absoluteFileName;

	QFileInfo fileInfo(absoluteFileName);

	// выделение имени файла
	QString fileName = fileInfo.fileName();
	// выделение имени директории
	QString absolutePath = fileInfo.absolutePath();

	// сравнение текущей директории с путем загруженым
	if (absolutePath + '/' == getRootPath() + mRelativePath)
	{
		ThumbnailCache::iterator iterTC = mThumbnailCache.find(absoluteFileName);

		// иконка должна быть в буфере
		Q_ASSERT(iterTC != mThumbnailCache.end());

		// установка иконки в кеш
		iterTC.value().setIcon(mIconSpriteError, false);
		iterTC.value().setChanged(false);

		QList<QListWidgetItem *> items = mListWidget->findItems(fileName, Qt::MatchCaseSensitive);
		if (items.size() == 1)
		{
			// установка только что загруженой иконки
			items.front()->setIcon(iterTC.value().getIcon());
		}
	}
}

void SpriteBrowser::onDirectoryChanged(const QString &path)
{
	qDebug() << "onDirectoryChanged: " << path;

	// если измененный путь совпадает с текущим путем
	if (path == getRootPath() + mRelativePath)
	{
		// сохранение старого пути для передачи в update();
		QString oldPath = mRelativePath;

		// проверка на не существование текущей директории
		while (!QDir(getRootPath() + mRelativePath).exists())
		{
			// удаление последнего слеша
			if (!mRelativePath.isEmpty())
				mRelativePath.resize(mRelativePath.size() - 1);
			// поиск предпоследнего слеша
			int i = mRelativePath.lastIndexOf('/');
			if (i == -1)
			{
				mRelativePath.clear();
				break;
			}
			// обрезка строки до предпоследнего слеша
			mRelativePath.resize(i + 1);
		}

		// пересоздание объекта слежения
		recreateWatcher();

		// обновить текущую директорию
		update(oldPath, mRelativePath);
	}
}

// слот об изменении файла внутри текущей директории
void SpriteBrowser::onFileChanged(const QString &absoluteFileName)
{
	qDebug() << "onFileChanged: " << absoluteFileName;

	// фильтр т.к. удаление/добавление/переименование обрабатывается в onDirectoryChanged
	if (!QFile::exists(absoluteFileName))
	{
		// файл удален - прекратить обработку
		return;
	}

	// формирование имени файла
	QString tempPath(absoluteFileName);
	// поиск последнего слеша пути
	int i = tempPath.lastIndexOf('/');

	Q_ASSERT(i != -1);

	// удаление содержимого строки с последнего слеша не включительно
	tempPath.resize(i + 1);

	if (tempPath == getRootPath() + mRelativePath)
	{
		ThumbnailCache::iterator iterTC = mThumbnailCache.find(absoluteFileName);

		// иконка должна быть в буфере
		Q_ASSERT(iterTC != mThumbnailCache.end());

		// старт отложенной загрузки
		iterTC.value().setIcon(mIconSpriteLoading, false);
		iterTC.value().setChanged(true);
	}
}

// Создает и инициализирует все виджеты на плавающем окне
void SpriteBrowser::createWidgets()
{
	// TODO: добавить проверку на существование директории
	// "Cannot find: " + rootPath

	// обнуление пути относительно корневой директории
	mRelativePath = "";

	mIconFolderUp = QIcon(":/images/folder_up.png");
	mIconFolder = QIcon(":/images/folder.png");
	mIconSpriteLoading = QIcon(":/images/sprite_loading.png");
	mIconSpriteError = QIcon(":/images/sprite_error.png");

	update("/", mRelativePath);
}

QString SpriteBrowser::getRootPath() const
{
	return Options::getSingleton().getDataDirectory() + "sprites/";
}

// Пересоздание объекта слежения за каталогом
void SpriteBrowser::recreateWatcher()
{
	// удаление объекта слежения
	delete mWatcher;
	mWatcher = NULL;

	// создание объекта для слежения за изменениями файлов внутри текущей директории
	mWatcher = new QFileSystemWatcher(this);
	// связывание сигнала со слотом, как только произойдут измененения
	connect(mWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(onDirectoryChanged(const QString &)));
	connect(mWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(onFileChanged(const QString &)));
}

// Обновление отображения текущего каталога
void SpriteBrowser::update(QString oldPath, QString newPath)
{
	qDebug() << "oldPath:" << oldPath << "newPath" << newPath;
	qDebug() << "getRootPath:" << getRootPath();
	qDebug() << "mRelativePath:" << mRelativePath;
	qDebug() << "--";

	// всплывающая подсказка с полным путем
	setToolTip(getRootPath() + mRelativePath);

	// отчистка поля ГУИ иконок
	mListWidget->clear();

	// (убрано из-за косяка с mWatcher) снятие слежения за текущей директорией
	//mWatcher->removePaths(mWatcher->directories());
	// (убрано из-за косяка с mWatcher) снятие слежения за файлами
	//mWatcher->removePaths(mWatcher->files());

	// установка слежения за текущей директорией если она существует
	if (QFile::exists(getRootPath() + mRelativePath))
		mWatcher->addPath(getRootPath() + mRelativePath);

	QDir currentDir = QDir(getRootPath() + mRelativePath);
	// получение только директорий
	currentDir.setFilter(QDir::NoDot | QDir::Dirs);
	currentDir.setNameFilters(QStringList());
	QStringList listEntries = currentDir.entryList();

	// создание папки ".."
	if (!mRelativePath.isEmpty())
	{
		QListWidgetItem *item = new QListWidgetItem(mIconFolderUp, "..", mListWidget);

		// запрещаем перетаскивание папок
		item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);
	}

	// проход по директориям
	for (QStringList::const_iterator iterEntries = listEntries.begin(); iterEntries != listEntries.end(); ++iterEntries)
	{
		if (*iterEntries != "..")
		{
			QListWidgetItem *item = new QListWidgetItem(mIconFolder, *iterEntries, mListWidget);

			// запрещаем перетаскивание папок
			item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);

			// всплывающая подсказка с полным именем
			item->setToolTip(*iterEntries);
		}
	}

	// получение только файлов png и jpg
	currentDir.setFilter(QDir::Files);
	QStringList fileNameFilters;
	fileNameFilters << "*.png" << "*.jpg";
	currentDir.setNameFilters(fileNameFilters);
	listEntries = currentDir.entryList();

	if (oldPath != newPath)
	{
		// каталог изменился
		qDebug() << "Dir was changed";

		// у всех в кеше снять измененность
		foreach (IconWithInfo element, mThumbnailCache)
			element.setChanged(false);

		// Отчистка списка файлов на загрузку второго потока
		mThumbnailLoader->clearQueueLoadingFiles();
	}
	else
	{
		// каталог не изменился
		qDebug() << "Dir was't changed";
	}

	foreach (const QString &fileName, listEntries)
	{
		// формирование полного пути к иконке
		QString absoluteFileName = getRootPath() + mRelativePath + fileName;

		// получение информации о текущем файле
		QFileInfo fileInfo(absoluteFileName);

		ThumbnailCache::iterator iterTC = mThumbnailCache.find(absoluteFileName);

		if (oldPath != newPath)
		{
			// каталог изменился

			if (iterTC != mThumbnailCache.end())
			{
				// иконка есть в кеше

				if (
					// иконка не загружена
					!iterTC.value().isIconLoaded()
					// размера файла не совпадает
					|| iterTC.value().getFileSize() != fileInfo.size()
					// дата создания не совпадает
					|| iterTC.value().getFileDate() != fileInfo.lastModified())
				{
					// отправка на немедленную подгрузку иконки в фоне
					mThumbnailLoader->addFileForLoading(absoluteFileName);

					// установка иконки "загружается"
					iterTC->setIcon(mIconSpriteLoading, false);
				}
			}
			else
			{
				// иконки нет в кеше

				// отправка на немедленную подгрузку иконки в фоне
				mThumbnailLoader->addFileForLoading(absoluteFileName);

				// добавление в кеше
				iterTC = mThumbnailCache.insert(absoluteFileName, IconWithInfo(mIconSpriteLoading, false));

			}
		}
		else
		{
			// каталог не изменился

			if (iterTC != mThumbnailCache.end())
			{
				// иконка есть в кеше

				if (
					// иконка не загружена
					!iterTC.value().isIconLoaded()
					// размера файла не совпадает
					|| iterTC.value().getFileSize() != fileInfo.size()
					// дата создания не совпадает
					|| iterTC.value().getFileDate() != fileInfo.lastModified())
				{

					if (!iterTC.value().isChanged())
					{
						// старт отложенной загрузки
						iterTC.value().setIcon(mIconSpriteLoading, false);
						iterTC.value().setChanged(true);
					}
				}
			}
			else
			{
				// иконки нет в кеше

				// добавление в кеш с отложенной загрузкой
				iterTC = mThumbnailCache.insert(absoluteFileName, IconWithInfo(mIconSpriteLoading, true));
				iterTC.value().setChanged(true);
			}
		}

		// создание иконки в ГУИ
		QListWidgetItem *item = new QListWidgetItem(fileName, mListWidget);
		item->setIcon(iterTC->getIcon());

		// сохраняем относительный путь к файлу в UserRole для поддержки перетаскивания
		item->setData(Qt::UserRole, "sprite://sprites/" + mRelativePath + fileName);

		// всплывающая подсказка с полным именем
		item->setToolTip(fileName);

		// устанавливаем слежку на файлом
		if (QFile::exists(absoluteFileName))
		{
			mWatcher->addPath(absoluteFileName);
		}
	}
}

bool SpriteBrowser::IconWithInfo::isIconLoaded() const
{
	Q_ASSERT(!mIcon.isNull());
	return mIconLoaded;
}

void SpriteBrowser::IconWithInfo::setIcon(const QIcon &icon, bool isIconLoaded)
{
	mIcon = icon;
	mIconLoaded = isIconLoaded;
}

QIcon SpriteBrowser::IconWithInfo::getIcon() const
{
	return mIcon;
}

void SpriteBrowser::IconWithInfo::setFileSize(qint64 fileSize)
{
	mFileSize = fileSize;
}

qint64 SpriteBrowser::IconWithInfo::getFileSize() const
{
	return mFileSize;
}

void SpriteBrowser::IconWithInfo::setFileDate(const QDateTime &fileDate)
{
	mFileDate = fileDate;
}

QDateTime SpriteBrowser::IconWithInfo::getFileDate() const
{
	return mFileDate;
}

void SpriteBrowser::IconWithInfo::setChanged(bool isChanged)
{
	mChanged = isChanged;

	if (mChanged)
		mTimer.start();
}

bool SpriteBrowser::IconWithInfo::isChanged() const
{
	return mChanged;
}

bool SpriteBrowser::IconWithInfo::isTimerFinished() const
{
	return mTimer.hasExpired(250);
}
