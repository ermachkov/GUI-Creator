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

	// Загружает слой из бинарного потока
	virtual bool load(QDataStream &stream);

	// Сохраняет слой в бинарный поток
	virtual bool save(QDataStream &stream);

	// Загружает слой из Lua скрипта
	virtual bool load(LuaScript &script, int depth);

	// Сохраняет слой в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Возвращает ограничивающий прямоугольник слоя
	virtual QRectF getBoundingRect() const;

	// Возвращает список всех игровых объектов
	virtual QList<GameObject *> getGameObjects() const;

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
	virtual void snapXCoord(qreal x, qreal y1, qreal y2, const QList<GameObject *> &excludedObjects, qreal &snappedX, qreal &distance, QLineF &line) const;

	// Привязывает координату по оси Y к игровым объектам
	virtual void snapYCoord(qreal y, qreal x1, qreal x2, const QList<GameObject *> &excludedObjects, qreal &snappedY, qreal &distance, QLineF &line) const;

	// Устанавливает текущий язык для слоя
	virtual void setCurrentLanguage(const QString &language);

	// Загружает переводы из Lua скрипта
	virtual void loadTranslations(LuaScript *script);

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
