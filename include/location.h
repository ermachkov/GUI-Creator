#ifndef LOCATION_H
#define LOCATION_H

class BaseLayer;
class GameObject;
class Layer;

// Класс игровой локации
class Location : public QObject
{
	Q_OBJECT

public:

	// Конструктор
	Location(QObject *parent);

	// Деструктор
	virtual ~Location();

	// Загружает локацию из файла
	bool load(const QString &fileName);

	// Сохраняет локацию в файл
	bool save(const QString &fileName);

	// Загружает файл переводов
	bool loadTranslationFile(const QString &fileName);

	// Возвращает корневой слой
	BaseLayer *getRootLayer() const;

	// Возвращает текущий активный слой
	BaseLayer *getActiveLayer() const;

	// Устанавливает текущий активный слой
	void setActiveLayer(BaseLayer *layer);

	// Возвращает доступный активный слой для добавления/вставки объектов
	Layer *getAvailableLayer() const;

	// Создает новый слой
	BaseLayer *createLayer(BaseLayer *parent = NULL, int index = 0);

	// Создает новую группу слоев
	BaseLayer *createLayerGroup(BaseLayer *parent = NULL, int index = 0);

	// Создает новый спрайт
	GameObject *createSprite(const QPointF &pos, const QString &fileName);

	// Создает новую надпись
	GameObject *createLabel(const QPointF &pos, const QString &fileName, int size);

	// Создает и загружает игровой объект из бинарного потока
	GameObject *loadGameObject(QDataStream &stream);

	// Генерирует имя для копии объекта
	QString generateDuplicateName(const QString &name) const;

	// Генерирует идентификатор для копии объекта
	int generateDuplicateObjectID();

	// Возвращает количество направляющих
	int getNumGuides(bool horz) const;

	// Возвращает координату направляющей
	qreal getGuide(bool horz, int index) const;

	// Устанавливает координату направляющей
	void setGuide(bool horz, int index, qreal coord);

	// Ищет направляющую по координате
	int findGuide(bool horz, qreal coord, qreal &distance) const;

	// Добавляет новую направляющую
	int addGuide(bool horz, qreal coord);

	// Удаляет направляющую
	void removeGuide(bool horz, int index);

private:

	BaseLayer       *mRootLayer;        // Корневой слой
	BaseLayer       *mActiveLayer;      // Текущий активный слой

	QList<qreal>    mHorzGuides;        // Горизонтальные направляющие
	QList<qreal>    mVertGuides;        // Вертикальные направляющие

	int             mObjectIndex;       // Текущий индекс для генерации уникальных идентификаторов объектов
	int             mLayerIndex;        // Текущий индекс для генерации имен слоев
	int             mLayerGroupIndex;   // Текущий индекс для генерации имен групп слоев
	int             mSpriteIndex;       // Текущий индекс для генерации имен спрайтов
	int             mLabelIndex;        // Текущий индекс для генерации имен надписей
};

#endif // LOCATION_H
