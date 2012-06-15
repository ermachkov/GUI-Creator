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
	mScenesDirectory = "scenes/";
	mLocalizationDirectory = "l10n/";
	mNamesDirectory = "names/";
	mSpritesDirectory = "sprites/";
	mFontsDirectory = "fonts/";

	mLanguages = QStringList("en");
	mLanguageNames = QStringList("&Английский");
	mDefaultLanguage = mCurrentLanguage = "en";
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

QString Project::getScenesDirectory() const
{
	return mScenesDirectory;
}

QString Project::getLocalizationDirectory() const
{
	return mLocalizationDirectory;
}

QString Project::getNamesDirectory() const
{
	return mNamesDirectory;
}

QString Project::getSpritesDirectory() const
{
	return mSpritesDirectory;
}

QString Project::getFontsDirectory() const
{
	return mFontsDirectory;
}

QStringList Project::getLanguages() const
{
	return mLanguages;
}

QStringList Project::getLanguageNames() const
{
	return mLanguageNames;
}

QString Project::getDefaultLanguage() const
{
	return mDefaultLanguage;
}

QString Project::getCurrentLanguage() const
{
	return mCurrentLanguage;
}

void Project::setCurrentLanguage(const QString &language)
{
	mCurrentLanguage = language;
}

bool Project::loadProjectFile(const QString &fileName)
{
	// сохраняем имя файла проекта
	mFileName = fileName;

	// определяем путь к корневому каталогу
	mRootDirectory = Utils::addTrailingSlash(QFileInfo(mFileName).canonicalPath());

	// загружаем Lua скрипт и проверяем, что он вернул корневую таблицу
	LuaScript script;
	if (!script.load(mFileName, 1) || !script.pushTable())
		return false;

	// загружаем относительные пути к ресурсным каталогам
	if (!script.getString("scenesDirectory", mScenesDirectory) || !script.getString("localizationDirectory", mLocalizationDirectory)
		|| !script.getString("namesDirectory", mNamesDirectory) || !script.getString("spritesDirectory", mSpritesDirectory)
		|| !script.getString("fontsDirectory", mFontsDirectory))
		return false;
	mScenesDirectory = Utils::addTrailingSlash(mScenesDirectory);
	mLocalizationDirectory = Utils::addTrailingSlash(mLocalizationDirectory);
	mNamesDirectory = Utils::addTrailingSlash(mNamesDirectory);
	mSpritesDirectory = Utils::addTrailingSlash(mSpritesDirectory);
	mFontsDirectory = Utils::addTrailingSlash(mFontsDirectory);

	// загружаем список языков
	int length = 0;
	if (!script.pushTable("languages") || (length = script.getLength()) == 0)
		return false;

	mLanguages.clear();
	for (int i = 1; i <= length; ++i)
	{
		QString str;
		if (!script.getString(i, str))
			return false;
		mLanguages.push_back(str);
	}

	script.popTable();

	// загружаем список названий языков
	if (!script.pushTable("languageNames") || script.getLength() != length)
		return false;

	mLanguageNames.clear();
	for (int i = 1; i <= length; ++i)
	{
		QString str;
		if (!script.getString(i, str))
			return false;
		mLanguageNames.push_back(str);
	}

	script.popTable();

	// загружаем язык по умолчанию и текущий выбранный язык
	if (!script.getString("defaultLanguage", mDefaultLanguage) || !mLanguages.contains(mDefaultLanguage)
		|| !script.getString("currentLanguage", mCurrentLanguage) || !mLanguages.contains(mCurrentLanguage))
		return false;

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

	// записываем код для возврата корневой таблицы
	stream << endl << "return" << endl;
	stream << "{" << endl;

	// записываем относительные пути к ресурсным каталогам
	stream << "\t-- Paths to resource directories relative to the project root directory" << endl;
	stream << "\tscenesDirectory = " << Utils::quotify(mScenesDirectory) << "," << endl;
	stream << "\tlocalizationDirectory = " << Utils::quotify(mLocalizationDirectory) << "," << endl;
	stream << "\tnamesDirectory = " << Utils::quotify(mNamesDirectory) << "," << endl;
	stream << "\tspritesDirectory = " << Utils::quotify(mSpritesDirectory) << "," << endl;
	stream << "\tfontsDirectory = " << Utils::quotify(mFontsDirectory) << "," << endl;

	// записываем список языков
	stream << endl << "\t-- List of all available languages" << endl;
	stream << "\tlanguages = {";
	for (int i = 0; i < mLanguages.size(); ++i)
		stream << Utils::quotify(mLanguages[i]) << (i < mLanguages.size() - 1 ? ", " : "");
	stream << "}," << endl;

	// записываем список названий языков
	stream << "\tlanguageNames = {";
	for (int i = 0; i < mLanguageNames.size(); ++i)
		stream << Utils::quotify(mLanguageNames[i]) << (i < mLanguageNames.size() - 1 ? ", " : "");
	stream << "}," << endl;

	// записываем язык по умолчанию и текущий выбранный язык
	stream << "\tdefaultLanguage = " << Utils::quotify(mDefaultLanguage) << "," << endl;
	stream << "\tcurrentLanguage = " << Utils::quotify(mCurrentLanguage) << endl;

	// записываем закрывающую фигурную скобку корневой таблицы
	stream << "}" << endl;

	return stream.status() == QTextStream::Ok;
}
