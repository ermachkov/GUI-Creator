#include "pch.h"
#include "font_browser.h"
#include "font_manager.h"
#include "project.h"
#include "utils.h"

FontBrowser::FontBrowser(QWidget *parent)
: QDockWidget(parent), mFrameBuffer(NULL)
{
	setupUi(this);

	mIconDrawText = QIcon(":/images/draw_text.png");

	// создание фреймбуффера первоначального размера
	recreateFrameBuffer(32, 256);

	// установка делегата
	mPreviewListWidget->setItemDelegate(new PreviewItemDelegate(this));

	// загрузка и отображение списка доступных шрифтов
	scanFonts();
}

FontBrowser::~FontBrowser()
{
	FontManager::getSingleton().makeCurrent();

	Q_ASSERT(mFrameBuffer);
	delete mFrameBuffer;
}

QString FontBrowser::getRootPath() const
{
	return Project::getSingleton().getRootDirectory() + Project::getSingleton().getFontsDirectory();
}

void FontBrowser::recreateFrameBuffer(int width, int height)
{
	FontManager::getSingleton().makeCurrent();

	delete mFrameBuffer;
	// FIXME: отваливается рисование
	mFrameBuffer = new QGLFramebufferObject(width, height, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGB);
}

void FontBrowser::scanFonts()
{
	// всплывающая подсказка с полным путем
	mFontListWidget->setToolTip(getRootPath());

	// очистка поля ГУИ иконок
	mFontListWidget->clear();

	QDir currentDir = QDir(getRootPath());
	currentDir.setFilter(QDir::Files);
	QStringList fileNameFilters;
	fileNameFilters << "*.ttf";
	currentDir.setNameFilters(fileNameFilters);
	QStringList listEntries = currentDir.entryList();

	foreach (const QString &fileName, listEntries)
	{
		// формирование полного пути к ttf файлу
		QString absoluteFileName = getRootPath() + fileName;

		// создание иконки в ГУИ шрифтов
		QListWidgetItem *item = new QListWidgetItem(fileName, mFontListWidget);

		// установка иконки по умолчанию
		item->setIcon(mIconDrawText);

		// сохраняем тип и относительный путь к файлу для поддержки перетаскивания
		item->setData(Qt::UserRole, "Label");
		item->setData(Qt::UserRole + 1, Project::getSingleton().getFontsDirectory() + fileName);

		// всплывающая подсказка с несокращенным именем
		item->setToolTip(fileName);

		int fontSize = 14;
		QString drawingText = fileName;

		// загрузка шрифта во временную переменную
		QSharedPointer<FTFont> tempFont = FontManager::getSingleton().loadFont(Project::getSingleton().getFontsDirectory() + fileName, fontSize, false);

		if (tempFont.isNull())
			qDebug() << "Error in FontBrowser::scanFonts()";

		// создание иконки в ГУИ используемых шрифтов
		QListWidgetItem *previewItem = new QListWidgetItem("", mPreviewListWidget);

		// определение размера требуемой области рисования
		QSizeF floatTextSize = QSizeF(tempFont->Advance(Utils::toStdWString(drawingText).c_str()), tempFont->LineHeight());
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
		tempFont->Render(Utils::toStdWString(drawingText).c_str());

		// копирование части отрисованного изображения
		QImage image = mFrameBuffer->toImage().copy(0, 0, textSize.width(), textSize.height());

		mFrameBuffer->release();


		// назначение type шрифта (UserRole + 0)
		previewItem->setData(Qt::UserRole, QString("Label"));

		// назначение относительного пути до шрифта /fonts/*.ttf (UserRole + 1)
		previewItem->setData(Qt::UserRole + 1, Project::getSingleton().getFontsDirectory() + fileName);

		// назначение размера шрифта ttf (UserRole + 2)
		previewItem->setData(Qt::UserRole + 2, fontSize);

		// назначение картинки с надписью (UserRole + 3)
		previewItem->setData(Qt::UserRole + 3, image);
	}
}

FontBrowser::PreviewItemDelegate::PreviewItemDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

void FontBrowser::PreviewItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	// родительская отрисовка
	QItemDelegate::paint(painter, option, index);

	// получение картинки
	QImage image = qvariant_cast<QImage>(index.data(Qt::UserRole + 3));

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
	// получение картинки
	QImage image = qvariant_cast<QImage>(index.data(Qt::UserRole + 3));

	// возврат требуемого размера под надпись
	return image.size();
}
