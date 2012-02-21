#include "pch.h"
#include "font_browser.h"
#include "font_manager.h"
#include "options.h"

FontBrowser::FontBrowser(QWidget *parent)
: QDockWidget(parent), mFrameBuffer(NULL)
{
	setupUi(this);

	mIconDrawText = QIcon(":/images/draw_text.png");

	// создание фреймбуффера
	recreateFrameBuffer(32, 32);

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
	return Options::getSingleton().getDataDirectory() + "fonts/";
}

void FontBrowser::recreateFrameBuffer(int width, int height)
{
	FontManager::getSingleton().makeCurrent();

	delete mFrameBuffer;
	mFrameBuffer = new QGLFramebufferObject(width, height);
}

void FontBrowser::scanFonts()
{
	// всплывающая подсказка с полным путем
	mFontListWidget->setToolTip(getRootPath());

	// отчистка поля ГУИ иконок
	mFontListWidget->clear();

	QDir currentDir = QDir(getRootPath());
	currentDir.setFilter(QDir::Files);
	QStringList fileNameFilters;
	fileNameFilters << "*.ttf";
	currentDir.setNameFilters(fileNameFilters);
	QStringList listEntries = currentDir.entryList();

	foreach (const QString &fileName, listEntries)
	{
		// формирование полного пути к иконке
		QString absoluteFileName = getRootPath() + fileName;

		// создание иконки в ГУИ шрифтов
		QListWidgetItem *item = new QListWidgetItem(fileName, mFontListWidget);

		// установка иконки по умолчанию
		item->setIcon(mIconDrawText);

		// сохраняем относительный путь к файлу в UserRole для поддержки перетаскивания
		item->setData(Qt::UserRole, "label://fonts/" + fileName);

		// всплывающая подсказка с несокращенным именем
		item->setToolTip(fileName);

		// FIXME: ограничить размер фреймбуфера

		// загрузка шрифта во временный список
		QSharedPointer<FTFont> tempFont = FontManager::getSingleton().loadFont("fonts/" + fileName, 200, false);
//		fontVector.insert(tempFont);
		// создание иконки в ГУИ используемых шрифтов
		QListWidgetItem *previewItem = new QListWidgetItem("", mPreviewListWidget);
		// генерация иконки предпросмотра текста

		if (tempFont.isNull())
			qDebug() << "Error in FontBrowser::scanFonts()";

		QString drawingText = fileName;

		// определение размера требуемой области рисования
		QSizeF floatTextSize = QSizeF(tempFont->Advance(drawingText.toStdWString().c_str()), tempFont->LineHeight());
		// округление по модулю вверх...
		QSize textSize =  QSize(qCeil(floatTextSize.width()), qCeil(floatTextSize.height()));

		// пересоздание фреймбуфера, если требуемые размеры больше текущии размеров фреймбуфера
		if (textSize.width() > mFrameBuffer->width() || textSize.height() > mFrameBuffer->height())
			recreateFrameBuffer(qMax(textSize.width(), mFrameBuffer->width()), qMax(textSize.height(), mFrameBuffer->height()));

		qDebug() << "text w:" << textSize.width() << "text h:" << textSize.height();

		FontManager::getSingleton().makeCurrent();

		mFrameBuffer->bind();

		// отчистка
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// устанавливаем систему координат с началом координат в левом верхнем углу
		glViewport(0, 0, textSize.width(), textSize.height());

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluOrtho2D(0.0, textSize.width(), textSize.height(), 0.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslated(0.0, qRound(tempFont->LineHeight() / 1.25), 0.0);
		glScaled(1.0, -1.0, 1.0);

		glColor4d(0.0, 0.0, 0.0, 1.0);

		// отрисовка предпросмотра шрифта
		tempFont->Render(drawingText.toStdWString().c_str());

		QImage image = mFrameBuffer->toImage();
		mFrameBuffer->release();

		//QIcon(QPixmap::fromImage(image)).
		previewItem->setIcon(QIcon(QPixmap::fromImage(image)));
	}
}

