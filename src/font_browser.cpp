#include "pch.h"
#include "font_browser.h"
#include "font_manager.h"
#include "project.h"
#include "utils.h"

FontBrowser::FontBrowser(QWidget *parent)
: QDockWidget(parent)
{
	setupUi(this);

	mFileModel = new FontFileSystemModel(this);

	connect(mFileModel, SIGNAL(directoryLoaded(const QString &)), this, SLOT(onDirectoryLoaded(const QString &)));

	mFileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot );
	QStringList fileNameFilters;
	fileNameFilters << "*.ttf";
	mFileModel->setNameFilters(fileNameFilters);
	mFileModel->setNameFilterDisables(false);

	QModelIndex rootIndex = mFileModel->setRootPath(getFontPath());

	mFontListView->setModel(mFileModel);
	mFontListView->setRootIndex(rootIndex);

	// установка делегата
	mPreviewItemDelegate = new PreviewItemDelegate(this, mFileModel);
	mFontListView->setItemDelegate(mPreviewItemDelegate);
}

QWidget *FontBrowser::getFontWidget() const
{
	return mFontListView;
}

bool FontBrowser::isImageLoaded(const QModelIndex &index) const
{
	QString fileName = index.data().toString();

	QMap<QString, Images> images = mPreviewItemDelegate->getImages();

	QMap<QString, Images>::const_iterator it = images.find(fileName);
	if (it != images.end())
		return true;

	return false;
}

void FontBrowser::onDirectoryLoaded(const QString &)
{
	QString path = mFileModel->rootPath();

	mFontListView->setRootIndex(mFileModel->index(path));

	// очистка списка загруженных шрифтов
	mPreviewItemDelegate->clearAllImages();
}

void FontBrowser::on_mFontListView_activated(const QModelIndex &index)
{
	if (!mFileModel->isDir(index))
		return;

	QString path = mFileModel->fileInfo(index).canonicalFilePath();

	if (Utils::addTrailingSlash(path) == getFontPath())
		mFileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
	else
		mFileModel->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDot);

	mFileModel->setRootPath(path);
}

FontBrowser::FontFileSystemModel::FontFileSystemModel(FontBrowser *parent)
: QFileSystemModel(parent), mFontBrowser(parent)
{
}

Qt::ItemFlags FontBrowser::FontFileSystemModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags flags = QFileSystemModel::flags(index);
	if (!index.isValid())
		return flags;

	// очистка флагов перетаскивания
	flags &= ~Qt::ItemIsDragEnabled;

	// разрешение перетаскивания если не директория и файл загружен
	if (!isDir(index) && mFontBrowser->isImageLoaded(index))
		flags |= Qt::ItemIsDragEnabled;

	return flags;
}

FontBrowser::PreviewItemDelegate::PreviewItemDelegate(QObject *parent, FontFileSystemModel *fileModel)
: QItemDelegate(parent), mFileModel(fileModel), mFrameBuffer(NULL)
{
	// создание фреймбуффера первоначального размера
	recreateFrameBuffer(32, 256);
}

FontBrowser::PreviewItemDelegate::~PreviewItemDelegate()
{
	FontManager::getSingleton().makeCurrent();

	Q_ASSERT(mFrameBuffer);
	delete mFrameBuffer;
}

void FontBrowser::PreviewItemDelegate::drawDisplay(QPainter *painter,
	const QStyleOptionViewItem &option,	const QRect &rect, const QString &text) const
{
	// получение картинки
	QImage image = getImage(option, text);

	// высчитывание смещения текста
	QStyle *style = QApplication::style();
	const int textMargin = style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0) + 1;

	if (!image.isNull())
	{
		// получение координат ячейки
		int x = rect.left() + textMargin;
		int y = rect.top();
		// prepare
		painter->save();
		// произвольная отрисовка
		painter->drawImage(x, y, image);
		// done
		painter->restore();
	}
	else
	{
		QItemDelegate::drawDisplay(painter, option, rect, text);
	}
}


QSize FontBrowser::PreviewItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	mFileModel->flags(index);

	// возврат требуемого размера под надпись
	if (!mFileModel->isDir(index))
	{
		createImage(option, index.data().toString());
		QImage image = getImage(option, index.data().toString());
		if (!image.isNull())
		{
			return image.size();
		}
	}

	return QItemDelegate::sizeHint(option, index);
}

void FontBrowser::PreviewItemDelegate::recreateFrameBuffer(int width, int height) const
{
	FontManager::getSingleton().makeCurrent();

	delete mFrameBuffer;
	mFrameBuffer = new QGLFramebufferObject(width, height, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGB);
}

