#ifndef TEXTURE_H
#define TEXTURE_H

// Класс текстуры
class Texture
{
public:

	// Конструктор
	Texture(const QImage &image, QGLWidget *widget);

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

	QGLWidget   *mGLWidget; // OpenGL виджет, используемый при удалении текстуры
	GLuint      mHandle;    // Идентификатор текстуры
	QSize       mSize;      // Размеры текстуры
};

#endif // TEXTURE_H
