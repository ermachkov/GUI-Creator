#include "pch.h"
#include "layer_group.h"
#include "layer.h"
#include "lua_script.h"

LayerGroup::LayerGroup()
{
}

LayerGroup::LayerGroup(const QString &name, BaseLayer *parent, int index)
: BaseLayer(name, parent, index)
{
}

LayerGroup::LayerGroup(const LayerGroup &group)
: BaseLayer(group)
{
}

bool LayerGroup::load(LuaScript &script)
{
	// загружаем общие свойства слоя
	if (!BaseLayer::load(script))
		return false;

	// загружаем дочерние слои
	int length = script.getLength();
	for (int i = 1; i <= length; ++i)
	{
		// заходим в текущую таблицу
		script.pushTable(i);

		// определяем тип дочернего слоя
		QString type;
		if (!script.getString("type", type))
			return false;

		// создаем дочерний слой
		BaseLayer *layer;
		if (type == "Layer")
			layer = new Layer();
		else if (type == "LayerGroup")
			layer = new LayerGroup();
		else
			return false;

		// добавляем созданный слой в список дочерних слоев
		addChildLayer(layer);

		// загружаем дочерний слой
		if (!layer->load(script))
			return false;

		// выходим из текущей таблицы
		script.popTable();
	}

	return true;
}

bool LayerGroup::save(QTextStream &stream, int indent)
{
	// делаем отступ и записываем тип слоя
	QString tabs(indent, '\t');
	stream << tabs << "{type = \"LayerGroup\", ";

	// сохраняем общие данные базового слоя
	if (!BaseLayer::save(stream, indent))
		return false;

	// сохраняем дочерние слои, если они есть
	if (!mChildLayers.empty())
	{
		stream << ",\n";
		for (int i = 0; i < mChildLayers.size(); ++i)
		{
			if (!mChildLayers[i]->save(stream, indent + 1))
				return false;
			stream << (i < mChildLayers.size() - 1 ? ",\n" : "\n");
		}
		stream << tabs;
	}

	// сохраняем закрывающую фигурную скобку
	stream << "}";
	return stream.status() == QTextStream::Ok;
}

QRectF LayerGroup::getBoundingRect() const
{
	// определяем общий ограничивающий прямоугольник для всех дочерних слоев
	QRectF rect = !mChildLayers.empty() ? mChildLayers.front()->getBoundingRect() : QRectF();
	foreach (BaseLayer *layer, mChildLayers)
		rect |= layer->getBoundingRect();
	return rect;
}

QList<GameObject *> LayerGroup::findActiveGameObjects() const
{
	QList<GameObject *> objects;

	// получаем список объектов в дочерних слоях от верхнего к нижнему, если группа слоев видима и не заблокирована
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (BaseLayer *layer, mChildLayers)
			objects.append(layer->findActiveGameObjects());
	}

	return objects;
}

GameObject *LayerGroup::findGameObjectByPoint(const QPointF &pt) const
{
	// ищем объект в дочерних слоях от верхнего к нижнему, если группа слоев видима и не заблокирована
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (BaseLayer *layer, mChildLayers)
		{
			GameObject *object = layer->findGameObjectByPoint(pt);
			if (object != NULL)
				return object;
		}
	}

	return NULL;
}

QList<GameObject *> LayerGroup::findGameObjectsByRect(const QRectF &rect) const
{
	QList<GameObject *> objects;

	// ищем объекты в дочерних слоях от верхнего к нижнему, если группа слоев видима и не заблокирована
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (BaseLayer *layer, mChildLayers)
			objects.append(layer->findGameObjectsByRect(rect));
	}

	return objects;
}

QList<GameObject *> LayerGroup::sortGameObjects(const QList<GameObject *> &objects) const
{
	QList<GameObject *> sortedObjects;

	// формируем упорядоченный по глубине список объектов, имеющихся в дочерних слоях
	foreach (BaseLayer *layer, mChildLayers)
		sortedObjects.append(layer->sortGameObjects(objects));

	return sortedObjects;
}

BaseLayer *LayerGroup::duplicate(BaseLayer *parent, int index) const
{
	// создаем свою копию и добавляем ее к родительскому слою
	LayerGroup *group = new LayerGroup(*this);
	if (parent != NULL)
		parent->insertChildLayer(index, group);
	return group;
}

QStringList LayerGroup::getMissedTextures() const
{
	// получаем список отсутствующих текстур во всех дочерних слоях
	QStringList list;
	foreach (BaseLayer *layer, mChildLayers)
		list.append(layer->getMissedTextures());
	list.removeDuplicates();
	return list;
}

void LayerGroup::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуру во всех дочерних слоях
	foreach (BaseLayer *layer, mChildLayers)
		layer->changeTexture(fileName, texture);
}

void LayerGroup::draw()
{
	// отрисовываем все дочерние слои от нижнего к верхнему, если группа слоев видима
	if (mVisibleState == LAYER_VISIBLE)
	{
		for (int i = mChildLayers.size() - 1; i >= 0; --i)
			mChildLayers[i]->draw();
	}
}
