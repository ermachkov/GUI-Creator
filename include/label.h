#ifndef LABEL_H
#define LABEL_H

#include "game_object.h"

// Класс текстовой надписи
class Label : public GameObject
{
public:

	// Конструктор
	Label();

	// Конструктор
	Label(const QString &name, const QPointF &pos, const QString &fileName, int size, Layer *parent = NULL);

	// Конструктор копирования
	Label(const Label &label);

	// Деструктор
	virtual ~Label();

	// Возвращает текст надписи
	QString getText() const;

	// Устанавливает текст надписи
	void setText(const QString &text);

	// Возвращает имя файла со шрифтом
	QString getFileName() const;

	// Устанавливает имя файла со шрифтом
	void setFileName(const QString &fileName);

	// Возвращает размер шрифта в пунктах
	int getFontSize() const;

	// Устанавливает размер шрифта в пунктах
	void setFontSize(int size);

	// Возвращает выравнивание текста
	FTGL::TextAlignment getAlignment() const;

	// Устанавливает выравнивание текста
	void setAlignment(FTGL::TextAlignment alignment);

	// Возвращает цвет текста
	QColor getColor() const;

	// Устанавливает цвет текста
	void setColor(const QColor &color);

	// Загружает объект из бинарного потока
	virtual bool load(QDataStream &stream);

	// Сохраняет объект в бинарный поток
	virtual bool save(QDataStream &stream);

	// Загружает объект из Lua скрипта
	virtual bool load(LuaScript &script);

	// Сохраняет объект в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Дублирует объект
	virtual GameObject *duplicate(Layer *parent = NULL) const;

	// Возвращает список отсутствующих файлов в объекте
	virtual QStringList getMissedFiles() const;

	// Заменяет текстуру в объекте
	virtual bool changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Отрисовывает объект
	virtual void draw();

private:

	QString                 mText;      // Текст надписи
	QString                 mFileName;  // Имя файла со шрифтом
	int                     mFontSize;  // Размер шрифта в пунктах
	QSharedPointer<FTFont>  mFont;      // Шрифт надписи
	FTGL::TextAlignment     mAlignment; // Выравнивание текста
	QColor                  mColor;     // Цвет текста
};

#endif // LABEL_H
