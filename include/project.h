#ifndef PROJECT_H
#define PROJECT_H

#include "singleton.h"

// Глобальный класс проекта
class Project : public Singleton<Project>
{
	Q_OBJECT

public:

	// Конструктор
	Project();

	// Открывает проект
	bool open(const QString &fileName);

	// Закрывает проект
	void close();

	// Проверяет, был ли проект открыт
	bool isOpen() const;

	// Сохраняет проект
	bool save();

	// Возвращает абсолютный путь к корневому каталогу проекта
	QString getRootDirectory() const;

	// Возвращает относительный путь к каталогу со сценами
	QString getScenesDirectory() const;

	// Возвращает относительный путь к каталогу с переводами
	QString getLocalizationDirectory() const;

	// Возвращает относительный путь к каталогу со спрайтами
	QString getSpritesDirectory() const;

	// Возвращает относительный путь к каталогу со шрифтами
	QString getFontsDirectory() const;

	// Возвращает список двухбуквенных кодов доступных языков
	QStringList getLanguages() const;

	// Возвращает список названий доступных языков
	QStringList getLanguageNames() const;

	// Возвращает код языка по умолчанию
	QString getDefaultLanguage() const;

	// Возвращает код текущего выбранного языка
	QString getCurrentLanguage() const;

	// Устанавливает код текущего выбранного языка
	void setCurrentLanguage(const QString &language);

private:

	// Загружает файл проекта
	bool loadProjectFile(const QString &fileName);

	// Сохраняет файл проекта
	bool saveProjectFile(const QString &fileName);

	QString     mFileName;              // Имя файла проекта
	QString     mRootDirectory;         // Абсолютный путь к корневому каталогу проекта
	QString     mScenesDirectory;       // Относительный путь к каталогу со сценами
	QString     mLocalizationDirectory; // Относительный путь к каталогу с переводами
	QString     mSpritesDirectory;      // Относительный путь к каталогу со спрайтами
	QString     mFontsDirectory;        // Относительный путь к каталогу со шрифтами

	QStringList mLanguages;             // Список двухбуквенных кодов доступных языков
	QStringList mLanguageNames;         // Список названий доступных языков
	QString     mDefaultLanguage;       // Код языка по умолчанию
	QString     mCurrentLanguage;       // Код текущего выбранного языка
};

#endif // PROJECT_H
