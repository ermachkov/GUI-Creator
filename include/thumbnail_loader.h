#ifndef THUMBNAIL_LOADER_H
#define THUMBNAIL_LOADER_H

// Класс для загрузки и создания миниатюр в фоновом потоке
class ThumbnailLoader : public QObject
{
	Q_OBJECT

public:

	// Конструктор
	ThumbnailLoader()
	: mThumbnailCount(0)
	{
	}

	// Устанавливает счетчик миниатюр
	void setThumbnailCount(int count)
	{
		mThumbnailCount = count;
	}

public slots:

	// Обработчик сигнала добавления изображения в очередь на загрузку
	void onThumbnailQueued(QString absoluteFileName, int count)
	{
		// если значения переданного и собственного счетчика совпадают, увеличиваем свой счетчик на единицу и загружаем спрайт
		if (mThumbnailCount.testAndSetOrdered(count, count + 1))
		{
			// загрузка спрайта
			QImage sourceImage(absoluteFileName);
			QImage centeredImage;
			if (!sourceImage.isNull())
			{
				// пережатие спрайта
				QImage scaledImage = sourceImage.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation);

				// создаем новую иконку и заливаем ее прозрачным черным цветом
				centeredImage = QImage(64, 64, QImage::Format_ARGB32_Premultiplied);
				centeredImage.fill(QColor(0, 0, 0, 0));

				// центровка спрайта
				QPainter painter(&centeredImage);
				QPoint topLeft((centeredImage.width() - scaledImage.width()) / 2, (centeredImage.height() - scaledImage.height()) / 2);
				painter.drawImage(topLeft, scaledImage);
			}

			// посылаем сигнал о завершении загрузки
			emit thumbnailLoaded(absoluteFileName, centeredImage);
		}
	}

signals:

	// Сигнал о завершении загрузки миниатюры
	void thumbnailLoaded(QString absoluteFileName, QImage image);

private:

	// Счетчик загруженных миниатюр
	QAtomicInt mThumbnailCount;
};

#endif // THUMBNAIL_LOADER_H
