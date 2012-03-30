#include "pch.h"
#include "game_object.h"
#include "layer.h"
#include "lua_script.h"
#include "project.h"
#include "utils.h"

GameObject::GameObject()
: mParentLayer(NULL)
{
}

GameObject::GameObject(const QString &name, int id, Layer *parent)
: mName(name), mObjectID(id), mRotationAngle(0.0), mRotationCenter(0.5, 0.5), mParentLayer(NULL)
{
	// добавляем себя в родительский слой
	if (parent != NULL)
		parent->insertGameObject(0, this);
}

GameObject::GameObject(const GameObject &object)
: mName(object.mName), mObjectID(object.mObjectID), mPosition(object.mPosition), mSize(object.mSize),
  mRotationAngle(object.mRotationAngle), mRotationCenter(object.mRotationCenter), mParentLayer(NULL),
  mPositionXMap(object.mPositionXMap), mPositionYMap(object.mPositionYMap), mWidthMap(object.mWidthMap), mHeightMap(object.mHeightMap)
{
	// обновляем текущую трансформацию
	updateTransform();
}

GameObject::~GameObject()
{
	// удаляемся из родительского слоя
	if (mParentLayer != NULL)
		mParentLayer->removeGameObject(mParentLayer->indexOfGameObject(this));
}

QString GameObject::getName() const
{
	return mName;
}

void GameObject::setName(const QString &name)
{
	mName = name;
}

int GameObject::getObjectID() const
{
	return mObjectID;
}

void GameObject::setObjectID(int id)
{
	mObjectID = id;
}

QPointF GameObject::getPosition() const
{
	return mPosition;
}

void GameObject::setPosition(const QPointF &position)
{
	// устанавливаем новую позицию
	mPosition = position;
	updateTransform();

	// записываем локализованные координаты для текущего языка
	QString language = Project::getSingleton().getCurrentLanguage();
	mPositionXMap[language] = mPosition.x();
	mPositionYMap[language] = mPosition.y();
}

QSizeF GameObject::getSize() const
{
	return mSize;
}

void GameObject::setSize(const QSizeF &size)
{
	// устанавливаем новый размер
	mSize = size;
	updateTransform();

	// записываем локализованные размеры для текущего языка
	QString language = Project::getSingleton().getCurrentLanguage();
	mWidthMap[language] = mSize.width();
	mHeightMap[language] = mSize.height();
}

qreal GameObject::getRotationAngle() const
{
	return mRotationAngle;
}

void GameObject::setRotationAngle(qreal angle)
{
	mRotationAngle = angle;
	updateTransform();
}

QPointF GameObject::getRotationCenter() const
{
	QPointF pt(mRotationCenter.x() * mSize.width(), mRotationCenter.y() * mSize.height());
	return localToWorld(pt);
}

void GameObject::setRotationCenter(const QPointF &center)
{
	QPointF pt = worldToLocal(center);
	mRotationCenter = QPointF(pt.x() / mSize.width(), pt.y() / mSize.height());
}

Layer *GameObject::getParentLayer() const
{
	return mParentLayer;
}

void GameObject::setParentLayer(Layer *parent)
{
	mParentLayer = parent;
}

QRectF GameObject::getBoundingRect() const
{
	return mBoundingRect;
}

bool GameObject::isContainPoint(const QPointF &pt) const
{
	// проверяем, что локальные координаты точки лежат внутри исходного прямоугольника объекта (0, 0, w, h)
	return QRectF(0.0, 0.0, mSize.width(), mSize.height()).contains(worldToLocal(pt));
}

bool GameObject::isContainedInRect(const QRectF &rect) const
{
	// проверяем, что все четыре вершины объекта лежат внутри заданного прямоугольника
	return rect.contains(mVertices[0]) && rect.contains(mVertices[1]) && rect.contains(mVertices[2]) && rect.contains(mVertices[3]);
}

void GameObject::snapXCoord(qreal x, qreal y1, qreal y2, qreal &snappedX, qreal &distance, QLineF &line) const
{
	// находим расстояния по оси X до левого края, центра и правого края
	qreal leftDistance = qAbs(x - mBoundingRect.left());
	qreal centerDistance = qAbs(x - mBoundingRect.center().x());
	qreal rightDistance = qAbs(x - mBoundingRect.right());

	// определяем наименьшее расстояние и привязываем координату к соответствующему краю
	if (centerDistance < distance && centerDistance <= leftDistance && centerDistance <= rightDistance)
	{
		snappedX = mBoundingRect.center().x();
		distance = centerDistance;
		line = QLineF(snappedX, qMin(y1, mBoundingRect.center().y()), snappedX, qMax(y2, mBoundingRect.center().y()));
	}
	else if (leftDistance < distance && leftDistance <= centerDistance && leftDistance <= rightDistance)
	{
		snappedX = mBoundingRect.left();
		distance = leftDistance;
		line = QLineF(snappedX, qMin(y1, mBoundingRect.top()), snappedX, qMax(y2, mBoundingRect.bottom()));
	}
	else if (rightDistance < distance && rightDistance <= leftDistance && rightDistance <= centerDistance)
	{
		snappedX = mBoundingRect.right();
		distance = rightDistance;
		line = QLineF(snappedX, qMin(y1, mBoundingRect.top()), snappedX, qMax(y2, mBoundingRect.bottom()));
	}
}

