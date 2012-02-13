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

	// Создает и загружает игровой объект из бинарного потока
	GameObject *loadGameObject(QDataStream &stream);

	// Генерирует имя для копии объекта
	QString generateDuplicateName(const QString &name) const;

private:

	BaseLayer   *mRootLayer;        // Корневой слой
	BaseLayer   *mActiveLayer;      // Текущий активный слой
	int         mLayerIndex;        // Текущий индекс для генерации имен слоев
	int         mLayerGroupIndex;   // Текущий индекс для генерации имен групп слоев
	int         mSpriteIndex;       // Текущий индекс для генерации имен спрайтов
};

#endif // LOCATION_H
