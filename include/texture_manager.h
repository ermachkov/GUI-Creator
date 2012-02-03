#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "singleton.h"
#include "texture.h"

class TextureLoader;

// Глобальный класс для загрузки и хранения текстур
class TextureManager : public Singleton<TextureManager>
{
	Q_OBJECT

public:

	// Конструктор
	TextureManager(QGLWidget *primaryGLWidget, QGLWidget *secondaryGLWidget);

	// Деструктор
	virtual ~TextureManager();

	// Возвращает указатель на текстуру по умолчанию
	QSharedPointer<Texture> getDefaultTexture() const;

	// Загружает текстуру и возвращает указатель на нее
	QSharedPointer<Texture> loadTexture(const QString &fileName, bool useDefaultTexture = true);

signals:

	// Сигнал изменения текстуры
	void textureChanged(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Сигнал добавления текстуры в очередь на загрузку
	void textureQueued(QString fileName);

protected:

	// Вызывается по срабатыванию таймера
	virtual void timerEvent(QTimerEvent *event);

private slots:

	// Обработчик изменения файла
	void onFileChanged(const QString &path);

	// Обработчик завершения загрузки текстуры
	void onTextureLoaded(QString fileName, QSharedPointer<Texture> texture);

private:

	// Структура с информацией о текстуре
	struct TextureInfo
	{
		// Конструктор
		TextureInfo(const QSharedPointer<Texture> &texture)
		: mTexture(texture), mChanged(false)
		{
		}

		QWeakPointer<Texture>   mTexture;       // Слабая ссылка на загруженную текстуру
		bool                    mChanged;       // Флаг изменения текстуры
		QElapsedTimer           mTimer;         // Таймер для отсчета времени с момента последнего изменения текстуры
	};

	// Тип для текстурного кэша
	typedef QMap<QString, TextureInfo> TextureCache;

	QGLWidget               *mPrimaryGLWidget;      // OpenGL виджет для загрузки текстур в главном потоке
	QThread                 *mBackgroundThread;     // Фоновый поток
	TextureLoader           *mTextureLoader;        // Загрузчик текстур
	QFileSystemWatcher      *mWatcher;              // Объект слежения за файловой системой
	QSharedPointer<Texture> mDefaultTexture;        // Текстура по умолчанию
	TextureCache            mTextureCache;          // Текстурный кэш
};

#endif // TEXTURE_MANAGER_H