void GameObject::snapYCoord(qreal y, qreal x1, qreal x2, qreal &snappedY, qreal &distance, QLineF &line) const
{
	// находим расстояния по оси Y до верхнего края, центра и нижнего края
	qreal topDistance = qAbs(y - mBoundingRect.top());
	qreal centerDistance = qAbs(y - mBoundingRect.center().y());
	qreal bottomDistance = qAbs(y - mBoundingRect.bottom());

	// определяем наименьшее расстояние и привязываем координату к соответствующему краю
	if (centerDistance < distance && centerDistance <= topDistance && centerDistance <= bottomDistance)
	{
		snappedY = mBoundingRect.center().y();
		distance = centerDistance;
		line = QLineF(qMin(x1, mBoundingRect.center().x()), snappedY, qMax(x2, mBoundingRect.center().x()), snappedY);
	}
	else if (topDistance < distance && topDistance <= centerDistance && topDistance <= bottomDistance)
	{
		snappedY = mBoundingRect.top();
		distance = topDistance;
		line = QLineF(qMin(x1, mBoundingRect.left()), snappedY, qMax(x2, mBoundingRect.right()), snappedY);
	}
	else if (bottomDistance < distance && bottomDistance <= topDistance && bottomDistance <= centerDistance)
	{
		snappedY = mBoundingRect.bottom();
		distance = bottomDistance;
		line = QLineF(qMin(x1, mBoundingRect.left()), snappedY, qMax(x2, mBoundingRect.right()), snappedY);
	}
}

bool GameObject::load(QDataStream &stream)
{
	// загружаем свойства объекта из потока
	stream >> mName >> mObjectID >> mPositionXMap >> mPositionYMap >> mWidthMap >> mHeightMap >> mRotationAngle >> mRotationCenter;
	return stream.status() == QDataStream::Ok;
}

bool GameObject::save(QDataStream &stream)
{
	// сохраняем данные в поток
	stream << mName << mObjectID << mPositionXMap << mPositionYMap << mWidthMap << mHeightMap << mRotationAngle << mRotationCenter;
	return stream.status() == QDataStream::Ok;
}

bool GameObject::load(LuaScript &script)
{
	// загружаем свойства объекта из Lua скрипта
	if (!script.getString("name", mName) || !script.getInt("id", mObjectID)
		|| !readRealMap(script, "posX", mPositionXMap) || !readRealMap(script, "posY", mPositionYMap)
		|| !readRealMap(script, "width", mWidthMap) || !readRealMap(script, "height", mHeightMap)
		|| !script.getReal("angle", mRotationAngle)
		|| !script.getReal("centerX", mRotationCenter.rx()) || !script.getReal("centerY", mRotationCenter.ry()))
		return false;
	return true;
}

bool GameObject::save(QTextStream &stream, int indent)
{
	// сохраняем свойства объекта в поток
	stream << "name = " << Utils::quotify(mName) << ", id = " << mObjectID;
	stream << ", posX = ";
	writeRealMap(stream, mPositionXMap);
	stream << ", posY = ";
	writeRealMap(stream, mPositionYMap);
	stream << ", width = ";
	writeRealMap(stream, mWidthMap);
	stream << ", height = ";
	writeRealMap(stream, mHeightMap);
	stream << ", angle = " << mRotationAngle << ", centerX = " << mRotationCenter.x() << ", centerY = " << mRotationCenter.y();
	return stream.status() == QTextStream::Ok;
}

void GameObject::setCurrentLanguage(const QString &language)
{
	// устанавливаем новые значения позиции и размера для текущего языка
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
	mPosition = QPointF(mPositionXMap[mPositionXMap.contains(language) ? language : defaultLanguage],
		mPositionYMap[mPositionYMap.contains(language) ? language : defaultLanguage]);
	mSize = QSizeF(mWidthMap[mWidthMap.contains(language) ? language : defaultLanguage],
		mHeightMap[mHeightMap.contains(language) ? language : defaultLanguage]);

	// обновляем текущую трансформацию
	updateTransform();
}

QPointF GameObject::localToWorld(const QPointF &pt) const
{
	return mTransform.map(pt);
}

