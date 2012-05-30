#include "pch.h"
#include "font.h"
#include "utils.h"

Font::Font()
: mFont(NULL), mDefault(true)
{
	load(":/default_font.ttf", 32);
}

Font::Font(const QString &fileName, int size)
: mFont(NULL), mDefault(false)
{
	load(fileName, size);
}

Font::~Font()
{
	delete mFont;
}

bool Font::isLoaded() const
{
	return mFont != NULL && mFont->Error() == 0;
}

bool Font::isDefault() const
{
	return mDefault;
}

qreal Font::getWidth(const QString &text) const
{
	return mFont->Advance(Utils::toStdWString(text).c_str());
}

qreal Font::getHeight() const
{
	return mHeight;
}

void Font::draw(const QString &text)
{
	mFont->Render(Utils::toStdWString(text).c_str());
}

void Font::load(const QString &fileName, int size)
{
	// загружаем файл шрифта в буфер
	QFile file(fileName);
	file.open(QIODevice::ReadOnly);
	mFontBuffer = file.readAll();
	if (mFontBuffer.isEmpty())
		return;

	// создаем и настраиваем шрифт
	mFont = new FTTextureFont(reinterpret_cast<unsigned char *>(mFontBuffer.data()), mFontBuffer.size());
	if (mFont->Error() != 0)
		return;
	mFont->GlyphLoadFlags(FT_LOAD_DEFAULT);
	mFont->FaceSize(size);
	mFont->CharMap(FT_ENCODING_UNICODE);

	// используем FreeType для получения правильной высоты шрифта, т.к. FTGL использует свои формулы на основе баундинг ректа
	FT_Library library;
	FT_Face face;
	FT_Init_FreeType(&library);
	FT_New_Memory_Face(library, reinterpret_cast<unsigned char *>(mFontBuffer.data()), mFontBuffer.size(), 0, &face);
	FT_Set_Char_Size(face, 0, size * 64, 72, 72);
	mHeight = face->size->metrics.height / 64.0;
	FT_Done_Face(face);
	FT_Done_FreeType(library);
}
