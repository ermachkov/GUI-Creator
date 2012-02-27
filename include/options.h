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

	// Возвращает флаг показа сетки
	bool isShowGrid() const;

	// Устанавливает флаг показа сетки
	void setShowGrid(bool showGrid);

	// Возвращает флаг отрисовки точек вместо линий сетки
	bool isShowDots() const;

	// Устанавливает флаг отрисовки точек вместо линий сетки
	void setShowDots(bool showDots);

	// Возвращает флаг привязки к сетке
	bool isSnapToGrid() const;

	// Устанавливает флаг привязки к сетке
	void setSnapToGrid(bool snapToGrid);

	// Возвращает флаг привязки только к видимым линиям сетки
	bool isSnapToVisibleLines() const;

	// Устанавливает флаг привязки только к видимым линиям сетки
	void setSnapToVisibleLines(bool snapToVisibleLines);

	// Возвращает шаг сетки
	int getGridSpacing() const;

	// Устанавливает шаг сетки
	void setGridSpacing(int gridSpacing);

	// Возвращает интервал между основными линиями сетки
	int getMajorLinesInterval() const;

	// Устанавливает интервал между основными линиями сетки
	void setMajorLinesInterval(int majorLinesInterval);

	// Возвращает флаг показа направляющих
	bool isShowGuides() const;

	// Устанавливает флаг показа направляющих
	void setShowGuides(bool showGuides);

	// Возвращает флаг привязки к направляющим
	bool isSnapToGuides() const;

	// Устанавливает флаг привязки к направляющим
	void setSnapToGuides(bool snapToGuides);

	// Возвращает флаг разрешения умных направляющих
	bool isEnableSmartGuides() const;

	// Устанавливает флаг разрешения умных направляющих
	void setEnableSmartGuides(bool enableSmartGuides);

private:

	QString     mDataDirectory;         // Полный путь к каталогу данных
	QString     mLastDirectory;         // Последний рабочий каталог
	QStringList mRecentFiles;           // Список последних файлов

	bool        mShowGrid;              // Флаг показа сетки
	bool        mShowDots;              // Флаг отрисовки точек вместо линий сетки
	bool        mSnapToGrid;            // Флаг привязки к сетке
	bool        mSnapToVisibleLines;    // Флаг привязки только к видимым линиям сетки
	int         mGridSpacing;           // Шаг сетки
	int         mMajorLinesInterval;    // Интервал между основными линиями сетки

	bool        mShowGuides;            // Флаг показа направляющих
	bool        mSnapToGuides;          // Флаг привязки к направляющим
	bool        mEnableSmartGuides;     // Флаг разрешения умных направляющих
};

#endif // OPTIONS_H
