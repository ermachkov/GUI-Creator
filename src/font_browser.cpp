#include "pch.h"
#include "font_browser.h"
#include "font_manager.h"
#include "project.h"
#include "utils.h"

FontBrowser::FontBrowser(QWidget *parent)
: QDockWidget(parent), mFrameBuffer(NULL)
{
	setupUi(this);

	// создание фреймбуффера первоначального размера
	recreateFrameBuffer(32, 256);

	mFileModel = new QFileSystemModel(this);

	connect(mFileModel, SIGNAL(directoryLoaded(const QString &)), this, SLOT(onDirectoryLoaded(const QString &)));
	connect(mFileModel, SIGNAL(fileRenamed(const QString &, const QString &, const QString &)), this, SLOT(onFileRenamed(const QString &, const QString &, const QString &)));
	connect(mFileModel, SIGNAL(rootPathChanged(const QString &)), this, SLOT(onRootPathChanged(const QString &)));

	mFileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot );
	QStringList fileNameFilters;
	fileNameFilters << "*.ttf";
	mFileModel->setNameFilters(fileNameFilters);
	mFileModel->setNameFilterDisables(false);

	QModelIndex rootIndex = mFileModel->setRootPath(getFontPath());

	mFontListView->setModel(mFileModel);
	mFontListView->setRootIndex(rootIndex);

	// установка делегата
	PreviewItemDelegate *previewItemDelegate = new PreviewItemDelegate(this);
	mFontListView->setItemDelegate(previewItemDelegate);
}

FontBrowser::~FontBrowser()
{
	FontManager::getSingleton().makeCurrent();

	Q_ASSERT(mFrameBuffer);
	delete mFrameBuffer;
}

QWidget *FontBrowser::getFontWidget() const
{
	return mFontListView;
}

void FontBrowser::onDirectoryLoaded(const QString &path)
{
	qDebug() << "onDirectoryLoaded:" << path;
	qDebug() << "root path:" << mFileModel->rootPath();

	if (path != mFileModel->rootPath())
		return;

	// загрузка и отображение списка доступных шрифтов
	scanFonts(path);

	// FIXME:
	mFontListView->setRootIndex(mFileModel->index(path));
}

void FontBrowser::onFileRenamed(const QString &path, const QString &oldName, const QString &newName)
{
	qDebug() << "onFileRenamed";
}

void FontBrowser::onRootPathChanged(const QString &newPath)
{
	qDebug() << "onRootPathChanged" << newPath;
}

void FontBrowser::on_mFontListView_activated(const QModelIndex &index)
{
	if (!mFileModel->fileInfo(index).isDir())
		return;

	QString path = mFileModel->fileInfo(index).absoluteFilePath();

	qDebug() << "on_mFontListView_activated:" << path;

	if (Utils::addTrailingSlash(path) == getFontPath())
		mFileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
	else
		mFileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDot);

	mFileModel->setRootPath(path);
}

FontBrowser::PreviewItemDelegate::PreviewItemDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

void FontBrowser::PreviewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// родительская отрисовка
	QItemDelegate::paint(painter, option, index);

	FontBrowser *fontBrowser = qobject_cast<FontBrowser *>(this->parent());
	QMap<QString, QImage>::iterator it = fontBrowser->mImages.find(index.data().toString());
	if (it == fontBrowser->mImages.end())
		return;

	// получение картинки
	QImage image = it.value();

	// получение координат ячейки
	int x = option.rect.left();
	int y = option.rect.top();

	// prepare
	painter->save();

	// произвольная отрисовка
	painter->drawImage(x, y, image);

	// done
	painter->restore();
}

QSize FontBrowser::PreviewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	FontBrowser *fontBrowser = qobject_cast<FontBrowser *>(this->parent());
	QMap<QString, QImage>::iterator it = fontBrowser->mImages.find(index.data().toString());
	if (it == fontBrowser->mImages.end())
		return QItemDelegate::sizeHint(option, index);

	// получение картинки
	QImage image = it.value();

	// возврат требуемого размера под надпись
	return image.size();
}

QString FontBrowser::getFontPath() const
{
	return Project::getSingleton().getRootDirectory() + Project::getSingleton().getFontsDirectory();
}

QString FontBrowser::getRootPath() const
{
	return Project::getSingleton().getRootDirectory();
}

