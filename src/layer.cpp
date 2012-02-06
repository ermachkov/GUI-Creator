#include "pch.h"
#include "layer.h"
#include "lua_script.h"
#include "sprite.h"

Layer::Layer()
{
}

Layer::Layer(const QString &name, BaseLayer *parent, int index)
: BaseLayer(name, parent, index)
{
}

Layer::Layer(const Layer &layer)
: BaseLayer(layer)
{
	// дублируем все дочерние игровые объекты
	for (int i = layer.mGameObjects.size() - 1; i >= 0; --i)
		layer.mGameObjects[i]->duplicate(this);
}

Layer::~Layer()
{
	// удаляем все дочерние игровые объекты
	while (!mGameObjects.empty())
		delete mGameObjects.front();
}

QList<GameObject *> Layer::getGameObjects() const
{
	return mGameObjects;
}

int Layer::getNumGameObjects() const
{
	return mGameObjects.size();
}

int Layer::indexOfGameObject(GameObject *object) const
{
	return mGameObjects.indexOf(object);
}

void Layer::addGameObject(GameObject *object)
{
	insertGameObject(mGameObjects.size(), object);
}

void Layer::insertGameObject(int index, GameObject *object)
{
	if (object->getParentLayer() == NULL)
	{
		object->setParentLayer(this);
		mGameObjects.insert(index, object);
	}
}

void Layer::removeGameObject(int index)
{
	mGameObjects.takeAt(index)->setParentLayer(NULL);
}

bool Layer::load(LuaScript &script)
{
	// загружаем общие свойства слоя
	if (!BaseLayer::load(script))
		return false;

	// загружаем дочерние объекты
	int length = script.getLength();
	for (int i = 1; i <= length; ++i)
	{
		// заходим в текущую таблицу
		script.pushTable(i);

		// определяем тип дочернего объекта
		QString type;
		if (!script.getString("type", type))
			return false;

		// создаем дочерний объект
		GameObject *object;
		if (type == "Sprite")
			object = new Sprite();
		else
			return false;

		// добавляем созданный объект в список дочерних объектов
		addGameObject(object);

		// загружаем дочерний объект
		if (!object->load(script))
			return false;

		// выходим из текущей таблицы
		script.popTable();
	}

	return true;
}

bool Layer::save(QTextStream &stream, int indent)
{
	// делаем отступ и записываем тип слоя
	QString tabs(indent, '\t');
	stream << tabs << "{type = \"Layer\", ";

	// сохраняем общие данные базового слоя
	if (!BaseLayer::save(stream, indent))
		return false;

	// сохраняем дочерние объекты, если они есть
	if (!mGameObjects.empty())
	{
		stream << ",\n";
		for (int i = 0; i < mGameObjects.size(); ++i)
		{
			if (!mGameObjects[i]->save(stream, indent + 1))
				return false;
			stream << (i < mGameObjects.size() - 1 ? ",\n" : "\n");
		}
		stream << tabs;
	}

	// сохраняем закрывающую фигурную скобку
	stream << "}";
	return stream.status() == QTextStream::Ok;
}

QRectF Layer::getBoundingRect() const
{
	// определяем общий ограничивающий прямоугольник для всех игровых объектов
	QRectF rect = !mGameObjects.empty() ? mGameObjects.front()->getBoundingRect() : QRectF();
	foreach (GameObject *object, mGameObjects)
		rect |= object->getBoundingRect();
	return rect;
}

QList<GameObject *> Layer::findActiveGameObjects() const
{
	// возвращаем список объектов, если слой видим и не заблокирован
	return mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED ? mGameObjects : QList<GameObject *>();
}

GameObject *Layer::findGameObjectByPoint(const QPointF &pt) const
{
	// ищем объект в списке, если слой видим и не заблокирован
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (GameObject *object, mGameObjects)
			if (object->isContainPoint(pt))
				return object;
	}

	return NULL;
}

QList<GameObject *> Layer::findGameObjectsByRect(const QRectF &rect) const
{
	QList<GameObject *> objects;

	// ищем объекты в списке, если слой видим и не заблокирован
	if (mVisibleState == LAYER_VISIBLE && mLockState == LAYER_UNLOCKED)
	{
		foreach (GameObject *object, mGameObjects)
			if (object->isContainedInRect(rect))
				objects.push_back(object);
	}

	return objects;
}

QList<GameObject *> Layer::sortGameObjects(const QList<GameObject *> &objects) const
{
	QList<GameObject *> sortedObjects;

	// формируем упорядоченный по глубине список объектов, имеющихся в слое
	foreach (GameObject *object, mGameObjects)
		if (objects.contains(object))
			sortedObjects.push_back(object);

	return sortedObjects;
}

BaseLayer *Layer::duplicate(BaseLayer *parent, int index) const
{
	// создаем свою копию и добавляем ее к родительскому слою
	Layer *layer = new Layer(*this);
	if (parent != NULL)
		parent->insertChildLayer(index, layer);
	return layer;
}

QStringList Layer::getMissedTextures() const
{
	// получаем список отсутствующих текстур во всех дочерних объектах
	QStringList list;
	foreach (GameObject *object, mGameObjects)
		list.append(object->getMissedTextures());
	list.removeDuplicates();
	return list;
}

QList<GameObject *> Layer::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуру во всех дочерних объектах
	QList<GameObject *> objects;
	foreach (GameObject *object, mGameObjects)
		if (object->changeTexture(fileName, texture))
			objects.push_back(object);
	return objects;
}

void Layer::draw(bool ignoreVisibleState)
{
	// отрисовываем все объекты в слое от нижнего к верхнему, если слой видим
	if (ignoreVisibleState || mVisibleState == LAYER_VISIBLE)
	{
		for (int i = mGameObjects.size() - 1; i >= 0; --i)
			mGameObjects[i]->draw();
	}
}
