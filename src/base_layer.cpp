#include "pch.h"
#include "base_layer.h"
#include "lua_script.h"
#include "utils.h"

BaseLayer::BaseLayer()
: mParentLayer(NULL)
{
}

BaseLayer::BaseLayer(const QString &name, BaseLayer *parent, int index)
: mName(name), mVisibleState(LAYER_VISIBLE), mLockState(LAYER_UNLOCKED), mExpanded(false), mParentLayer(NULL)
{
	// добавляем себя в родительский слой
	if (parent != NULL)
		parent->insertChildLayer(index, this);
}

BaseLayer::BaseLayer(const BaseLayer &layer)
: mName(layer.mName), mVisibleState(layer.mVisibleState), mLockState(layer.mLockState), mExpanded(layer.mExpanded), mThumbnail(layer.mThumbnail), mParentLayer(NULL)
{
	// дублируем все дочерние слои
	for (int i = layer.mChildLayers.size() - 1; i >= 0; --i)
		layer.mChildLayers[i]->duplicate(this);
}

BaseLayer::~BaseLayer()
{
	// удаляемся из родительского слоя
	if (mParentLayer != NULL)
		mParentLayer->removeChildLayer(mParentLayer->indexOfChildLayer(this));

	// удаляем все дочерние слои
	while (!mChildLayers.empty())
		delete mChildLayers.front();
}

QString BaseLayer::getName() const
{
	return mName;
}

void BaseLayer::setName(const QString &name)
{
	mName = name;
}

BaseLayer::VisibleState BaseLayer::getVisibleState() const
{
	return mVisibleState;
}

void BaseLayer::setVisibleState(VisibleState visibleState)
{
	mVisibleState = visibleState;
}

BaseLayer::LockState BaseLayer::getLockState() const
{
	return mLockState;
}

void BaseLayer::setLockState(LockState lockState)
{
	mLockState = lockState;
}

bool BaseLayer::isExpanded() const
{
	return mExpanded;
}

void BaseLayer::setExpanded(bool expanded)
{
	mExpanded = expanded;
}

QIcon BaseLayer::getThumbnail() const
{
	return mThumbnail;
}

void BaseLayer::setThumbnail(const QIcon &thumbnail)
{
	mThumbnail = thumbnail;
}

BaseLayer *BaseLayer::getParentLayer() const
{
	return mParentLayer;
}

void BaseLayer::setParentLayer(BaseLayer *parent)
{
	mParentLayer = parent;
}

QList<BaseLayer *> BaseLayer::getChildLayers() const
{
	return mChildLayers;
}

BaseLayer *BaseLayer::getChildLayer(int index) const
{
	return mChildLayers[index];
}

int BaseLayer::getNumChildLayers() const
{
	return mChildLayers.size();
}

int BaseLayer::indexOfChildLayer(BaseLayer *layer) const
{
	return mChildLayers.indexOf(layer);
}

BaseLayer *BaseLayer::findLayerByName(const QString &name)
{
	// проверяем себя на заданное имя
	if (mName == name)
		return this;

	// проводим рекурсивный поиск в дочерних слоях
	foreach (BaseLayer *child, mChildLayers)
	{
		BaseLayer *layer = child->findLayerByName(name);
		if (layer != NULL)
			return layer;
	}

	return NULL;
}

void BaseLayer::addChildLayer(BaseLayer *layer)
{
	insertChildLayer(mChildLayers.size(), layer);
}

void BaseLayer::insertChildLayer(int index, BaseLayer *layer)
{
	if (layer->getParentLayer() == NULL)
	{
		layer->setParentLayer(this);
		mChildLayers.insert(index, layer);
	}
}

void BaseLayer::removeChildLayer(int index)
{
	mChildLayers.takeAt(index)->setParentLayer(NULL);
}

bool BaseLayer::load(LuaScript &script)
{
	// загружаем свойства слоя
	int visibleState, lockState;
	if (!script.getString("name", mName) || !script.getInt("visible_state", visibleState) || !script.getInt("lock_state", lockState)
		|| !script.getBool("expanded", mExpanded))
		return false;

	// преобразуем свойства видимости и блокировки к перечислимым типам
	mVisibleState = static_cast<VisibleState>(visibleState);
	mLockState = static_cast<LockState>(lockState);
	return true;
}

bool BaseLayer::save(QTextStream &stream, int indent)
{
	// сохраняем свойства слоя в поток
	stream << "name = \"" << Utils::insertBackslashes(mName) << "\", visible_state = " << mVisibleState << ", lock_state = " << mLockState
		<< ", expanded = " << (mExpanded ? "true" : "false");
	return stream.status() == QTextStream::Ok;
}
