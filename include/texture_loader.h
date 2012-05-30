#ifndef TEXTURE_LOADER_H
#define TEXTURE_LOADER_H

#include "project.h"
#include "texture.h"
#include "utils.h"

// Класс для загрузки текстур в фоновом потоке
class TextureLoader : public QObject
{
	Q_OBJECT

public:

	// Конструктор
	TextureLoader(QGLWidget *secondaryGLWidget)
	: mSecondaryGLWidget(secondaryGLWidget)
	{
	}

public slots:

	// Обработчик сигнала добавления текстуры в очередь на загрузку
	void onTextureQueued(QString fileName)
	{
		// загружаем текстуру и отправляем сигнал о завершении загрузки
		QSharedPointer<Texture> texture;
		QString path = Project::getSingleton().getRootDirectory() + fileName;
		if (Utils::fileExists(path))
		{
			mSecondaryGLWidget->makeCurrent();
			texture = QSharedPointer<Texture>(new Texture(path));
			if (!texture->isLoaded())
				texture.clear();
			mSecondaryGLWidget->doneCurrent();
		}
		emit textureLoaded(fileName, texture);
	}

signals:

	// Сигнал о завершении загрузки текстуры
	void textureLoaded(QString fileName, QSharedPointer<Texture> texture);

private:

	// OpenGL виджет для загрузки текстур в фоновом потоке
	QGLWidget *mSecondaryGLWidget;
};

#endif // TEXTURE_LOADER_H
