#ifndef FONT_H
#define FONT_H

// Класс шрифта
class Font
{
public:

	// Конструктор
	Font();

	// Конструктор
	Font(const QString &fileName, int size);

	// Деструктор
	~Font();

	// Проверяет, что шрифт загружен
	bool isLoaded() const;

	// Проверяет, что шрифт дефолтный
	bool isDefault() const;

	// Возвращает ширину заданной строки
	qreal getWidth(const QString &text) const;

	// Возвращает высоту шрифта
	qreal getHeight() const;

	// Рисует однострочный текст
	void draw(const QString &text);

private:

	// Загружает шрифт из файла
	void load(const QString &fileName, int size);

	FTFont      *mFont;         // Указатель на объект шрифта
	QByteArray  mFontBuffer;    // Буфер в памяти для шрифта
	qreal       mHeight;        // Высота шрифта в пикселях, полученная от FreeType
	bool        mDefault;       // Флаг шрифта по умолчанию
};

#endif // FONT_H
