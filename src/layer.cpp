#include "pch.h"
#include "layer.h"
#include "label.h"
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

bool Layer::load(LuaScript &script, int depth)
{
	// загружаем общие свойства слоя
	if (!BaseLayer::load(script, depth))
		return false;

	// загружаем дочерние объекты
	int length = script.getLength();
	for (int i = 1; i <= length; ++i)
	{
		// заходим в текущую таблицу
		QString type;
		if (!script.pushTable(i) || !script.getString("type", type))
			return false;

		// создаем дочерний объект
		GameObject *object;
		if (type == "Sprite")
			object = new Sprite();
		else if (type == "Label")
			object = new Label();
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

GameObject *Layer::findGameObjectByName(const QString &name) const
{
	// ищем игровой объект в списке дочерних объектов
	foreach (GameObject *object, mGameObjects)
		if (object->getName() == name)
			return object;
	return NULL;
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
	// ищем объекты в списке, если слой видим и не заблокирован
	QList<GameObject *> objects;
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
	// формируем упорядоченный по глубине список объектов, имеющихся в слое
	QList<GameObject *> sortedObjects;
	foreach (GameObject *object, mGameObjects)
		if (objects.contains(object))
			sortedObjects.push_back(object);
	return sortedObjects;
}

void Layer::snapXCoord(qreal x, qreal y1, qreal y2, const QList<GameObject *> &objects, qreal &snappedX, qreal &distance, QLineF &line) const
{
	// проходим по всем дочерним игровым объектам
	foreach (GameObject *object, mGameObjects)
		if (!objects.contains(object))
			object->snapXCoord(x, y1, y2, snappedX, distance, line);
}

void Layer::snapYCoord(qreal y, qreal x1, qreal x2, const QList<GameObject *> &objects, qreal &snappedY, qreal &distance, QLineF &line) const
{
	// проходим по всем дочерним игровым объектам
	foreach (GameObject *object, mGameObjects)
		if (!objects.contains(object))
			object->snapYCoord(y, x1, x2, snappedY, distance, line);
}

BaseLayer *Layer::duplicate(BaseLayer *parent, int index) const
{
	// создаем свою копию и добавляем ее к родительскому слою
	Layer *layer = new Layer(*this);
	if (parent != NULL)
		parent->insertChildLayer(index, layer);
	return layer;
}

QStringList Layer::getMissedFiles() const
{
	// получаем список отсутствующих файлов во всех дочерних объектах
	QStringList missedFiles;
	foreach (GameObject *object, mGameObjects)
		missedFiles.append(object->getMissedFiles());
	missedFiles.removeDuplicates();
	return missedFiles;
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
