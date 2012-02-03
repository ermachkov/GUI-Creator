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
	connect(mThumbnailLoader, SIGNAL(thumbnailLoaded(QString, QImage, QFileInfo)),
		this, SLOT(onThumbnailLoaded(QString, QImage, QFileInfo)));
	connect(mThumbnailLoader, SIGNAL(thumbnailNotLoaded(QString)),
		this, SLOT(onThumbnailNotLoaded(QString)));

	// создание объекта для слежения за изменениями файлов внутри текущей директории
	mWatcher = NULL;
	recreateWatcher();

	createWidgets();
}

void SpriteBrowser::on_mListWidget_itemActivated(QListWidgetItem *item)
{
	// qDebug() << "on_mListWidget_itemActivated" << item->text();

	// определение - файл или каталог
	QFileInfo info(getRootPath() + mRelativePath + item->text());
	if (info.isFile())
		return;

	// обновление текущего пути
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
	update();
}

// слот о загруженности иконки - обновление иконки
void SpriteBrowser::onThumbnailLoaded(QString absoluteFileName, QImage image, QFileInfo fileInfo)
{
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
			// преобразование QImage в QIcon
			QIcon icon = QIcon(QPixmap::fromImage(image));

			// добавление иконки в буфер
			mThumbnailCache.insert(absoluteFileName, IconWithInfo(icon, fileInfo.size(), fileInfo.lastModified()));

			// установка только что загруженой иконки
			items.front()->setIcon(icon);
		}
	}
}

// слот об ошибке при загрузке иконки
void SpriteBrowser::onThumbnailNotLoaded(QString absoluteFileName)
{
	// сравнение текущей директории с путем загруженым
	QFileInfo fileInfo(absoluteFileName);
	// выделение имени файла
	QString fileName = fileInfo.fileName();

	// выделение имени директории
	QString absolutePath = fileInfo.absolutePath();

	// сравнение строк путей на равенство
	if (absolutePath + '/' == getRootPath() + mRelativePath)
	{
		QList<QListWidgetItem *> items = mListWidget->findItems(fileName, Qt::MatchCaseSensitive);
		if (items.size() == 1)
		{
			// установка иконки-ошибки
			items.front()->setIcon(mIconSpriteError);
		}
	}
}

// слот об изменении списка файлов внутри текущей директории
void SpriteBrowser::onDirectoryChanged(const QString &path)
{
	// qDebug() << "onDirectoryChanged: " << path;

	// если измененный путь совпадает с текущим путем
	if (path == getRootPath() + mRelativePath)
	{
		// Отчистка списка файлов на загрузку
		mThumbnailLoader->clearQueueLoadingFiles();

		//qDebug() << "before: " << mRelativePath;

		// проверка на не существование текущей директории
		while (!QDir(getRootPath() + mRelativePath).exists())
		{
			//qDebug() << "working...";

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

		//qDebug() << "after: " << mRelativePath;

		// пересоздание объекта слежения
		recreateWatcher();

		// обновить текущую директорию
		update();
	}
}

// слот об изменении файла внутри текущей директории
void SpriteBrowser::onFileChanged(const QString &absoluteFileName)
{
	//qDebug() << "onFileChanged";

	// проверка отсутствия файла
	if (!QFile::exists(absoluteFileName))
	{
		// файл удален - прекратить обработку
		return;
	}

	QString temptePath(absoluteFileName);
	// поиск последнего слеша пути
	int i = temptePath.lastIndexOf('/');
	if (i != -1)
	{
		// удаление содержимого строки с последнего слеша не включительно
		temptePath.resize(i + 1);
	}
	else
	{
		qDebug() << "Error in onFileChanged";
	}

	// отправка на подгрузку иконки в фоне
	if (temptePath == getRootPath() + mRelativePath)
		mThumbnailLoader->addFileForLoading(absoluteFileName);
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

	update();
}

QString SpriteBrowser::getRootPath() const
{
	return Utils::addTrailingSlash(Options::getSingleton().getDataDirectory());
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
void SpriteBrowser::update()
{
	// qDebug() << "update:" << mRelativePath;

	// отчистка поля иконок
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

	// проход по файлам
	for (QStringList::const_iterator iterEntries = listEntries.begin(); iterEntries != listEntries.end(); ++iterEntries)
	{
		QListWidgetItem *item = new QListWidgetItem((*iterEntries), mListWidget);

		// формирование полного пути к иконке
		QString absoluteFileName = getRootPath() + mRelativePath + (*iterEntries);

		// получение информации о текущем файле
		QFileInfo fileInfo(absoluteFileName);

		QMap<QString, IconWithInfo>::const_iterator iterIB = mThumbnailCache.find(absoluteFileName);

		if (
			// проверка отсутствия иконки в буфере
			iterIB == mThumbnailCache.end()
			// проверка несовпадения размера файла
			|| iterIB.value().mFileSize != fileInfo.size()
			// проверка несовпадения даты создания
			|| iterIB.value().mFileDate != fileInfo.lastModified())
		{
			// установка иконки "загружается"
			item->setIcon(mIconSpriteLoading);

			// отправка на подгрузку иконки в фоне
			mThumbnailLoader->addFileForLoading(absoluteFileName);
		}
		else
		{
			// установка сохраненной иконки
			item->setIcon(iterIB.value().mIcon);
		}

		// сохраняем относительный путь к файлу в UserRole для поддержки перетаскивания
		item->setData(Qt::UserRole, "sprite://" + mRelativePath + *iterEntries);

		// всплывающая подсказка с полным именем
		item->setToolTip(*iterEntries);

		// устанавливаем слежку на файлом
		if (QFile::exists(absoluteFileName))
			mWatcher->addPath(absoluteFileName);
	}
}
