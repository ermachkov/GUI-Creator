#ifndef SPRITE_H
#define SPRITE_H

#include "game_object.h"
#include "texture.h"

// Класс спрайта
class Sprite : public GameObject
{
public:

	// Конструктор
	Sprite();

	// Конструктор
	Sprite(const QString &name, int id, const QPointF &pos, const QString &fileName, Layer *parent = NULL);

	// Деструктор
	virtual ~Sprite();

	// Возвращает имя файла с текстурой
	QString getFileName() const;

	// Устанавливает имя файла с текстурой
	void setFileName(const QString &fileName);

	// Возвращает размер текстуры в пикселях
	QSizeF getTextureSize() const;

	// Возвращает флаг блокировки изменения размеров
	bool isSizeLocked() const;

	// Устанавливает флаг блокировки изменения размеров
	void setSizeLocked(bool locked);

	// Возвращает цвет спрайта
	QColor getColor() const;

	// Устанавливает цвет спрайта
	void setColor(const QColor &color);

	// Загружает объект из бинарного потока
	virtual bool load(QDataStream &stream);

	// Сохраняет объект в бинарный поток
	virtual bool save(QDataStream &stream);

	// Загружает объект из Lua скрипта
	virtual bool load(LuaScript &script);

	// Сохраняет объект в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Устанавливает текущий язык для объекта
	virtual void setCurrentLanguage(const QString &language);

	// Определяет, локализован ли объект для текущего языка
	virtual bool isLocalized() const;

	// Устанавливает/сбрасывает локализацию для текущего языка
	virtual void setLocalized(bool localized);

	// Дублирует объект
	virtual GameObject *duplicate(Layer *parent = NULL) const;

	// Возвращает список отсутствующих файлов в объекте
	virtual QStringList getMissedFiles() const;

	// Заменяет текстуру в объекте
	virtual bool changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Отрисовывает объект
	virtual void draw();

private:

	// Тип для списка локализованных текстур
	typedef QMap<QString, QSharedPointer<Texture> > TextureMap;

	// Загружает локализованные текстуры
	void loadTextures();

	QString                 mFileName;          // Имя файла с текстурой
	QSharedPointer<Texture> mTexture;           // Текстура спрайта
	bool                    mSizeLocked;        // Флаг блокировки изменения размеров
	QColor                  mColor;             // Цвет спрайта

	StringMap               mFileNameMap;       // Список локализованных имен файлов
	TextureMap              mTextureMap;        // Список локализованных текстур
	RealMap                 mTextureWidthMap;   // Список ширин локализованных текстур
	RealMap                 mTextureHeightMap;  // Список высот локализованных текстур
};

#endif // SPRITE_H
