#include "pch.h"
#include "game_object.h"
#include "layer.h"
#include "lua_script.h"

GameObject::GameObject()
: mParentLayer(NULL)
{
}

GameObject::GameObject(const QPointF &position, const QSizeF &size, Layer *parent)
: mPosition(position), mSize(size), mScale(1.0, 1.0), mRotationAngle(0.0), mRotationCenter(size.width() / 2.0, size.height() / 2.0), mParentLayer(NULL)
{
	// добавляем себя в родительский слой и обновляем текущую трансформацию
	if (parent != NULL)
		parent->insertGameObject(0, this);
	updateTransform();
}

GameObject::GameObject(const GameObject &object)
: mPosition(object.mPosition), mSize(object.mSize), mScale(object.mScale), mRotationAngle(object.mRotationAngle), mRotationCenter(object.mRotationCenter), mParentLayer(NULL)
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
	// пересчитываем локальные координаты центра вращения
	mRotationCenter = QPointF(mRotationCenter.x() * size.width() / mSize.width(), mRotationCenter.y() * size.height() / mSize.height());
	mSize = size;
	updateTransform();
}

QPointF GameObject::getScale() const
{
	return mScale;
}

void GameObject::setScale(const QPointF &scale)
{
	// сохраняем масштаб, если он не делает объект меньше 1 пикселя
	if (qAbs(mSize.width() * scale.x()) >= 1.0)
		mScale.rx() = scale.x();
	if (qAbs(mSize.height() * scale.y()) >= 1.0)
		mScale.ry() = scale.y();
	updateTransform();
}

qreal GameObject::getRotationAngle() const
{
	return mRotationAngle;
}

void GameObject::setRotationAngle(qreal angle)
{
	// приводим угол к диапазону [0; 360) градусов
	mRotationAngle = fmod(angle, 360.0);
	if (mRotationAngle < 0.0)
		mRotationAngle += 360.0;
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

bool GameObject::load(QDataStream &stream)
{
	// загружаем свойства объекта из потока
	stream >> mPosition >> mSize >> mScale >> mRotationAngle >> mRotationCenter;
	if (stream.status() != QDataStream::Ok)
		return false;

	// обновляем текущую трансформацию
	updateTransform();
	return true;
}

bool GameObject::save(QDataStream &stream)
{
	// сохраняем данные в поток
	stream << mPosition << mSize << mScale << mRotationAngle << mRotationCenter;
	return stream.status() == QDataStream::Ok;
}

bool GameObject::load(LuaScript &script)
{
	// загружаем свойства объекта из Lua скрипта
	if (!script.getReal("pos_x", mPosition.rx()) || !script.getReal("pos_y", mPosition.ry())
		|| !script.getReal("width", mSize.rwidth()) || !script.getReal("height", mSize.rheight())
		|| !script.getReal("scale_x", mScale.rx()) || !script.getReal("scale_y", mScale.ry())
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
	stream << "pos_x = " << mPosition.x() << ", pos_y = " << mPosition.y()
		<< ", width = " << mSize.width() << ", height = " << mSize.height()
		<< ", scale_x = " << mScale.x() << ", scale_y = " << mScale.y()
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
	// проверяем угол поворота на 0/90/180/270 градусов
	const qreal EPS = 0.1;
	if (qAbs(mRotationAngle) <= EPS)
		mRotationAngle = 0.0;
	else if (qAbs(mRotationAngle - 90.0) <= EPS)
		mRotationAngle = 90.0;
	else if (qAbs(mRotationAngle - 180.0) <= EPS)
		mRotationAngle = 180.0;
	else if (qAbs(mRotationAngle - 270.0) <= EPS)
		mRotationAngle = 270.0;

	// для неповернутых спрайтов округляем координаты и размер до целочисленных значений
	if (mRotationAngle == 0.0 || mRotationAngle == 90.0 || mRotationAngle == 180.0 || mRotationAngle == 270.0)
	{
		mPosition = QPointF(qRound(mPosition.x()), qRound(mPosition.y()));
		mScale = QPointF(qRound(mSize.width() * mScale.x()) / mSize.width(), qRound(mSize.height() * mScale.y()) / mSize.height());
	}

	// рассчитываем прямую и обратную матрицы трансформации
	mTransform.reset();
	mTransform.translate(mPosition.x(), mPosition.y());
	mTransform.rotate(mRotationAngle);
	mTransform.scale(mScale.x(), mScale.y());
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
