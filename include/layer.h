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
	virtual bool load(LuaScript &script, int depth);

	// Сохраняет слой в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Возвращает ограничивающий прямоугольник слоя
	virtual QRectF getBoundingRect() const;

	// Ищет все активные игровые объекты
	virtual QList<GameObject *> findActiveGameObjects() const;

	// Рекурсивно ищет игровой объект по заданному имени
	virtual GameObject *findGameObjectByName(const QString &name) const;

	// Ищет игровой объект, содержащий заданную точку
	virtual GameObject *findGameObjectByPoint(const QPointF &pt) const;

	// Ищет игровые объекты, попадающие в заданный прямоугольник
	virtual QList<GameObject *> findGameObjectsByRect(const QRectF &rect) const;

	// Сортирует игровые объекты по глубине
	virtual QList<GameObject *> sortGameObjects(const QList<GameObject *> &objects) const;

	// Привязывает координату по оси X к игровым объектам
	virtual void snapXCoord(qreal x, qreal y1, qreal y2, const QList<GameObject *> &objects, qreal &snappedX, qreal &distance, QLineF &line) const;

	// Привязывает координату по оси Y к игровым объектам
	virtual void snapYCoord(qreal y, qreal x1, qreal x2, const QList<GameObject *> &objects, qreal &snappedY, qreal &distance, QLineF &line) const;

	// Дублирует слой
	virtual BaseLayer *duplicate(BaseLayer *parent = NULL, int index = 0) const;

	// Возвращает список отсутствующих файлов в слое
	virtual QStringList getMissedFiles() const;

	// Заменяет текстуру в слое
	virtual QList<GameObject *> changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Отрисовывает слой
	virtual void draw(bool ignoreVisibleState = false);

private:

	// Список игровых объектов
	QList<GameObject *> mGameObjects;
};

#endif // LAYER_H