void FontBrowser::PreviewItemDelegate::createImage(const QStyleOptionViewItem &option, const QString &text) const
{
	// получение цвета из родительского виджета
	FontBrowser *fontBrowser = qobject_cast<FontBrowser *>(parent());

	QMap<QString, Images>::iterator it = mImages.find(text);
	if (it == mImages.end())
	{
		// создание иконок предпросмотра
		Images images;

		// размер шрифта для предпросмотра
		int fontSize = 24;

		// формирование полного пути к ttf файлу
		QString absoluteFileName = Utils::addTrailingSlash(mFileModel->rootPath()) + text;

		// полный путь к файлу шрифта должен содержать коренную директорию
		Q_ASSERT(fontBrowser->getRootPath() == absoluteFileName.mid(0, fontBrowser->getRootPath().size()));

		// формирование относительного пути к ttf файлу
		QString relativeFileName = absoluteFileName.mid(fontBrowser->getRootPath().size());

		// загрузка шрифта во временную переменную
		QSharedPointer<Font> tempFont = FontManager::getSingleton().loadFont(relativeFileName, fontSize, false);

		if (tempFont.isNull())
		{
			// qWarning() << "image" << index.data().toString() << "not loaded";
			return;
		}

		// определение размера требуемой области рисования
		QSizeF floatTextSize = QSizeF(tempFont->getWidth(text), tempFont->getHeight());
		// округление по модулю вверх...
		QSize textSize =  QSize(qCeil(floatTextSize.width()), qCeil(floatTextSize.height()));

		// пересоздание фреймбуфера, если требуемые размеры больше текущии размеров фреймбуфера
		if (textSize.width() > mFrameBuffer->width() || textSize.height() > mFrameBuffer->height())
			recreateFrameBuffer(qMax(textSize.width(), mFrameBuffer->width()), qMax(textSize.height(), mFrameBuffer->height()));

		// установка контекста OpenGL
		FontManager::getSingleton().makeCurrent();

		// захват фреймбуфера в рабство
		mFrameBuffer->bind();

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
		glTranslated(0.0, qRound(tempFont->getHeight() / 1.25), 0.0);
		glScaled(1.0, -1.0, 1.0);

		// @@ выделенный текст с фокусом
		QColor backgroungColor = option.palette.color(QPalette::Normal, QPalette::Highlight);
		QColor textColor = option.palette.color(QPalette::Normal, QPalette::HighlightedText);

		glClearColor(backgroungColor.redF(), backgroungColor.greenF(), backgroungColor.blueF(), 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4d(textColor.redF(), textColor.greenF(), textColor.blueF(), 1.0);

		// отрисовка предпросмотра шрифта
		tempFont->draw(text);

		// копирование части отрисованного изображения
		images.mHighlighted = mFrameBuffer->toImage().copy(0, 0, textSize.width(), textSize.height());

		// @@ выделенный текст без фокуса
		backgroungColor = option.palette.color(QPalette::Inactive, QPalette::Highlight);
		textColor = option.palette.color(QPalette::Inactive, QPalette::HighlightedText);

		glClearColor(backgroungColor.redF(), backgroungColor.greenF(), backgroungColor.blueF(), 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4d(textColor.redF(), textColor.greenF(), textColor.blueF(), 1.0);

		// отрисовка предпросмотра шрифта
		tempFont->draw(text);

		images.mHighlightedInactive = mFrameBuffer->toImage().copy(0, 0, textSize.width(), textSize.height());

		// @@ невыделенный текст
		backgroungColor = fontBrowser->palette().base().color();
		textColor = fontBrowser->palette().text().color();

		glClearColor(backgroungColor.redF(), backgroungColor.greenF(), backgroungColor.blueF(), 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glColor4d(textColor.redF(), textColor.greenF(), textColor.blueF(), 1.0);

		// отрисовка предпросмотра шрифта
		tempFont->draw(text);

		images.mNormal = mFrameBuffer->toImage().copy(0, 0, textSize.width(), textSize.height());

		// отпускание фреймбуфера на волю
		mFrameBuffer->release();

		mImages.insert(text, images);
	}
}

QImage FontBrowser::PreviewItemDelegate::getImage(const QStyleOptionViewItem &option, const QString &text) const
{
	QMap<QString, Images>::iterator it = mImages.find(text);
	if (it != mImages.end())
	{
		// возврат существующей иконки предпросмотра
		if (option.showDecorationSelected && (option.state & QStyle::State_Selected))
		{
			if (option.state & QStyle::State_Active)
			{
				// выделенный текст с фокусом
				return it.value().mHighlighted;
			}
			else
			{
				// выделенный текст без фокуса
				return it.value().mHighlightedInactive;
			}
		}
		else
		{
			// невыделенный текст
			return it.value().mNormal;
		}
	}

	// вернуть пустое изображение
	return QImage();
}

void FontBrowser::PreviewItemDelegate::clearAllImages()
{
	mImages.clear();
}

const QMap<QString, FontBrowser::Images> &FontBrowser::PreviewItemDelegate::getImages() const
{
	return mImages;
}

QString FontBrowser::getFontPath() const
{
	return Project::getSingleton().getRootDirectory() + Project::getSingleton().getFontsDirectory();
}

QString FontBrowser::getRootPath() const
{
	return Project::getSingleton().getRootDirectory();
}