QPointF GameObject::worldToLocal(const QPointF &pt) const
{
	return mInvTransform.map(pt);
}

void GameObject::updateTransform()
{
	// рассчитываем прямую и обратную матрицы трансформации
	mTransform.reset();
	mTransform.translate(mPosition.x(), mPosition.y());
	mTransform.rotate(mRotationAngle);
	mInvTransform = mTransform.inverted();

	// определяем мировые координаты вершин объекта
	mVertices[0] = localToWorld(QPointF(0.0, 0.0));
	mVertices[1] = localToWorld(QPointF(0.0, mSize.height()));
	mVertices[2] = localToWorld(QPointF(mSize.width(), mSize.height()));
	mVertices[3] = localToWorld(QPointF(mSize.width(), 0.0));

	// определяем ограничивающий прямоугольник объекта
	mBoundingRect = QRectF(mVertices[0], mVertices[0]);
	for (int i = 1; i < 4; ++i)
	{
		mBoundingRect.setLeft(qMin(mBoundingRect.left(), mVertices[i].x()));
		mBoundingRect.setTop(qMin(mBoundingRect.top(), mVertices[i].y()));
		mBoundingRect.setRight(qMax(mBoundingRect.right(), mVertices[i].x()));
		mBoundingRect.setBottom(qMax(mBoundingRect.bottom(), mVertices[i].y()));
	}
}

bool GameObject::readRealMap(LuaScript &script, const QString &name, RealMap &map)
{
	// очищаем список локализации
	map.clear();

	// пробуем получить одиночное значение
	qreal value;
	if (script.getReal(name, value))
	{
		map[Project::getSingleton().getDefaultLanguage()] = value;
		return true;
	}

	// пробуем получить таблицу с локализованными значениями
	if (script.pushTable(name))
	{
		// получаем все элементы таблицы
		script.firstEntry();
		while (script.nextEntry())
		{
			QString key;
			if (!script.getReal(value) || !script.getString(key, false))
				return false;
			map[key] = value;
		}

		// извлекаем таблицу из стека
		script.popTable();

		// проверяем, что в списке есть язык по умолчанию
		return map.contains(Project::getSingleton().getDefaultLanguage());
	}

	return false;
}

void GameObject::writeRealMap(QTextStream &stream, RealMap &map)
{
	// удаляем дубликаты дефолтного значения из списка локализации
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
	qreal defaultValue = map[defaultLanguage];
	for (RealMap::iterator it = map.begin(); it != map.end();)
	{
		if (it.key() != defaultLanguage && Utils::isEqual(*it, defaultValue))
			map.erase(it++);
		else
			++it;
	}

	// записываем значение свойства
	if (map.size() > 1)
	{
		// записываем таблицу
		stream << "{";
		for (RealMap::const_iterator it = map.begin(); it != map.end(); ++it)
			stream << "[" << Utils::quotify(it.key()) << "] = " << *it << (it != --map.end() ? ", " : "");
		stream << "}";
	}
	else
	{
		// записываем одиночное значение
		stream << defaultValue;
	}
}

bool GameObject::readStringMap(LuaScript &script, const QString &name, StringMap &map)
{
	// очищаем список локализации
	map.clear();

	// пробуем получить одиночное значение
	QString value;
	if (script.getString(name, value))
	{
		map[Project::getSingleton().getDefaultLanguage()] = value;
		return true;
	}

	// пробуем получить таблицу с локализованными значениями
	if (script.pushTable(name))
	{
		// получаем все элементы таблицы
		script.firstEntry();
		while (script.nextEntry())
		{
			QString key;
			if (!script.getString(value) || !script.getString(key, false))
				return false;
			map[key] = value;
		}

		// извлекаем таблицу из стека
		script.popTable();

		// проверяем, что в списке есть язык по умолчанию
		return map.contains(Project::getSingleton().getDefaultLanguage());
	}

	return false;
}

void GameObject::writeStringMap(QTextStream &stream, StringMap &map)
{
	// удаляем дубликаты дефолтного значения из списка локализации
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
	QString defaultValue = map[defaultLanguage];
	for (StringMap::iterator it = map.begin(); it != map.end();)
	{
		if (it.key() != defaultLanguage && *it == defaultValue)
			map.erase(it++);
		else
			++it;
	}

	// записываем значение свойства
	if (map.size() > 1)
	{
		// записываем таблицу
		stream << "{";
		for (StringMap::const_iterator it = map.begin(); it != map.end(); ++it)
			stream << "[" << Utils::quotify(it.key()) << "] = " << Utils::quotify(*it) << (it != --map.end() ? ", " : "");
		stream << "}";
	}
	else
	{
		// записываем одиночное значение
		stream << Utils::quotify(defaultValue);
	}
}
