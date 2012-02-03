#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include "options.h"
#include "texture.h"

// Класс для загрузки текстур в фоновом потоке
class TextureLoader : public QObject
{
	Q_OBJECT

public:

	// Конструктор
	TextureLoader(QGLWidget *primaryGLWidget, QGLWidget *secondaryGLWidget)
	: mPrimaryGLWidget(primaryGLWidget), mSecondaryGLWidget(secondaryGLWidget)
	{
	}

public slots:

	// Обработчик сигнала добавления текстуры в очередь на загрузку
	void onTextureQueued(QString fileName)
	{
		// загружаем текстуру и отправляем сигнал о завершении загрузки
		QSharedPointer<Texture> texture;
		QImage image(Options::getSingleton().getDataDirectory() + fileName);
		if (!image.isNull())
		{
			mSecondaryGLWidget->makeCurrent();
			texture = QSharedPointer<Texture>(new Texture(image, mPrimaryGLWidget));
			mSecondaryGLWidget->doneCurrent();
		}
		emit textureLoaded(fileName, texture);
	}

signals:

	// Сигнал о завершении загрузки текстуры
	void textureLoaded(QString fileName, QSharedPointer<Texture> texture);

private:

	QGLWidget   *mPrimaryGLWidget;   // OpenGL виджет для загрузки текстур в главном потоке
	QGLWidget   *mSecondaryGLWidget; // OpenGL виджет для загрузки текстур в фоновом потоке
};

#endif // TEXTURE_LOADER_H
