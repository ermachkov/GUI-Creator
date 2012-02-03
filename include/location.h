#ifndef LOCATION_H
#define LOCATION_H

class BaseLayer;
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

private:

	BaseLayer   *mRootLayer;        // Корневой слой
	BaseLayer   *mActiveLayer;      // Текущий активный слой
	int         mLayerIndex;        // Текущий индекс слоя для генерации имен "Слой XXX"
	int         mLayerGroupIndex;   // Текущий индекс группы слоев для генерации имен "Группа XXX"
};

#endif // LOCATION_H
