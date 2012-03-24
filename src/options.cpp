#include "pch.h"
#include "options.h"
#include "utils.h"

template<> Options *Singleton<Options>::mSingleton = NULL;

Options::Options(QSettings &settings)
{
	// загружаем общие настройки
	settings.beginGroup("General");
	mLastOpenedDirectory = Utils::addTrailingSlash(settings.value("LastOpenedDirectory", QDir::currentPath()).toString());
	mRecentFiles = settings.value("RecentFiles").toStringList();
	settings.endGroup();

	// загружаем настройки сетки
	settings.beginGroup("Grid");
	mShowGrid = settings.value("ShowGrid", true).toBool();
	mShowDots = settings.value("ShowDots", false).toBool();
	mSnapToGrid = settings.value("SnapToGrid", false).toBool();
	mSnapToVisibleLines = settings.value("SnapToVisibleLines", false).toBool();
	mGridSpacing = settings.value("GridSpacing", 16).toInt();
	mMajorLinesInterval = settings.value("MajorLinesInterval", 5).toInt();
	settings.endGroup();

	// загружаем настройки направляющих
	settings.beginGroup("Guides");
	mShowGuides = settings.value("ShowGuides", true).toBool();
	mSnapToGuides = settings.value("SnapToGuides", true).toBool();
	mEnableSmartGuides = settings.value("EnableSmartGuides", true).toBool();
	settings.endGroup();
}

void Options::save(QSettings &settings)
{
	// сохраняем общие настройки
	settings.beginGroup("General");
	settings.setValue("LastOpenedDirectory", mLastOpenedDirectory);
	settings.setValue("RecentFiles", mRecentFiles);
	settings.endGroup();

	// сохраняем настройки сетки
	settings.beginGroup("Grid");
	settings.setValue("ShowGrid", mShowGrid);
	settings.setValue("ShowDots", mShowDots);
	settings.setValue("SnapToGrid", mSnapToGrid);
	settings.setValue("SnapToVisibleLines", mSnapToVisibleLines);
	settings.setValue("GridSpacing", mGridSpacing);
	settings.setValue("MajorLinesInterval", mMajorLinesInterval);
	settings.endGroup();

	// сохраняем настройки направляющих
	settings.beginGroup("Guides");
	settings.setValue("ShowGuides", mShowGuides);
	settings.setValue("SnapToGuides", mSnapToGuides);
	settings.setValue("EnableSmartGuides", mEnableSmartGuides);
	settings.endGroup();
}

QString Options::getLastOpenedDirectory() const
{
	return mLastOpenedDirectory;
}

void Options::setLastOpenedDirectory(const QString &dir)
{
	mLastOpenedDirectory = Utils::addTrailingSlash(dir);
}

QStringList Options::getRecentFiles() const
{
	return mRecentFiles;
}

void Options::setRecentFiles(const QStringList &list)
{
	mRecentFiles = list;
}

bool Options::isShowGrid() const
{
	return mShowGrid;
}

void Options::setShowGrid(bool showGrid)
{
	mShowGrid = showGrid;
}

bool Options::isShowDots() const
{
	return mShowDots;
}

void Options::setShowDots(bool showDots)
{
	mShowDots = showDots;
}

bool Options::isSnapToGrid() const
{
	return mSnapToGrid;
}

void Options::setSnapToGrid(bool snapToGrid)
{
	mSnapToGrid = snapToGrid;
}

bool Options::isSnapToVisibleLines() const
{
	return mSnapToVisibleLines;
}

void Options::setSnapToVisibleLines(bool snapToVisibleLines)
{
	mSnapToVisibleLines = snapToVisibleLines;
}

int Options::getGridSpacing() const
{
	return mGridSpacing;
}

void Options::setGridSpacing(int gridSpacing)
{
	mGridSpacing = gridSpacing;
}

int Options::getMajorLinesInterval() const
{
	return mMajorLinesInterval;
}

void Options::setMajorLinesInterval(int majorLinesInterval)
{
	mMajorLinesInterval = majorLinesInterval;
}

bool Options::isShowGuides() const
{
	return mShowGuides;
}

void Options::setShowGuides(bool showGuides)
{
	mShowGuides = showGuides;
}

bool Options::isSnapToGuides() const
{
	return mSnapToGuides;
}

void Options::setSnapToGuides(bool snapToGuides)
{
	mSnapToGuides = snapToGuides;
}

bool Options::isEnableSmartGuides() const
{
	return mEnableSmartGuides;
}

void Options::setEnableSmartGuides(bool enableSmartGuides)
{
	mEnableSmartGuides = enableSmartGuides;
}
