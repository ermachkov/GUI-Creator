#include "pch.h"
#include "scene.h"
#include "label.h"
#include "layer.h"
#include "layer_group.h"
#include "lua_script.h"
#include "sprite.h"
#include "utils.h"

Scene::Scene(QObject *parent)
: QObject(parent), mCommandIndex(0), mObjectIndex(1), mLayerIndex(1), mLayerGroupIndex(1), mSpriteIndex(1), mLabelIndex(1)
{
	// создаем корневой слой
	mRootLayer = new LayerGroup("");

	// добавляем в него новый слой по умолчанию и делаем его активным
	mActiveLayer = createLayer(mRootLayer);

	// создаем стек отмен
	mUndoStack = new QUndoStack(this);
	connect(mUndoStack, SIGNAL(indexChanged(int)), this, SLOT(onUndoStackIndexChanged(int)));

	// сохраняем начальное состояние сцены
	mInitialState = new UndoCommand("", this);
}

Scene::~Scene()
{
	delete mInitialState;
	delete mRootLayer;
}

bool Scene::load(const QString &fileName)
{
	// загружаем Lua скрипт и проверяем, что он вернул корневую таблицу
	LuaScript script;
	if (!script.load(fileName, 1) || !script.pushTable())
		return false;

	// пересоздаем корневой слой
	delete mRootLayer;
	mRootLayer = new LayerGroup();

	// загружаем слои
	QString type;
	if (!script.pushTable("layers") || script.getLength() == 0 || !script.getString("type", type) || type != "LayerGroup" || !mRootLayer->load(script, 0))
		return false;
	script.popTable();

	// устанавливаем активный слой
	int length = 0;
	if (!script.pushTable("activeLayer") || (length = script.getLength()) == 0)
		return false;

	mActiveLayer = mRootLayer;
	for (int i = 1; i <= length; ++i)
	{
		int index;
		if (!script.getInt(i, index) || index < 0 || index >= mActiveLayer->getNumChildLayers())
			return false;
		mActiveLayer = mActiveLayer->getChildLayer(index);
	}

	script.popTable();

	// загружаем счетчики для генерации имен
	if (!script.getInt("objectIndex", mObjectIndex) || !script.getInt("layerIndex", mLayerIndex) || !script.getInt("layerGroupIndex", mLayerGroupIndex)
		|| !script.getInt("spriteIndex", mSpriteIndex) || !script.getInt("labelIndex", mLabelIndex))
		return false;

	// загружаем горизонтальные направляющие
	if (!script.pushTable("horzGuides"))
		return false;

	length = script.getLength();
	for (int i = 1; i <= length; ++i)
	{
		qreal coord;
		if (!script.getReal(i, coord))
			return false;
		mHorzGuides.push_back(coord);
	}

	script.popTable();

	// загружаем вертикальные направляющие
	if (!script.pushTable("vertGuides"))
		return false;

	length = script.getLength();
	for (int i = 1; i <= length; ++i)
	{
		qreal coord;
		if (!script.getReal(i, coord))
			return false;
		mVertGuides.push_back(coord);
	}

	script.popTable();

	// извлекаем из стека корневую таблицу
	script.popTable();

	// пересохраняем начальное состояние сцены
	delete mInitialState;
	mInitialState = new UndoCommand("", this);

	return true;
}

bool Scene::save(const QString &fileName)
{
	// открываем файл
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly))
		return false;

	// создаем текстовый поток
	QTextStream stream(&file);
	stream << qSetRealNumberPrecision(8) << uppercasedigits;

	// записываем шапку файла
	Utils::writeFileHeader(stream);

	// записываем код для возврата корневой таблицы
	stream << endl << "return" << endl;
	stream << "{" << endl;

	// сохраняем слои
	stream << "\t-- Layers table" << endl;
	stream << "\tlayers =" << endl;
	mRootLayer->save(stream, 1);
	stream << "," << endl;

	// сохраняем последовательность индексов, ведущую к активному слою
	QStringList indices;
	for (BaseLayer *layer = mActiveLayer; layer != mRootLayer; layer = layer->getParentLayer())
		indices.push_front(QString::number(layer->getParentLayer()->indexOfChildLayer(layer)));
	stream << endl << "\t-- Path to the current active layer" << endl;
	stream << "\tactiveLayer = {" << indices.join(", ") << "}," << endl;

	// сохраняем счетчики для генерации имен
	stream << endl << "\t-- Counters for generating various object IDs" << endl;
	stream << "\tobjectIndex = " << mObjectIndex << "," << endl;
	stream << "\tlayerIndex = " << mLayerIndex << "," << endl;
	stream << "\tlayerGroupIndex = " << mLayerGroupIndex << "," << endl;
	stream << "\tspriteIndex = " << mSpriteIndex << "," << endl;
	stream << "\tlabelIndex = " << mLabelIndex << "," << endl;

	// сохраняем горизонтальные направляющие
	QStringList horzGuides;
	foreach (qreal coord, mHorzGuides)
		horzGuides.push_back(QString::number(coord));
	stream << endl << "\t-- Coordinates of horizontal and vertical guides" << endl;
	stream << "\thorzGuides = {" << horzGuides.join(", ") << "}," << endl;

	// сохраняем вертикальные направляющие
	QStringList vertGuides;
	foreach (qreal coord, mVertGuides)
		vertGuides.push_back(QString::number(coord));
	stream << "\tvertGuides = {" << vertGuides.join(", ") << "}" << endl;

	// записываем закрывающую фигурную скобку корневой таблицы
	stream << "}" << endl;

	return stream.status() == QTextStream::Ok;
}

