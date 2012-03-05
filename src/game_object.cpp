#include "pch.h"
#include "game_object.h"
#include "layer.h"
#include "lua_script.h"
#include "utils.h"

GameObject::GameObject()
: mParentLayer(NULL)
{
}

GameObject::GameObject(const QString &name, int id, Layer *parent)
: mName(name), mObjectID(id), mRotationAngle(0.0), mParentLayer(NULL)
{
	// добавляем себя в родительский слой
	if (parent != NULL)
		parent->insertGameObject(0, this);
}

GameObject::GameObject(const GameObject &object)
: mName(object.mName), mObjectID(object.mObjectID), mPosition(object.mPosition), mSize(object.mSize),
  mRotationAngle(object.mRotationAngle), mRotationCenter(object.mRotationCenter), mParentLayer(NULL)
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
	mPosition = position;
	updateTransform();
}

QSizeF GameObject::getSize() const
{
	return mSize;
}

void GameObject::setSize(const QSizeF &size)
{
	// пересчитываем локальные координаты центра вращения и сохраняем новый размер
	mRotationCenter = QPointF(mRotationCenter.x() * size.width() / mSize.width(), mRotationCenter.y() * size.height() / mSize.height());
	mSize = size;
	updateTransform();
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
	return localToWorld(mRotationCenter);
}

void GameObject::setRotationCenter(const QPointF &center)
{
	mRotationCenter = worldToLocal(center);
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
	if (leftDistance < distance && leftDistance <= centerDistance && leftDistance <= rightDistance)
	{
		snappedX = mBoundingRect.left();
		distance = leftDistance;
		line = QLineF(snappedX, qMin(y1, mBoundingRect.top()), snappedX, qMax(y2, mBoundingRect.bottom()));
	}
	else if (centerDistance < distance && centerDistance <= leftDistance && centerDistance <= rightDistance)
	{
		snappedX = mBoundingRect.center().x();
		distance = centerDistance;
		line = QLineF(snappedX, qMin(y1, mBoundingRect.center().y()), snappedX, qMax(y2, mBoundingRect.center().y()));
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
	if (topDistance < distance && topDistance <= centerDistance && topDistance <= bottomDistance)
	{
		snappedY = mBoundingRect.top();
		distance = topDistance;
		line = QLineF(qMin(x1, mBoundingRect.left()), snappedY, qMax(x2, mBoundingRect.right()), snappedY);
	}
	else if (centerDistance < distance && centerDistance <= topDistance && centerDistance <= bottomDistance)
	{
		snappedY = mBoundingRect.center().y();
		distance = centerDistance;
		line = QLineF(qMin(x1, mBoundingRect.center().x()), snappedY, qMax(x2, mBoundingRect.center().x()), snappedY);
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
	stream >> mName >> mObjectID >> mPosition >> mSize >> mRotationAngle >> mRotationCenter;
	if (stream.status() != QDataStream::Ok)
		return false;

	// обновляем текущую трансформацию
	updateTransform();
	return true;
}

bool GameObject::save(QDataStream &stream)
{
	// сохраняем данные в поток
	stream << mName << mObjectID << mPosition << mSize << mRotationAngle << mRotationCenter;
	return stream.status() == QDataStream::Ok;
}

bool GameObject::load(LuaScript &script)
{
	// загружаем свойства объекта из Lua скрипта
	if (!script.getString("name", mName) || !script.getInt("id", mObjectID)
		|| !script.getReal("pos_x", mPosition.rx()) || !script.getReal("pos_y", mPosition.ry())
		|| !script.getReal("width", mSize.rwidth()) || !script.getReal("height", mSize.rheight())
		|| !script.getReal("angle", mRotationAngle)
		|| !script.getReal("center_x", mRotationCenter.rx()) || !script.getReal("center_y", mRotationCenter.ry()))
		return false;

	// обновляем текущую трансформацию
	updateTransform();
	return true;
}

bool GameObject::save(QTextStream &stream, int indent)
{
	// сохраняем свойства объекта в поток
	stream << "name = \"" << Utils::insertBackslashes(mName) << "\", id = " << mObjectID
		<< ", pos_x = " << mPosition.x() << ", pos_y = " << mPosition.y()
		<< ", width = " << mSize.width() << ", height = " << mSize.height()
		<< ", angle = " << mRotationAngle
		<< ", center_x = " << mRotationCenter.x() << ", center_y = " << mRotationCenter.y();
	return stream.status() == QTextStream::Ok;
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
