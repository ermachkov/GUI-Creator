#ifndef LAYER_H
#define LAYER_H

#include "base_layer.h"

// Класс обычного слоя
class Layer : public BaseLayer
{
public:

	// Конструктор
	Layer();

	// Конструктор
	Layer(const QString &name, BaseLayer *parent = NULL, int index = 0);

	// Конструктор копирования
	Layer(const Layer &layer);

	// Деструктор
	virtual ~Layer();

	// Возвращает список игровых объектов
	QList<GameObject *> getGameObjects() const;

	// Возвращает количество игровых объектов в слое
	int getNumGameObjects() const;

	// Возвращает индекс игрового объекта в списке
	int indexOfGameObject(GameObject *object) const;

	// Добавляет игровой объект
	void addGameObject(GameObject *object);

	// Вставляет игровой объект
	void insertGameObject(int index, GameObject *object);

	// Удаляет игровой объект
	void removeGameObject(int index);

	// Загружает слой из Lua скрипта
	virtual bool load(LuaScript &script);

	// Сохраняет слой в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Возвращает ограничивающий прямоугольник слоя
	virtual QRectF getBoundingRect() const;

	// Ищет все активные игровые объекты
	virtual QList<GameObject *> findActiveGameObjects() const;

	// Ищет игровой объект, содержащий заданную точку
	virtual GameObject *findGameObjectByPoint(const QPointF &pt) const;

	// Ищет игровые объекты, попадающие в заданный прямоугольник
	virtual QList<GameObject *> findGameObjectsByRect(const QRectF &rect) const;

	// Сортирует игровые объекты по глубине
	virtual QList<GameObject *> sortGameObjects(const QList<GameObject *> &objects) const;

	// Дублирует слой
	virtual BaseLayer *duplicate(BaseLayer *parent = NULL, int index = 0) const;

	// Возвращает список отсутствующих текстур в слое
	virtual QStringList getMissedTextures() const;

	// Заменяет текстуру в слое
	virtual void changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Отрисовывает слой
	virtual void draw();

private:

	// Список игровых объектов
	QList<GameObject *> mGameObjects;
};

#endif // LAYER_H
