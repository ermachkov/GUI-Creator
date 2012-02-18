#ifndef TEXTURE_H
#define TEXTURE_H

// Класс текстуры
class Texture
{
public:

	// Конструктор
	Texture(const QImage &image);

	// Деструктор
	~Texture();

	// Возвращает OpenGL идентификатор текстуры
	GLuint getHandle() const;

	// Возвращает размер текстуры
	QSize getSize() const;

	// Возвращает ширину текстуры
	int getWidth() const;

	// Возвращает высоту текстуры
	int getHeight() const;

	// Рисует текстуру
	void draw();

private:

	GLuint  mHandle;    // Идентификатор текстуры
	QSize   mSize;      // Размеры текстуры
};

#endif // TEXTURE_H
