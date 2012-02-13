#ifndef LAYER_GROUP_H
#define LAYER_GROUP_H

#include "base_layer.h"

// Класс группы слоев (папки)
class LayerGroup : public BaseLayer
{
public:

	// Конструктор
	LayerGroup();

	// Конструктор
	LayerGroup(const QString &name, BaseLayer *parent = NULL, int index = 0);

	// Конструктор копирования
	LayerGroup(const LayerGroup &group);

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

	// Дублирует слой
	virtual BaseLayer *duplicate(BaseLayer *parent = NULL, int index = 0) const;

	// Возвращает список отсутствующих файлов в слое
	virtual QStringList getMissedFiles() const;

	// Заменяет текстуру в слое
	virtual QList<GameObject *> changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Отрисовывает слой
	virtual void draw(bool ignoreVisibleState = false);
};

#endif // LAYER_GROUP_H
