#include "pch.h"
#include "options.h"
#include "utils.h"

template<> Options *Singleton<Options>::mSingleton = NULL;

Options::Options(QSettings &settings)
{
	settings.beginGroup("/General");
	mDataDirectory = Utils::addTrailingSlash(settings.value("/DataDirectory", QDir::currentPath()).toString());
	mLastDirectory = Utils::addTrailingSlash(settings.value("/LastDirectory").toString());
	mRecentFiles = settings.value("/RecentFiles").toStringList();
	settings.endGroup();
}

void Options::save(QSettings &settings)
{
	settings.beginGroup("/General");
	settings.setValue("/DataDirectory", mDataDirectory);
	settings.setValue("/LastDirectory", mLastDirectory);
	settings.setValue("/RecentFiles", mRecentFiles);
	settings.endGroup();
}

QString Options::getDataDirectory() const
{
	return mDataDirectory;
}

void Options::setDataDirectory(const QString &dir)
{
	mDataDirectory = Utils::addTrailingSlash(dir);
}

QString Options::getLastDirectory() const
{
	return mLastDirectory;
}

void Options::setLastDirectory(const QString &dir)
{
	mLastDirectory = Utils::addTrailingSlash(dir);
}

QStringList Options::getRecentFiles() const
{
	return mRecentFiles;
}

void Options::setRecentFiles(const QStringList &list)
{
	mRecentFiles = list;
}
