#include "pch.h"
#include "thumbnail_loader.h"

ThumbnailLoader::ThumbnailLoader(QObject *parent)
: QThread(parent),
  mWakeSemaphore(0), // по умолчанию поток спит
  mQueueSemaphore(1) // по умолчанию доступ к очереди разрешен
{
	//moveToThread(this);

	mQuit = false;

	// старт потока
	start(LowPriority);
}

ThumbnailLoader::~ThumbnailLoader()
{
	// установка флага выхода фонового потока
	mQuit = true;

	// пробудить поток
	mWakeSemaphore.release();

	// ожидание завершения цикла нити
	wait();
}

// Добавление пути в список для загрузки и старт/пробуждение фонового потока
void ThumbnailLoader::addFileForLoading(const QString &absoluteFileName)
{
	mQueueSemaphore.acquire();
	mQueuePaths.push_back(absoluteFileName);
	mQueueSemaphore.release();

	// поток должен быть запущен
	if (!isRunning())
	{
		// TODO: выдать ошибку синхронизации потоков
		//start(LowPriority);
	}

	// если нить спит, то разбудить
	if (mWakeSemaphore.available() == 0)
		mWakeSemaphore.release();
}

// Отчистка списка файлов на загрузку
void ThumbnailLoader::clearQueueLoadingFiles()
{
	mQueueSemaphore.acquire();
	mQueuePaths.clear();
	mQueueSemaphore.release();
}


void ThumbnailLoader::run()
{
	// запуск бесконечного цикла нити
	while (!mQuit)
	{
		// попробовать захватить ресурс
		mWakeSemaphore.acquire();

		while (!mQuit)
		{
			// захватить ресурс - очередь
			mQueueSemaphore.acquire();

			if (mQueuePaths.empty())
			{
				// освободить ресурс - очередь
				mQueueSemaphore.release();
				break;
			}

			// забрать путь в изображению из очереди
			QString absoluteFileName = mQueuePaths.front();
			mQueuePaths.pop_front();

			// освободить ресурс - очередь
			mQueueSemaphore.release();

			// TODO:
			// проверка наличия иконки
			//..
			// TODO:
			// временная блокировка загружаемого файла
			//..

			// загрузка спрайта
			QImage image(absoluteFileName);

			// если изображение не загружено послать сигнал об ошибке
			if (image.isNull())
			{
				emit thumbnailNotLoaded(absoluteFileName);
				continue;
			}

			// пережатие спрайта
			image = image.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

			// центровка спрайта
			QImage centredImage(64, 64, QImage::Format_ARGB32_Premultiplied);
			centredImage.fill(qRgba(0, 0, 0, 0));
			QPainter painter(&centredImage);
			// рассчет смещения для создания центровки
			QPoint topLeft((centredImage.width() - image.width()) / 2, (centredImage.height() - image.height()) / 2);
			painter.drawImage(topLeft, image);
			// получение информации о файле
			QFileInfo fileInfo(absoluteFileName);

//			fileInfo.size();
//			fileInfo.

			// TODO:
			// разблокировка загружаемого файла
			//..

			emit thumbnailLoaded(absoluteFileName, centredImage, fileInfo);
		}
	}
}