bool Scene::loadTranslationFile(const QString &fileName)
{
	// загружаем Lua скрипт и проверяем, что он вернул корневую таблицу
	LuaScript script;
	if (!script.load(fileName, 1) || !script.pushTable())
	{
		mRootLayer->loadTranslations(NULL);
		return false;
	}

	// загружаем переводы из Lua скрипта
	mRootLayer->loadTranslations(&script);

	// извлекаем из стека корневую таблицу
	script.popTable();

	return true;
}

QList<GameObject *> Scene::findUsedGameObjects(const QString &fileName, const QList<GameObject *> &objects) const
{
	// загружаем Lua скрипт и проверяем, что он вернул корневую таблицу
	LuaScript script;
	if (!script.load(fileName, 1) || !script.pushTable())
		return QList<GameObject *>();

	// ищем объекты, используемые в файле имен
	QList<GameObject *> usedObjects;
	QString name;
	foreach (GameObject *object, objects)
		if (script.getString(object->getObjectID(), name))
			usedObjects.push_back(object);

	// извлекаем из стека корневую таблицу
	script.popTable();

	return usedObjects;
}

BaseLayer *Scene::getRootLayer() const
{
	return mRootLayer;
}

BaseLayer *Scene::getActiveLayer() const
{
	return mActiveLayer;
}

void Scene::setActiveLayer(BaseLayer *layer)
{
	mActiveLayer = layer;
}

Layer *Scene::getAvailableLayer() const
{
	// возвращаем указатель на активный слой, если он не папка, видим и не заблокирован
	Layer *layer = dynamic_cast<Layer *>(mActiveLayer);
	return layer != NULL && layer->getVisibleState() == BaseLayer::LAYER_VISIBLE && layer->getLockState() == BaseLayer::LAYER_UNLOCKED ? layer : NULL;
}

QUndoStack *Scene::getUndoStack() const
{
	return mUndoStack;
}

bool Scene::isClean() const
{
	return mUndoStack->isClean();
}

void Scene::setClean()
{
	mUndoStack->setClean();
}

void Scene::pushCommand(const QString &commandName)
{
	mCommandIndex = mUndoStack->index() + 1;
	mUndoStack->push(new UndoCommand(commandName, this));
}

bool Scene::canUndo() const
{
	return mUndoStack->canUndo();
}

bool Scene::canRedo() const
{
	return mUndoStack->canRedo();
}

void Scene::undo()
{
	mUndoStack->undo();
}

void Scene::redo()
{
	mUndoStack->redo();
}

BaseLayer *Scene::createLayer(BaseLayer *parent, int index)
{
	return new Layer(QString("Слой %1").arg(mLayerIndex++), parent, index);
}

BaseLayer *Scene::createLayerGroup(BaseLayer *parent, int index)
{
	return new LayerGroup(QString("Группа %1").arg(mLayerGroupIndex++), parent, index);
}

GameObject *Scene::createSprite(const QPointF &pos, const QString &fileName)
{
	return new Sprite(QString("Спрайт %1").arg(mSpriteIndex++), mObjectIndex++, pos, fileName, getAvailableLayer());
}

GameObject *Scene::createLabel(const QPointF &pos, const QString &fileName, int size)
{
	return new Label(QString("Текст %1").arg(mLabelIndex++), mObjectIndex++, pos, fileName, size, getAvailableLayer());
}

GameObject *Scene::loadGameObject(QDataStream &stream)
{
	// читаем тип объекта
	QString type;
	stream >> type;
	if (stream.status() != QDataStream::Ok)
		return NULL;

	// создаем объект нужного типа
	GameObject *object;
	if (type == "Sprite")
		object = new Sprite();
	else if (type == "Label")
		object = new Label();
	else
		return NULL;

	// загружаем объект
	if (!object->load(stream))
	{
		delete object;
		return NULL;
	}

	return object;
}

