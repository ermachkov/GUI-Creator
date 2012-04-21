#include "pch.h"
#include "location.h"
#include "label.h"
#include "layer.h"
#include "layer_group.h"
#include "lua_script.h"
#include "sprite.h"
#include "utils.h"

Location::Location(QObject *parent)
: QObject(parent), mObjectIndex(1), mLayerIndex(1), mLayerGroupIndex(1), mSpriteIndex(1), mLabelIndex(1)
{
	// создаем корневой слой
	mRootLayer = new LayerGroup("");

	// добавляем в него новый слой по умолчанию и делаем его активным
	mActiveLayer = createLayer(mRootLayer);
}

Location::~Location()
{
	delete mRootLayer;
}

bool Location::load(const QString &fileName)
{
	// загружаем Lua скрипт
	LuaScript script;
	if (!script.load(fileName))
		return false;

	// пересоздаем корневой слой
	delete mRootLayer;
	mRootLayer = new LayerGroup();

	// читаем корневую таблицу
	if (!script.pushTable(Utils::toCamelCase(QFileInfo(fileName).baseName())))
		return false;

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

	return true;
}

bool Location::save(const QString &fileName)
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

	// записываем корневую таблицу
	stream << endl << "-- Location root table. Do not declare global variables with the same name!" << endl;
	stream << Utils::toCamelCase(QFileInfo(fileName).baseName()) << " =" << endl;
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

bool Location::loadTranslationFile(const QString &fileName)
{
	// загружаем Lua скрипт
	LuaScript script;
	if (!script.load(fileName))
	{
		mRootLayer->loadTranslations(NULL);
		return false;
	}

	// читаем корневую таблицу
	if (!script.pushTable(Utils::toCamelCase(QFileInfo(fileName).baseName()) + "_translations"))
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

BaseLayer *Location::getRootLayer() const
{
	return mRootLayer;
}

BaseLayer *Location::getActiveLayer() const
{
	return mActiveLayer;
}

void Location::setActiveLayer(BaseLayer *layer)
{
	mActiveLayer = layer;
}

Layer *Location::getAvailableLayer() const
{
	// возвращаем указатель на активный слой, если он не папка, видим и не заблокирован
	Layer *layer = dynamic_cast<Layer *>(mActiveLayer);
	return layer != NULL && layer->getVisibleState() == BaseLayer::LAYER_VISIBLE && layer->getLockState() == BaseLayer::LAYER_UNLOCKED ? layer : NULL;
}

BaseLayer *Location::createLayer(BaseLayer *parent, int index)
{
	return new Layer(QString("Слой %1").arg(mLayerIndex++), parent, index);
}

BaseLayer *Location::createLayerGroup(BaseLayer *parent, int index)
{
	return new LayerGroup(QString("Группа %1").arg(mLayerGroupIndex++), parent, index);
}

GameObject *Location::createSprite(const QPointF &pos, const QString &fileName)
{
	return new Sprite(QString("Спрайт %1").arg(mSpriteIndex++), mObjectIndex++, pos, fileName, getAvailableLayer());
}

GameObject *Location::createLabel(const QPointF &pos, const QString &fileName, int size)
{
	return new Label(QString("Текст %1").arg(mLabelIndex++), mObjectIndex++, pos, fileName, size, getAvailableLayer());
}

GameObject *Location::loadGameObject(QDataStream &stream)
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

QString Location::generateDuplicateName(const QString &name) const
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

int Location::generateDuplicateObjectID()
{
	return mObjectIndex++;
}

int Location::getNumGuides(bool horz) const
{
	const QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	return guides.size();
}

qreal Location::getGuide(bool horz, int index) const
{
	const QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	return guides[index];
}

void Location::setGuide(bool horz, int index, qreal coord)
{
	QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	guides[index] = coord;
}

int Location::findGuide(bool horz, qreal coord, qreal &distance) const
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

int Location::addGuide(bool horz, qreal coord)
{
	QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	guides.push_back(coord);
	return guides.size() - 1;
}

void Location::removeGuide(bool horz, int index)
{
	QList<qreal> &guides = horz ? mHorzGuides : mVertGuides;
	guides.removeAt(index);
}

bool Location::load(QDataStream &stream)
{
	// пересоздаем корневой слой
	delete mRootLayer;
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

bool Location::save(QDataStream &stream)
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
