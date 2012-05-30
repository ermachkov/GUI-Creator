#include "pch.h"
#include "font_manager.h"
#include "project.h"
#include "utils.h"

template<> FontManager *Singleton<FontManager>::mSingleton = NULL;

FontManager::FontManager(QGLWidget *primaryGLWidget)
: mPrimaryGLWidget(primaryGLWidget)
{
}

QSharedPointer<Font> FontManager::loadFont(const QString &fileName, int size, bool useDefaultFont)
{
	// сначала ищем шрифт в кэше
	for (FontCache::iterator it = mFontCache.begin(); it != mFontCache.end(); ++it)
		if (it->mFileName == fileName && it->mSize == size)
		{
			QSharedPointer<Font> font = it->mFont.toStrongRef();
			if (!font.isNull())
				return font;
			mFontCache.erase(it);
			break;
		}

	// не нашли в кэше - загружаем шрифт из файла
	QSharedPointer<Font> font;
	QString path = Project::getSingleton().getRootDirectory() + fileName;
	mPrimaryGLWidget->makeCurrent();
	if (Utils::fileExists(path) && (font = QSharedPointer<Font>(new Font(path, size)))->isLoaded())
	{
		// добавляем шрифт в кэш
		mFontCache.push_back(FontInfo(fileName, size, font));
	}
	else if (useDefaultFont)
	{
		// возвращаем шрифт по умолчанию
		font = QSharedPointer<Font>(new Font());
		mFontCache.push_back(FontInfo(fileName, size, font));
	}
	else
	{
		// очищаем указатель на шрифт в случае ошибки
		font.clear();
	}

	return font;
}

void FontManager::makeCurrent()
{
	mPrimaryGLWidget->makeCurrent();
}