void FontBrowser::recreateFrameBuffer(int width, int height)
{
	FontManager::getSingleton().makeCurrent();

	delete mFrameBuffer;
	mFrameBuffer = new QGLFramebufferObject(width, height, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGB);
}

void FontBrowser::scanFonts(const QString &path)
{
	qDebug() << "scanFonts" << path;

//	// всплывающая подсказка с полным путем
//	mFontListWidget->setToolTip(getFontPath());

//	// очистка поля ГУИ иконок
//	mFontListWidget->clear();

	// очистка изображений предпросмотра
	mImages.clear();

	//QModelIndex rootIndex = mFileModel->index(mFileModel->rootPath());
	//QModelIndex rootIndex = mFontListView->rootIndex();
	QModelIndex rootIndex = mFileModel->index(path);

	qDebug() << "lala:" << rootIndex.data();
	qDebug() << "count:" << mFileModel->rowCount(rootIndex);

	for (int row = 0; row < mFileModel->rowCount(rootIndex); ++row)
	{
		QModelIndex index = mFileModel->index(row, 2, rootIndex);

		//qDebug() << "indexes 2:" << index.data();

		// Если каталог - то пропустить
		if (mFileModel->data(index) == QVariant("File Folder"))
			continue;

		index = mFileModel->index(row, 0, rootIndex);
		QString fileName = mFileModel->data(index).toString();

		//qDebug() << "indexes 0:" << fileName;

		// формирование полного пути к ttf файлу
		QString absoluteFileName =  mFileModel->fileInfo(index).absoluteFilePath();

		// полный путь к файлу шрифта должен содержать коренную директорию
		Q_ASSERT(getRootPath() == absoluteFileName.mid(0, getRootPath().size()));

		// формирование относительного пути к ttf файлу
		QString relativeFileName = absoluteFileName.mid(getRootPath().size());

		qDebug() << "-- relativeFileName:" << relativeFileName;

		// размер шрифта для предпросмотра
		int fontSize = 18;

		// загрузка шрифта во временную переменную
		QSharedPointer<FTFont> tempFont = FontManager::getSingleton().loadFont(relativeFileName, fontSize, false);

		// FIXME: правильно обработать случай, когда шрифт не загрузился, и не пытаться дергать нулевой указатель
		if (tempFont.isNull())
			qDebug() << "Error in FontBrowser::scanFonts()";

		// определение размера требуемой области рисования
		QSizeF floatTextSize = QSizeF(tempFont->Advance(Utils::toStdWString(fileName).c_str()), tempFont->LineHeight());
		// округление по модулю вверх...
		QSize textSize =  QSize(qCeil(floatTextSize.width()), qCeil(floatTextSize.height()));

		// пересоздание фреймбуфера, если требуемые размеры больше текущии размеров фреймбуфера
		if (textSize.width() > mFrameBuffer->width() || textSize.height() > mFrameBuffer->height())
			recreateFrameBuffer(qMax(textSize.width(), mFrameBuffer->width()), qMax(textSize.height(), mFrameBuffer->height()));

		// установка контекста OpenGL
		FontManager::getSingleton().makeCurrent();

		mFrameBuffer->bind();

		// очистка
		QColor color = palette().base().color();
		glClearColor(color.redF(), color.greenF(), color.blueF(), 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// настройка текстурирования
		glEnable(GL_TEXTURE_2D);

		// настройка смешивания
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// устанавливаем систему координат с началом координат в левом верхнем углу
		glViewport(0, 0, mFrameBuffer->width(), mFrameBuffer->height());

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, mFrameBuffer->width(), mFrameBuffer->height(), 0.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslated(0.0, qRound(tempFont->LineHeight() / 1.25), 0.0);
		glScaled(1.0, -1.0, 1.0);

		glColor4d(0.0, 0.0, 0.0, 1.0);

		// отрисовка предпросмотра шрифта
		tempFont->Render(Utils::toStdWString(fileName).c_str());

		// копирование части отрисованного изображения
		QImage image = mFrameBuffer->toImage().copy(0, 0, textSize.width(), textSize.height());

		mFrameBuffer->release();

		// запись изображения предпросмотра в массив
		mImages.insert(fileName, image);
	}
}

