#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include "singleton.h"

// Глобальный класс для создания и хранения шрифтов
class FontManager : public Singleton<FontManager>
{
	Q_OBJECT

public:

	// Конструктор
	FontManager(QGLWidget *primaryGLWidget);

	// Возвращает шрифт по умолчанию
	QSharedPointer<FTFont> getDefaultFont() const;

	// Загружает шрифт и возвращает указатель на него
	QSharedPointer<FTFont> loadFont(const QString &fileName, int size, bool useDefaultFont = true);

	// Устанавливает текущий контекст OpenGL
	void makeCurrent();

private:

	// Структура с информацией о шрифте
	struct FontInfo
	{
		// Конструктор
		FontInfo(const QString &fileName, int size, const QSharedPointer<FTFont> &font)
		: mFileName(fileName), mSize(size), mFont(font)
		{
		}

		QString                 mFileName;  // Имя файла со шрифтом
		int                     mSize;      // Размер шрифта в пунктах
		QWeakPointer<FTFont>    mFont;      // Слабая ссылка на объект шрифта
	};

	// Тип для шрифтового кэша
	typedef QList<FontInfo> FontCache;

	QGLWidget               *mPrimaryGLWidget;  // OpenGL виджет для загрузки текстур в главном потоке
	QSharedPointer<FTFont>  mDefaultFont;       // Шрифт по умолчанию
	QByteArray              mDefaultFontBuffer; // Буфер в памяти для шрифта по умолчанию
	FontCache               mFontCache;         // Шрифтовый кэш
};

#endif // FONT_MANAGER_H
