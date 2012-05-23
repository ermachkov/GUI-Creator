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

bool LayerGroup::load(QDataStream &stream)
{
	// загружаем общие свойства базового слоя
	if (!BaseLayer::load(stream))
		return false;

	// загружаем количество дочерних слоев
	int numChildLayers;
	stream >> numChildLayers;
	if (stream.status() != QDataStream::Ok)
		return false;

	// загружаем дочерние слои
	for (int i = 0; i < numChildLayers; ++i)
	{
		// читаем тип слоя
		QString type;
		stream >> type;
		if (stream.status() != QDataStream::Ok)
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
		if (!layer->load(stream))
			return false;
	}

	return true;
}

bool LayerGroup::save(QDataStream &stream)
{
	// сохраняем тип слоя
	stream << QString("LayerGroup");
	if (stream.status() != QDataStream::Ok)
		return false;

	// сохраняем общие свойства базового слоя
	if (!BaseLayer::save(stream))
		return false;

	// сохраняем количество дочерних слоев
	stream << mChildLayers.size();

	// сохраняем дочерние слои
	foreach (BaseLayer *layer, mChildLayers)
		if (!layer->save(stream))
			return false;

	return stream.status() == QDataStream::Ok;
}

bool LayerGroup::load(LuaScript &script, int depth)
{
	// загружаем общие свойства базового слоя
	if (!BaseLayer::load(script, depth))
		return false;

	// загружаем дочерние слои
	int length = script.getLength();
	for (int i = 1; i <= length; ++i)
	{
		// заходим в текущую таблицу
		QString type;
		if (!script.pushTable(i) || !script.getString("type", type))
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
		if (!layer->load(script, depth + 1))
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

	// сохраняем общие свойства базового слоя
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
	// получаем список объектов в дочерних слоях от верхнего к нижнему, если группа слоев видима и не заблокирована
	QList<GameObject *> objects;
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (BaseLayer *layer, mChildLayers)
			objects.append(layer->findActiveGameObjects());
	}

	return objects;
}

GameObject *LayerGroup::findGameObjectByName(const QString &name) const
{
	// ищем игровой объект в списке дочерних слоев
	foreach (BaseLayer *layer, mChildLayers)
	{
		GameObject *object = layer->findGameObjectByName(name);
		if (object != NULL)
			return object;
	}

	return NULL;
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
	// ищем объекты в дочерних слоях от верхнего к нижнему, если группа слоев видима и не заблокирована
	QList<GameObject *> objects;
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (BaseLayer *layer, mChildLayers)
			objects.append(layer->findGameObjectsByRect(rect));
	}

	return objects;
}

QList<GameObject *> LayerGroup::sortGameObjects(const QList<GameObject *> &objects) const
{
	// формируем упорядоченный по глубине список объектов, имеющихся в дочерних слоях
	QList<GameObject *> sortedObjects;
	foreach (BaseLayer *layer, mChildLayers)
		sortedObjects.append(layer->sortGameObjects(objects));

	return sortedObjects;
}

void LayerGroup::snapXCoord(qreal x, qreal y1, qreal y2, const QList<GameObject *> &excludedObjects, qreal &snappedX, qreal &distance, QLineF &line) const
{
	// проходим по всем дочерним слоям, если группа слоев видима и не заблокирована
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
		foreach (BaseLayer *layer, mChildLayers)
			layer->snapXCoord(x, y1, y2, excludedObjects, snappedX, distance, line);
}

void LayerGroup::snapYCoord(qreal y, qreal x1, qreal x2, const QList<GameObject *> &excludedObjects, qreal &snappedY, qreal &distance, QLineF &line) const
{
	// проходим по всем дочерним слоям, если группа слоев видима и не заблокирована
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
		foreach (BaseLayer *layer, mChildLayers)
			layer->snapYCoord(y, x1, x2, excludedObjects, snappedY, distance, line);
}

void LayerGroup::setCurrentLanguage(const QString &language)
{
	// устанавливаем текущий язык для всех дочерних слоев
	foreach (BaseLayer *layer, mChildLayers)
		layer->setCurrentLanguage(language);

	// удаляем иконку предпросмотра
	mThumbnail = QIcon();
}

void LayerGroup::loadTranslations(LuaScript *script)
{
	// загружаем переводы для всех дочерних слоев
	foreach (BaseLayer *layer, mChildLayers)
		layer->loadTranslations(script);
}

BaseLayer *LayerGroup::duplicate(BaseLayer *parent, int index) const
{
	// создаем свою копию и добавляем ее к родительскому слою
	LayerGroup *group = new LayerGroup(*this);
	if (parent != NULL)
		parent->insertChildLayer(index, group);
	return group;
}

QStringList LayerGroup::getMissedFiles() const
{
	// получаем список отсутствующих файлов во всех дочерних слоях
	QStringList missedFiles;
	foreach (BaseLayer *layer, mChildLayers)
		missedFiles.append(layer->getMissedFiles());
	missedFiles.removeDuplicates();
	return missedFiles;
}

QList<GameObject *> LayerGroup::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуру во всех дочерних слоях
	QList<GameObject *> objects;
	foreach (BaseLayer *layer, mChildLayers)
		objects.append(layer->changeTexture(fileName, texture));
	return objects;
}

void LayerGroup::draw(bool ignoreVisibleState)
{
	// отрисовываем все дочерние слои от нижнего к верхнему, если группа слоев видима
	if (ignoreVisibleState || mVisibleState == LAYER_VISIBLE)
	{
		for (int i = mChildLayers.size() - 1; i >= 0; --i)
			mChildLayers[i]->draw();
	}
}
