#include "pch.h"
#include "font_manager.h"
#include "options.h"
#include "utils.h"

template<> FontManager *Singleton<FontManager>::mSingleton = NULL;

FontManager::FontManager()
{
	// загружаем шрифт по умолчанию
	QFile file(":/default_font.ttf");
	file.open(QIODevice::ReadOnly);
	QByteArray buf = file.readAll();

	mDefaultFont = QSharedPointer<FTFont>(new FTTextureFont(reinterpret_cast<unsigned char *>(buf.data()), buf.size()));
	mDefaultFont->FaceSize(16);
	mDefaultFont->CharMap(FT_ENCODING_UNICODE);
}

QSharedPointer<FTFont> FontManager::getDefaultFont() const
{
	return mDefaultFont;
}

QSharedPointer<FTFont> FontManager::loadFont(const QString &fileName, int size, bool useDefaultFont)
{
	// сначала ищем шрифт в кэше
	for (FontCache::iterator it = mFontCache.begin(); it != mFontCache.end(); ++it)
		if (it->mFileName == fileName && it->mSize == size)
		{
			QSharedPointer<FTFont> font = it->mFont.toStrongRef();
			if (!font.isNull())
				return font;
			mFontCache.erase(it);
			break;
		}

	// не нашли в кэше - загружаем шрифт из файла
	QSharedPointer<FTFont> font;
	QString path = Options::getSingleton().getDataDirectory() + fileName;
	if (Utils::fileExists(path) && (font = QSharedPointer<FTFont>(new FTTextureFont(path.toAscii().data())))->Error() == 0)
	{
		// настраиваем загруженный шрифт и добавляем его в кэш
		font->FaceSize(size);
		font->CharMap(FT_ENCODING_UNICODE);
		mFontCache.push_back(FontInfo(fileName, size, font));
	}
	else if (useDefaultFont)
	{
		// возвращаем шрифт по умолчанию
		font = mDefaultFont;
		mFontCache.push_back(FontInfo(fileName, size, font));
	}

	return font;
}
