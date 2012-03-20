#ifndef LABEL_H
#define LABEL_H

#include "game_object.h"

// Класс текстовой надписи
class Label : public GameObject
{
public:

	// Тип для горизонтального выравнивания текста
	enum HorzAlignment
	{
		HORZ_ALIGN_LEFT,
		HORZ_ALIGN_CENTER,
		HORZ_ALIGN_RIGHT
	};

	// Тип для вертикального выравнивания текста
	enum VertAlignment
	{
		VERT_ALIGN_TOP,
		VERT_ALIGN_CENTER,
		VERT_ALIGN_BOTTOM
	};

	// Конструктор
	Label();

	// Конструктор
	Label(const QString &name, int id, const QPointF &pos, const QString &fileName, int size, Layer *parent = NULL);

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

	// Возвращает горизонтальное выравнивание текста
	HorzAlignment getHorzAlignment() const;

	// Устанавливает горизонтальное выравнивание текста
	void setHorzAlignment(HorzAlignment alignment);

	// Возвращает вертикальное выравнивание текста
	VertAlignment getVertAlignment() const;

	// Устанавливает вертикальное выравнивание текста
	void setVertAlignment(VertAlignment alignment);

	// Возвращает межстрочный интервал
	qreal getLineSpacing() const;

	// Устанавливает межстрочный интервал
	void setLineSpacing(qreal lineSpacing);

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

	QString                 mText;          // Текст надписи
	QString                 mFileName;      // Имя файла со шрифтом
	int                     mFontSize;      // Размер шрифта в пунктах
	QSharedPointer<FTFont>  mFont;          // Шрифт надписи
	HorzAlignment           mHorzAlignment; // Горизонтальное выравнивание текста
	VertAlignment           mVertAlignment; // Вертикальное выравнивание текста
	qreal                   mLineSpacing;   // Межстрочный интервал
	QColor                  mColor;         // Цвет текста
};

#endif // LABEL_H
