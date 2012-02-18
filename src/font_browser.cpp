#include "font_browser.h"
#include "options.h"

FontBrowser::FontBrowser(QWidget *parent) :
	QDockWidget(parent)
{
	setupUi(this);

	scanFonts();
}

QString FontBrowser::getRootPath() const
{
	return Options::getSingleton().getDataDirectory() + "fonts/";
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

		// создание иконки в ГУИ
		QListWidgetItem *item = new QListWidgetItem(fileName, mFontListWidget);
		// FIXME:
		item->setIcon(QIcon());

		// сохраняем относительный путь к файлу в UserRole для поддержки перетаскивания
		item->setData(Qt::UserRole, "sprite://sprites/" + fileName);

		// всплывающая подсказка с несокращенным именем
		item->setToolTip(fileName);
	}

}

