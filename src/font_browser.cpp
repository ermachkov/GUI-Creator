#include "pch.h"
#include "font_browser.h"
#include "font_manager.h"
#include "options.h"
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

		int fontSize = 14;
		QString drawingText = fileName;

		// загрузка шрифта во временную переменную
		QSharedPointer<FTFont> tempFont = FontManager::getSingleton().loadFont("fonts/" + fileName, fontSize, false);

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

		// отчистка
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

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
		previewItem->setData(Qt::UserRole + 1, "fonts/" + fileName);

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

/*
void QItemDelegate::paint(QPainter *painter,
						  const QStyleOptionViewItem &option,
						  const QModelIndex &index) const
{
	Q_D(const QItemDelegate);
	Q_ASSERT(index.isValid());

	QStyleOptionViewItemV4 opt = setOptions(index, option);

	const QStyleOptionViewItemV2 *v2 = qstyleoption_cast<const QStyleOptionViewItemV2 *>(&option);
	opt.features = v2 ? v2->features
					: QStyleOptionViewItemV2::ViewItemFeatures(QStyleOptionViewItemV2::None);
	const QStyleOptionViewItemV3 *v3 = qstyleoption_cast<const QStyleOptionViewItemV3 *>(&option);
	opt.locale = v3 ? v3->locale : QLocale();
	opt.widget = v3 ? v3->widget : 0;

	// prepare
	painter->save();
	if (d->clipPainting)
		painter->setClipRect(opt.rect);

	// get the data and the rectangles

	QVariant value;

	QPixmap pixmap;
	QRect decorationRect;
	value = index.data(Qt::DecorationRole);
	if (value.isValid())
	{
		// ### we need the pixmap to call the virtual function
		pixmap = decoration(opt, value);
		if (value.type() == QVariant::Icon) {
			d->tmp.icon = qvariant_cast<QIcon>(value);
			d->tmp.mode = d->iconMode(option.state);
			d->tmp.state = d->iconState(option.state);
			const QSize size = d->tmp.icon.actualSize(option.decorationSize,
													  d->tmp.mode, d->tmp.state);
			decorationRect = QRect(QPoint(0, 0), size);
		}
		else
		{
			d->tmp.icon = QIcon();
			decorationRect = QRect(QPoint(0, 0), pixmap.size());
		}
	}
	else
	{
		d->tmp.icon = QIcon();
		decorationRect = QRect();
	}

	QString text;
	QRect displayRect;
	value = index.data(Qt::DisplayRole);
	if (value.isValid() && !value.isNull())
	{
		text = QItemDelegatePrivate::valueToText(value, opt);
		displayRect = textRectangle(painter, d->textLayoutBounds(opt), opt.font, text);
	}

	QRect checkRect;
	Qt::CheckState checkState = Qt::Unchecked;
	value = index.data(Qt::CheckStateRole);
	if (value.isValid())
	{
		checkState = static_cast<Qt::CheckState>(value.toInt());
		checkRect = check(opt, opt.rect, value);
	}

	// do the layout

	doLayout(opt, &checkRect, &decorationRect, &displayRect, false);

	// draw the item

	drawBackground(painter, opt, index);
	drawCheck(painter, opt, checkRect, checkState);
	drawDecoration(painter, opt, decorationRect, pixmap);
	drawDisplay(painter, opt, displayRect, text);
	drawFocus(painter, opt, displayRect);

	// done
	painter->restore();
}

	Returns the size needed by the delegate to display the item
	specified by \a index, taking into account the style information
	provided by \a option.

	When reimplementing this function, note that in case of text
	items, QItemDelegate adds a margin (i.e. 2 *
	QStyle::PM_FocusFrameHMargin) to the length of the text.

QSize QItemDelegate::sizeHint(const QStyleOptionViewItem &option,
							  const QModelIndex &index) const
{
	QVariant value = index.data(Qt::SizeHintRole);
	if (value.isValid())
		return qvariant_cast<QSize>(value);

	QRect decorationRect = rect(option, index, Qt::DecorationRole);
	QRect displayRect = rect(option, index, Qt::DisplayRole);
	QRect checkRect = rect(option, index, Qt::CheckStateRole);

	doLayout(option, &checkRect, &decorationRect, &displayRect, true);

	return (decorationRect|displayRect|checkRect).size();
}
*/