QString Scene::generateDuplicateName(const QString &name) const
{
	// выделяем базовую часть имени
	QString baseName = name;
	int index = 0;
	QRegExp regexp;
	if ((regexp = QRegExp("(.*) копия (\\d+)")).exactMatch(name) && regexp.capturedTexts()[2].toInt() >= 2)
	{
		baseName = regexp.capturedTexts()[1];
		index = regexp.capturedTexts()[2].toInt();
	}
	else if ((regexp = QRegExp("(.*) копия")).exactMatch(name))
	{
		baseName = regexp.capturedTexts()[1];
		index = 1;
	}

	// подбираем индекс для нового имени
	QString newName = name;
	for (int i = index; mRootLayer->findGameObjectByName(newName) != NULL; ++i)
		newName = baseName + " копия" + (i > 1 ? " " + QString::number(i) : "");
	return newName;
}

int Scene::generateDuplicateObjectID()
{
	return mObjectIndex++;
}

int Scene::getNumGuides(bool horz) const
{
	const QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	return guides.size();
}

qreal Scene::getGuide(bool horz, int index) const
{
	const QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	return guides[index];
}

void Scene::setGuide(bool horz, int index, qreal coord)
{
	QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	guides[index] = coord;
}

int Scene::findGuide(bool horz, qreal coord, qreal &distance) const
{
	// ищем ближайшую направляющую к заданной координате
	const QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	int index = -1;
	for (int i = 0; i < guides.size(); ++i)
	{
		qreal diff = qAbs(guides[i] - coord);
		if (diff < distance)
		{
			distance = diff;
			index = i;
		}
	}
	return index;
}

int Scene::addGuide(bool horz, qreal coord)
{
	QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	guides.push_back(coord);
	return guides.size() - 1;
}

void Scene::removeGuide(bool horz, int index)
{
	QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	guides.removeAt(index);
}

void Scene::onUndoStackIndexChanged(int index)
{
	// восстанавливаем ранее сохраненное состояние сцены и посылаем сигнал об изменении текущей команды в стеке отмен
	if (mCommandIndex != index)
	{
		mCommandIndex = index;
		QUndoCommand *command = mCommandIndex > 0 ? const_cast<QUndoCommand *>(mUndoStack->command(mCommandIndex - 1)) : mInitialState;
		dynamic_cast<UndoCommand *>(command)->restore();
		emit undoCommandChanged();
	}
}

bool Scene::load(QDataStream &stream)
{
	// сохраняем текущий корневой слой для отложенного удаления при выходе из функции, чтобы минимизировать загрузку/выгрузку ресурсов
	QScopedPointer<BaseLayer> oldRootLayer(mRootLayer);

	// пересоздаем корневой слой
	mRootLayer = new LayerGroup();

	// загружаем слои
	QString type;
	stream >> type;
	if (stream.status() != QDataStream::Ok || type != "LayerGroup" || !mRootLayer->load(stream) || mRootLayer->getNumChildLayers() == 0)
		return false;

	// устанавливаем активный слой
	QList<int> indices;
	stream >> indices;
	if (stream.status() != QDataStream::Ok || indices.empty())
		return false;

	mActiveLayer = mRootLayer;
	foreach (int index, indices)
	{
		if (index < 0 || index >= mActiveLayer->getNumChildLayers())
			return false;
		mActiveLayer = mActiveLayer->getChildLayer(index);
	}

	// загружаем счетчики для генерации имен
	stream >> mObjectIndex >> mLayerIndex >> mLayerGroupIndex >> mSpriteIndex >> mLabelIndex;

	// загружаем направляющие
	stream >> mHorzGuides >> mVertGuides;

	return stream.status() == QDataStream::Ok;
}

bool Scene::save(QDataStream &stream)
{
	// сохраняем слои
	mRootLayer->save(stream);

	// сохраняем последовательность индексов, ведущую к активному слою
	QList<int> indices;
	for (BaseLayer *layer = mActiveLayer; layer != mRootLayer; layer = layer->getParentLayer())
		indices.push_front(layer->getParentLayer()->indexOfChildLayer(layer));
	stream << indices;

	// сохраняем счетчики для генерации имен
	stream << mObjectIndex << mLayerIndex << mLayerGroupIndex << mSpriteIndex << mLabelIndex;

	// сохраняем направляющие
	stream << mHorzGuides << mVertGuides;

	return stream.status() == QDataStream::Ok;
}

Scene::UndoCommand::UndoCommand(const QString &text, Scene *scene)
: QUndoCommand(text), mScene(scene)
{
	QDataStream stream(&mData, QIODevice::WriteOnly);
	mScene->save(stream);
}

void Scene::UndoCommand::restore()
{
	QDataStream stream(&mData, QIODevice::ReadOnly);
	mScene->load(stream);
}
