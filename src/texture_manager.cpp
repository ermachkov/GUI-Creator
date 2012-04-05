#include "pch.h"
#include "texture_manager.h"
#include "texture_loader.h"

template<> TextureManager *Singleton<TextureManager>::mSingleton = NULL;

TextureManager::TextureManager(QGLWidget *primaryGLWidget, QGLWidget *secondaryGLWidget)
: mPrimaryGLWidget(primaryGLWidget)
{
	// создаем фоновый поток
	mBackgroundThread = new QThread(this);

	// создаем загрузчик текстур и переносим его в фоновый поток
	mTextureLoader = new TextureLoader(secondaryGLWidget);
	qRegisterMetaType<QSharedPointer<Texture> >("QSharedPointer<Texture>");
	connect(this, SIGNAL(textureQueued(QString)), mTextureLoader, SLOT(onTextureQueued(QString)));
	connect(mTextureLoader, SIGNAL(textureLoaded(QString, QSharedPointer<Texture>)), this, SLOT(onTextureLoaded(QString, QSharedPointer<Texture>)));
	mTextureLoader->moveToThread(mBackgroundThread);

	// создаем объект слежения за файловой системой
	mWatcher = new QFileSystemWatcher(this);
	connect(mWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(onFileChanged(const QString &)));

	// загружаем текстуру по умолчанию
	mPrimaryGLWidget->makeCurrent();
	mDefaultTexture = QSharedPointer<Texture>(new Texture(QImage(":/images/default_texture.jpg")));

	// запускаем фоновый поток
	mBackgroundThread->start();

	// запускаем таймер для отслеживания изменений файлов
	startTimer(250);
}

TextureManager::~TextureManager()
{
	// дожидаемся завершения фонового потока
	mBackgroundThread->quit();
	mBackgroundThread->wait();

	// удаляем загрузчик текстур
	delete mTextureLoader;
}

QSharedPointer<Texture> TextureManager::getDefaultTexture() const
{
	return mDefaultTexture;
}

QSharedPointer<Texture> TextureManager::loadTexture(const QString &fileName, bool useDefaultTexture)
{
	// сначала ищем текстуру в кэше
	TextureCache::const_iterator it = mTextureCache.find(fileName);
	if (it != mTextureCache.end())
	{
		QSharedPointer<Texture> texture = it->mTexture.toStrongRef();
		if (!texture.isNull())
			return texture;
	}

	// не нашли в кэше - загружаем текстуру из файла
	QSharedPointer<Texture> texture;
	QImage image;
	QString path = Project::getSingleton().getRootDirectory() + fileName;
	if (Utils::fileExists(path) && !(image = QImage(path)).isNull())
	{
		// добавляем файл на слежение
		if (!mWatcher->files().contains(path))
			mWatcher->addPath(path);

		// создаем текстуру и добавляем ее в кэш
		mPrimaryGLWidget->makeCurrent();
		texture = QSharedPointer<Texture>(new Texture(image));
		mTextureCache.insert(fileName, TextureInfo(texture));
	}
	else if (useDefaultTexture)
	{
		// возвращаем текстуру по умолчанию
		texture = mDefaultTexture;
		mTextureCache.insert(fileName, TextureInfo(texture));
	}

	return texture;
}

void TextureManager::makeCurrent()
{
	mPrimaryGLWidget->makeCurrent();
}

void TextureManager::timerEvent(QTimerEvent *event)
{
	// обновляем состояние всех текстур в кэше
	TextureCache::iterator it = mTextureCache.begin();
	while (it != mTextureCache.end())
	{
		QString path = Project::getSingleton().getRootDirectory() + it.key();
		if (!it->mTexture.isNull())
		{
			if (it->mTexture == mDefaultTexture)
			{
				// если файл текстуры был удален, а потом восстановлен, отправляем текстуру на загрузку
				if (Utils::fileExists(path))
					emit textureQueued(it.key());
			}
			else if (it->mChanged)
			{
				// если с момента последнего изменения текстуры прошло достаточно много времени, отправляем ее на загрузку
				if (it->mTimer.hasExpired(250))
				{
					it->mChanged = false;
					if (Utils::fileExists(path))
						emit textureQueued(it.key());
					else
						onTextureLoaded(it.key(), QSharedPointer<Texture>());
				}
			}
			++it;
		}
		else
		{
			// удаляем неиспользуемую текстуру из кэша
			if (mWatcher->files().contains(path))
				mWatcher->removePath(path);
			mTextureCache.erase(it++);
		}
	}
}

void TextureManager::onFileChanged(const QString &path)
{
	// проверяем, используется ли текстура игровыми объектами
	QString fileName = path.mid(Project::getSingleton().getRootDirectory().size());
	TextureCache::iterator it = mTextureCache.find(fileName);
	if (it != mTextureCache.end() && !it->mTexture.isNull())
	{
		// помечаем текстуру как измененную
		it->mChanged = true;
		it->mTimer.start();
	}
}

void TextureManager::onTextureLoaded(QString fileName, QSharedPointer<Texture> texture)
{
	// проверяем, используется ли текстура игровыми объектами
	TextureCache::iterator it = mTextureCache.find(fileName);
	if (it != mTextureCache.end() && !it->mTexture.isNull())
	{
		// добавляем файл на слежение
		if (!texture.isNull())
		{
			QString path = Project::getSingleton().getRootDirectory() + fileName;
			if (!mWatcher->files().contains(path))
				mWatcher->addPath(path);
		}

		// выдаем сигнал об изменении текстуры и заменяем ее в кэше
		QSharedPointer<Texture> &newTexture = !texture.isNull() ? texture : mDefaultTexture;
		emit textureChanged(fileName, newTexture);
		Q_ASSERT(it->mTexture.isNull() || it->mTexture == mDefaultTexture);
		it->mTexture = newTexture;
	}
}
