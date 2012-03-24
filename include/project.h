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

	// Возвращает относительный путь к каталогу с локациями
	QString getLocationsDirectory() const;

	// Возвращает относительный путь к каталогу со спрайтами
	QString getSpritesDirectory() const;

	// Возвращает относительный путь к каталогу со шрифтами
	QString getFontsDirectory() const;

private:

	// Загружает файл проекта
	bool loadProjectFile(const QString &fileName);

	// Сохраняет файл проекта
	bool saveProjectFile(const QString &fileName);

	QString mFileName;              // Имя файла проекта
	QString mRootDirectory;         // Абсолютный путь к корневому каталогу проекта
	QString mLocationsDirectory;    // Относительный путь к каталогу с локациями
	QString mSpritesDirectory;      // Относительный путь к каталогу со спрайтами
	QString mFontsDirectory;        // Относительный путь к каталогу со шрифтами
};

#endif // PROJECT_H
