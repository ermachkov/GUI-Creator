#ifndef THUMBNAIL_LOADER_H
#define THUMBNAIL_LOADER_H

class ThumbnailLoader : public QThread
{
	Q_OBJECT

public:
	explicit ThumbnailLoader(QObject *parent = NULL);
	virtual ~ThumbnailLoader();

	// Добавление пути в список для загрузки и старт/пробуждение фонового потока
	void addFileForLoading(const QString &absoluteFileName);

	// Отчистка списка файлов на загрузку
	void clearQueueLoadingFiles();

signals:
	// сигнал об окончании загрузки и преобразования картинки в иконку
	void thumbnailLoaded(QString absolutePath, QImage image, QFileInfo fileInfo);

	// сигнал об ошибке загрузки картинки для иконки
	void thumbnailNotLoaded(QString absolutePath);

protected:
	virtual void run();

private:
	// семафор для пробуждения нити
	QSemaphore mWakeSemaphore;

	// семафор для защиты очереди
	QSemaphore mQueueSemaphore;

	// очередь на загрузку изображений для иконок
	QQueue<QString> mQueuePaths;

	QAtomicInt mQuit;
};

#endif // THUMBNAIL_LOADER_H
