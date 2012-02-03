#ifndef OPTIONS_H
#define OPTIONS_H

#include "singleton.h"

// Класс-синглетон с настройками редактора
class Options : public Singleton<Options>
{
	Q_OBJECT

public:

	// Конструктор
	Options(QSettings &settings);

	// Сохраняет текущие настройки
	void save(QSettings &settings);

	// Возвращает полный путь к каталогу данных
	QString getDataDirectory() const;

	// Устанавливает путь к каталогу данных
	void setDataDirectory(const QString &dir);

	// Возвращает путь к последнему рабочему каталогу
	QString getLastDirectory() const;

	// Устанавливает путь к последнему рабочему каталогу
	void setLastDirectory(const QString &dir);

	// Возвращает список последних файлов
	QStringList getRecentFiles() const;

	// Устанавливает список последних файлов
	void setRecentFiles(const QStringList &list);

private:

	QString     mDataDirectory;     // Полный путь к каталогу данных
	QString     mLastDirectory;     // Последний рабочий каталог
	QStringList mRecentFiles;       // Список последних файлов
};

#endif // OPTIONS_H
