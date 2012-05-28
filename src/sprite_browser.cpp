#include "pch.h"
#include "sprite_browser.h"
#include "project.h"
#include "thumbnail_loader.h"

SpriteBrowser::SpriteBrowser(QWidget *parent)
: QDockWidget(parent), mThumbnailCount(0)
{
	setupUi(this);

	// запрещаем перетаскивание внутри виджета спрайтов
	mListWidget->setAcceptDrops(false);

	// создаем фоновый поток
	mBackgroundThread = new QThread(this);

	// создаем загрузчик миниатюр и переносим его в фоновый поток
	mThumbnailLoader = new ThumbnailLoader();
	connect(this, SIGNAL(thumbnailQueued(QString, int)), mThumbnailLoader, SLOT(onThumbnailQueued(QString, int)));
	connect(mThumbnailLoader, SIGNAL(thumbnailLoaded(QString, QImage)), this, SLOT(onThumbnailLoaded(QString, QImage)));
	mThumbnailLoader->moveToThread(mBackgroundThread);

	// создание объекта для слежения за изменениями файлов внутри текущей директории
	mWatcher = NULL;
	recreateWatcher();

	// Создает и инициализирует все виджеты на плавающем окне
	// TODO: добавить проверку на существование директории
	// "Cannot find: " + rootPath

	// обнуление пути относительно корневой директории
	mRelativePath = "";

	// загрузка иконок для списка спрайтов
	mIconFolderUp = QIcon(":/images/folder_up.png");
	mIconFolder = QIcon(":/images/folder.png");
	mIconSpriteLoading = QIcon(":/images/sprite_loading.png");
	mIconSpriteError = QIcon(":/images/sprite_error.png");

	// считывание каталога спрайтов и заполнение списка спрайтов
	update("/", mRelativePath);

	// запускаем фоновый поток
	mBackgroundThread->start();

	// запускаем таймер для отслеживания изменений файлов
	startTimer(250);
}

SpriteBrowser::~SpriteBrowser()
{
	// дожидаемся завершения фонового потока
	mBackgroundThread->quit();
	mBackgroundThread->wait();

	// удаляем загрузчик миниатюр
	delete mThumbnailLoader;
}

QWidget *SpriteBrowser::getSpriteWidget() const
{
	return mListWidget;
}

void SpriteBrowser::timerEvent(QTimerEvent *event)
{
	for (ThumbnailCache::iterator it = mThumbnailCache.begin(); it != mThumbnailCache.end(); ++it)
	{
		if (it.value().isChanged() && it.value().isTimerFinished())
		{
			// отправка на подгрузку иконки в фоне
			emit thumbnailQueued(it.key(), mThumbnailCount++);

			it.value().setChanged(false);
		}
	}
}

void SpriteBrowser::on_mListWidget_itemActivated(QListWidgetItem *item)
{
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

void SpriteBrowser::onThumbnailLoaded(QString absoluteFileName, QImage image)
{
	// иконка должна быть в буфере
	ThumbnailCache::iterator iterTC = mThumbnailCache.find(absoluteFileName);
	Q_ASSERT(iterTC != mThumbnailCache.end());

	// проверяем, загружена ли миниатюра или нет
	if (!image.isNull())
	{
		// преобразование QImage в QIcon
		iterTC.value().setIcon(QIcon(QPixmap::fromImage(image)), true);
	}
	else
	{
		// установка иконки ошибки
		iterTC.value().setIcon(mIconSpriteError, false);
	}

	// установка иконки в кеш
	QFileInfo fileInfo(absoluteFileName);
	iterTC.value().setFileDate(fileInfo.lastModified());
	iterTC.value().setFileSize(fileInfo.size());

	// выделение имени файла
	QString fileName = fileInfo.fileName();

	// выделение имени директории
	QString absolutePath = fileInfo.absolutePath();

	// сравнение текущей директории с путем загруженым
	if (absolutePath + '/' == getRootPath() + mRelativePath)
	{
		QList<QListWidgetItem *> items = mListWidget->findItems(fileName, Qt::MatchCaseSensitive);
		if (items.size() == 1)
		{
			// установка только что загруженой иконки
			items.front()->setIcon(iterTC.value().getIcon());

			// разрешение перетаскивания если загрузка успешна
			if (!image.isNull())
				items.front()->setFlags(items.front()->flags() | Qt::ItemIsDragEnabled);
		}
	}
}

void SpriteBrowser::onDirectoryChanged(const QString &path)
{
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

void SpriteBrowser::onFileChanged(const QString &absoluteFileName)
{
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

QString SpriteBrowser::getRootPath() const
{
	return Project::getSingleton().getRootDirectory() + Project::getSingleton().getSpritesDirectory();
}

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

void SpriteBrowser::update(QString oldPath, QString newPath)
{
	// всплывающая подсказка с полным путем
	mListWidget->setToolTip(getRootPath() + mRelativePath);

	// очистка поля ГУИ иконок
	mListWidget->clear();

	// (убрано из-за косяка с mWatcher) снятие слежения за текущей директорией
	//mWatcher->removePaths(mWatcher->directories());
	// (убрано из-за косяка с mWatcher) снятие слежения за файлами
	//mWatcher->removePaths(mWatcher->files());

	// установка слежения за текущей директорией если она существует
	if (QFile::exists(getRootPath() + mRelativePath))
		mWatcher->addPath(getRootPath() + mRelativePath);
	else
		qWarning() << "Warning: Directory not added to Watcher" << getRootPath() + mRelativePath;

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
			// FIXME:
			QListWidgetItem *item = new QListWidgetItem(mIconFolder, *iterEntries, mListWidget);
			//QListWidgetItem *item = new QListWidgetItem(style()->standardIcon(QStyle::SP_DirIcon), *iterEntries, mListWidget);

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
		// путь к каталогу изменился

		// у всех в кеше снять измененность
		foreach (IconWithInfo element, mThumbnailCache)
			element.setChanged(false);

		// Очистка списка файлов на загрузку второго потока
		mThumbnailLoader->setThumbnailCount(mThumbnailCount);
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
					emit thumbnailQueued(absoluteFileName, mThumbnailCount++);

					// установка иконки "загружается"
					iterTC->setIcon(mIconSpriteLoading, false);
				}
			}
			else
			{
				// иконки нет в кеше

				// отправка на немедленную подгрузку иконки в фоне
				emit thumbnailQueued(absoluteFileName, mThumbnailCount++);

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

		// запрет перетаскивания по умолчанию пока иконка не загружена в кэш
		if (iterTC.value().isIconLoaded())
			item->setFlags(item->flags() | Qt::ItemIsDragEnabled);
		else
			item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);

		// сохраняем полный путь к файлу для поддержки перетаскивания
		item->setData(Qt::UserRole, getRootPath() + mRelativePath + fileName);

		// всплывающая подсказка с несокращенным именем
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
