#include "pch.h"
#include "project.h"
#include "lua_script.h"
#include "utils.h"

template<> Project *Singleton<Project>::mSingleton = NULL;

Project::Project()
{
	close();
}

bool Project::open(const QString &fileName)
{
	// закрываем проект, если он был ранее открыт
	close();

	// загружаем файл проекта
	if (!loadProjectFile(fileName))
	{
		close();
		return false;
	}

	return true;
}

void Project::close()
{
	// устанавливаем значения по умолчанию
	mFileName = "";
	mRootDirectory = Utils::addTrailingSlash(QDir::currentPath());
	mLocationsDirectory = "levels/";
	mSpritesDirectory = "sprites/";
	mFontsDirectory = "fonts/";
}

bool Project::isOpen() const
{
	return !mFileName.isEmpty();
}

bool Project::save()
{
	return !mFileName.isEmpty() ? saveProjectFile(mFileName) : false;
}

QString Project::getRootDirectory() const
{
	return mRootDirectory;
}

QString Project::getLocationsDirectory() const
{
	return mLocationsDirectory;
}

QString Project::getSpritesDirectory() const
{
	return mSpritesDirectory;
}

QString Project::getFontsDirectory() const
{
	return mFontsDirectory;
}

bool Project::loadProjectFile(const QString &fileName)
{
	// сохраняем имя файла проекта
	mFileName = fileName;

	// определяем путь к корневому каталогу
	mRootDirectory = Utils::addTrailingSlash(QFileInfo(mFileName).canonicalPath());

	// загружаем Lua скрипт
	LuaScript script;
	if (!script.load(mFileName))
		return false;

	// читаем корневую таблицу
	if (!script.pushTable(Utils::convertToCamelCase(QFileInfo(mFileName).baseName())))
		return false;

	// загружаем относительные пути к ресурсным каталогам
	if (!script.getString("locationsDirectory", mLocationsDirectory) || !script.getString("spritesDirectory", mSpritesDirectory)
		|| !script.getString("fontsDirectory", mFontsDirectory))
		return false;
	mLocationsDirectory = Utils::addTrailingSlash(mLocationsDirectory);
	mSpritesDirectory = Utils::addTrailingSlash(mSpritesDirectory);
	mFontsDirectory = Utils::addTrailingSlash(mFontsDirectory);

	// извлекаем из стека корневую таблицу
	script.popTable();

	return true;
}

bool Project::saveProjectFile(const QString &fileName)
{
	// открываем файл
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	// создаем текстовый поток
	QTextStream stream(&file);

	// записываем шапку файла
	Utils::writeFileHeader(stream);

	// записываем корневую таблицу
	stream << endl << "-- Project root table. Do not declare global variables with the same name!" << endl;
	stream << Utils::convertToCamelCase(QFileInfo(fileName).baseName()) << " =" << endl;
	stream << "{" << endl;

	// записываем относительные пути к ресурсным каталогам
	stream << "\t-- Paths to resource directories relative to the project root directory" << endl;
	stream << "\tlocationsDirectory = \"" << Utils::insertBackslashes(mLocationsDirectory) << "\"," << endl;
	stream << "\tspritesDirectory = \"" << Utils::insertBackslashes(mSpritesDirectory) << "\"," << endl;
	stream << "\tfontsDirectory = \"" << Utils::insertBackslashes(mFontsDirectory) << "\"" << endl;

	// записываем закрывающую фигурную скобку корневой таблицы
	stream << "}" << endl;

	return stream.status() == QTextStream::Ok;
}
