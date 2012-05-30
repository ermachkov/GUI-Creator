#ifndef TEXTURE_H
#define TEXTURE_H

// Класс текстуры
class Texture
{
public:

	// Конструктор
	Texture();

	// Конструктор
	Texture(const QString &fileName);

	// Деструктор
	~Texture();

	// Проверяет, что текстура загружена
	bool isLoaded() const;

	// Проверяет, что текстура дефолтная
	bool isDefault() const;

	// Возвращает размер текстуры
	QSize getSize() const;

	// Возвращает ширину текстуры
	int getWidth() const;

	// Возвращает высоту текстуры
	int getHeight() const;

	// Рисует текстуру
	void draw();

private:

	// Загружает текстуру из файла
	void load(const QString &fileName);

	GLuint  mHandle;    // Идентификатор текстуры
	QSize   mSize;      // Размеры текстуры
	bool    mDefault;   // Флаг текстуры по умолчанию
};

#endif // TEXTURE_H