/*
!
	\internal

	Code duplicated in QCommonStylePrivate::viewItemLayout

void QItemDelegate::doLayout(const QStyleOptionViewItem &option,
							 QRect *checkRect, QRect *pixmapRect, QRect *textRect,
							 bool hint) const
{
	Q_ASSERT(checkRect && pixmapRect && textRect);
	Q_D(const QItemDelegate);
	const QWidget *widget = d->widget(option);
	QStyle *style = widget ? widget->style() : QApplication::style();
	const bool hasCheck = checkRect->isValid();
	const bool hasPixmap = pixmapRect->isValid();
	const bool hasText = textRect->isValid();
	const int textMargin = hasText ? style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1 : 0;
	const int pixmapMargin = hasPixmap ? style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1 : 0;
	const int checkMargin = hasCheck ? style->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, widget) + 1 : 0;
	int x = option.rect.left();
	int y = option.rect.top();
	int w, h;

	textRect->adjust(-textMargin, 0, textMargin, 0); // add width padding
	if (textRect->height() == 0 && (!hasPixmap || !hint)) {
		//if there is no text, we still want to have a decent height for the item sizeHint and the editor size
		textRect->setHeight(option.fontMetrics.height());
	}

	QSize pm(0, 0);
	if (hasPixmap) {
		pm = pixmapRect->size();
		pm.rwidth() += 2 * pixmapMargin;
	}
	if (hint) {
		h = qMax(checkRect->height(), qMax(textRect->height(), pm.height()));
		if (option.decorationPosition == QStyleOptionViewItem::Left
			|| option.decorationPosition == QStyleOptionViewItem::Right) {
			w = textRect->width() + pm.width();
		} else {
			w = qMax(textRect->width(), pm.width());
		}
	} else {
		w = option.rect.width();
		h = option.rect.height();
	}

	int cw = 0;
	QRect check;
	if (hasCheck) {
		cw = checkRect->width() + 2 * checkMargin;
		if (hint) w += cw;
		if (option.direction == Qt::RightToLeft) {
			check.setRect(x + w - cw, y, cw, h);
		} else {
			check.setRect(x + checkMargin, y, cw, h);
		}
	}

	// at this point w should be the *total* width

	QRect display;
	QRect decoration;
	switch (option.decorationPosition) {
	case QStyleOptionViewItem::Top: {
		if (hasPixmap)
			pm.setHeight(pm.height() + pixmapMargin); // add space
		h = hint ? textRect->height() : h - pm.height();

		if (option.direction == Qt::RightToLeft) {
			decoration.setRect(x, y, w - cw, pm.height());
			display.setRect(x, y + pm.height(), w - cw, h);
		} else {
			decoration.setRect(x + cw, y, w - cw, pm.height());
			display.setRect(x + cw, y + pm.height(), w - cw, h);
		}
		break; }
	case QStyleOptionViewItem::Bottom: {
		if (hasText)
			textRect->setHeight(textRect->height() + textMargin); // add space
		h = hint ? textRect->height() + pm.height() : h;

		if (option.direction == Qt::RightToLeft) {
			display.setRect(x, y, w - cw, textRect->height());
			decoration.setRect(x, y + textRect->height(), w - cw, h - textRect->height());
		} else {
			display.setRect(x + cw, y, w - cw, textRect->height());
			decoration.setRect(x + cw, y + textRect->height(), w - cw, h - textRect->height());
		}
		break; }
	case QStyleOptionViewItem::Left: {
		if (option.direction == Qt::LeftToRight) {
			decoration.setRect(x + cw, y, pm.width(), h);
			display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
		} else {
			display.setRect(x, y, w - pm.width() - cw, h);
			decoration.setRect(display.right() + 1, y, pm.width(), h);
		}
		break; }
	case QStyleOptionViewItem::Right: {
		if (option.direction == Qt::LeftToRight) {
			display.setRect(x + cw, y, w - pm.width() - cw, h);
			decoration.setRect(display.right() + 1, y, pm.width(), h);
		} else {
			decoration.setRect(x, y, pm.width(), h);
			display.setRect(decoration.right() + 1, y, w - pm.width() - cw, h);
		}
		break; }
	default:
		qWarning("doLayout: decoration position is invalid");
		decoration = *pixmapRect;
		break;
	}

	if (!hint) { // we only need to do the internal layout if we are going to paint
		*checkRect = QStyle::alignedRect(option.direction, Qt::AlignCenter,
										 checkRect->size(), check);
		*pixmapRect = QStyle::alignedRect(option.direction, option.decorationAlignment,
										  pixmapRect->size(), decoration);
		// the text takes up all available space, unless the decoration is not shown as selected
		if (option.showDecorationSelected)
			*textRect = display;
		else
			*textRect = QStyle::alignedRect(option.direction, option.displayAlignment,
											textRect->size().boundedTo(display.size()), display);
	} else {
		*checkRect = check;
		*pixmapRect = decoration;
		*textRect = display;
	}
}


*/
