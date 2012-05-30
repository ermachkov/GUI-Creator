#include "pch.h"
#include "texture.h"

Texture::Texture()
: mHandle(0), mDefault(true)
{
	load(":/images/default_texture.jpg");
}

Texture::Texture(const QString &fileName)
: mHandle(0), mDefault(false)
{
	load(fileName);
}

Texture::~Texture()
{
	if (mHandle != 0)
		glDeleteTextures(1, &mHandle);
}

bool Texture::isLoaded() const
{
	return mHandle != 0;
}

bool Texture::isDefault() const
{
	return mDefault;
}

QSize Texture::getSize() const
{
	return mSize;
}

int Texture::getWidth() const
{
	return mSize.width();
}

int Texture::getHeight() const
{
	return mSize.height();
}

void Texture::draw()
{
	glBindTexture(GL_TEXTURE_2D, mHandle);
	glBegin(GL_TRIANGLE_STRIP);
	glTexCoord2d(0.0, 0.0);
	glVertex2d(0.0, mSize.height());
	glTexCoord2d(1.0, 0.0);
	glVertex2d(mSize.width(), mSize.height());
	glTexCoord2d(0.0, 1.0);
	glVertex2d(0.0, 0.0);
	glTexCoord2d(1.0, 1.0);
	glVertex2d(mSize.width(), 0.0);
	glEnd();
}

void Texture::load(const QString &fileName)
{
	// загружаем изображение из файла
	QImage image(fileName);
	if (!image.isNull())
	{
		// конвертируем изображение в формат, подходящий для OpenGL
		QImage GLImage = QGLWidget::convertToGLFormat(image);
		mSize = GLImage.size();

		// создаем текстуру
		glGenTextures(1, &mHandle);
		glBindTexture(GL_TEXTURE_2D, mHandle);
		glTexImage2D(GL_TEXTURE_2D, 0, 4, GLImage.width(), GLImage.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, GLImage.bits());

		// настраиваем параметры текстуры
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}
