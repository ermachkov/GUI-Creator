#ifndef BASE_LAYER_H
#define BASE_LAYER_H

class GameObject;
class LuaScript;
class Texture;

// Базовый класс слоя
class BaseLayer
{
public:

	// Максимальное количество вложенных слоев
	static const int MAX_NESTED_LAYERS = 10;

	// Состояние видимости слоя
	enum VisibleState
	{
		LAYER_VISIBLE,              // Слой полностью видим
		LAYER_PARTIALLY_VISIBLE,    // Слой видим, но родительский слой - нет
		LAYER_INVISIBLE             // Слой полностью невидим
	};

	// Состояние блокировки слоя
	enum LockState
	{
		LAYER_UNLOCKED,             // Слой разблокирован
		LAYER_PARTIALLY_UNLOCKED,   // Слой разблокирован, но родительский слой - нет
		LAYER_LOCKED                // Слой заблокирован
	};

	// Конструктор
	BaseLayer();

	// Конструктор
	BaseLayer(const QString &name, BaseLayer *parent = NULL, int index = 0);

	// Конструктор копирования
	BaseLayer(const BaseLayer &layer);

	// Деструктор
	virtual ~BaseLayer();

	// Возвращает имя слоя
	QString getName() const;

	// Устанавливает имя слоя
	void setName(const QString &name);

	// Возвращает состояние видимости слоя
	VisibleState getVisibleState() const;

	// Устанавливает состояние видимости слоя
	void setVisibleState(VisibleState visibleState);

	// Возвращает состояние блокировки слоя
	LockState getLockState() const;

	// Устанавливает состояние блокировки слоя
	void setLockState(LockState lockState);

	// Возвращает флаг разворачивания слоя
	bool isExpanded() const;

	// Устанавливает флаг разворачивания слоя
	void setExpanded(bool expanded);

	// Возвращает иконку предпросмотра
	QIcon getThumbnail() const;

	// Устанавливает иконку предпросмотра
	void setThumbnail(const QIcon &thumbnail);

	// Возвращает родительский слой
	BaseLayer *getParentLayer() const;

	// Устанавливает родительский слой
	void setParentLayer(BaseLayer *parent);

	// Возвращает список дочерних слоев
	QList<BaseLayer *> getChildLayers() const;

	// Возвращает дочерний слой по индексу
	BaseLayer *getChildLayer(int index) const;

	// Возвращает количество дочерних слоев
	int getNumChildLayers() const;

	// Возвращает индекс дочернего слоя в списке
	int indexOfChildLayer(BaseLayer *layer) const;

	// Рекурсивно ищет слой по заданному имени
	BaseLayer *findLayerByName(const QString &name);

	// Добавляет дочерний слой
	void addChildLayer(BaseLayer *layer);

	// Вставляет дочерний слой
	void insertChildLayer(int index, BaseLayer *layer);

	// Удаляет дочерний слой
	void removeChildLayer(int index);

	// Загружает слой из Lua скрипта
	virtual bool load(LuaScript &script, int depth);

	// Сохраняет слой в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Возвращает ограничивающий прямоугольник слоя
	virtual QRectF getBoundingRect() const = 0;

	// Ищет все активные игровые объекты
	virtual QList<GameObject *> findActiveGameObjects() const = 0;

	// Рекурсивно ищет игровой объект по заданному имени
	virtual GameObject *findGameObjectByName(const QString &name) const = 0;

	// Ищет игровой объект, содержащий заданную точку
	virtual GameObject *findGameObjectByPoint(const QPointF &pt) const = 0;

	// Ищет игровые объекты, попадающие в заданный прямоугольник
	virtual QList<GameObject *> findGameObjectsByRect(const QRectF &rect) const = 0;

	// Сортирует игровые объекты по глубине
	virtual QList<GameObject *> sortGameObjects(const QList<GameObject *> &objects) const = 0;

	// Дублирует слой
	virtual BaseLayer *duplicate(BaseLayer *parent = NULL, int index = 0) const = 0;

	// Возвращает список отсутствующих файлов в слое
	virtual QStringList getMissedFiles() const = 0;

	// Заменяет текстуру в слое
	virtual QList<GameObject *> changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture) = 0;

	// Отрисовывает слой
	virtual void draw(bool ignoreVisibleState = false) = 0;

protected:

	QString             mName;          // Имя (текстовое описание) слоя
	VisibleState        mVisibleState;  // Состояние видимости слоя
	LockState           mLockState;     // Состояние блокировки слоя
	bool                mExpanded;      // Флаг разворачивания слоя
	QIcon               mThumbnail;     // Иконка предпросмотра
	BaseLayer           *mParentLayer;  // Указатель на родительский слой
	QList<BaseLayer *>  mChildLayers;   // Список дочерних слоев
};

#endif // BASE_LAYER_H
