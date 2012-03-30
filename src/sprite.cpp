#include "pch.h"
#include "sprite.h"
#include "layer.h"
#include "lua_script.h"
#include "project.h"
#include "texture_manager.h"
#include "utils.h"

Sprite::Sprite()
{
}

Sprite::Sprite(const QString &name, int id, const QPointF &pos, const QString &fileName, Layer *parent)
: GameObject(name, id, parent), mFileName(fileName), mColor(Qt::white)
{
	// загружаем текстуру спрайта
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);

	// задаем начальную позицию и размер спрайта
	if (!mTexture.isNull())
	{
		mSize = mTexture->getSize();
		mPosition = QPointF(qFloor(pos.x() - mSize.width() / 2.0), qFloor(pos.y() - mSize.height() / 2.0));
	}

	// инициализируем локализованные свойства
	QString language = Project::getSingleton().getDefaultLanguage();
	mPositionXMap[language] = mPosition.x();
	mPositionYMap[language] = mPosition.y();
	mWidthMap[language] = mSize.width();
	mHeightMap[language] = mSize.height();

	mFileNameMap[language] = mFileName;
	mTextureMap[language] = mTexture;

	// обновляем текущую трансформацию
	updateTransform();
}

Sprite::~Sprite()
{
	// устанавливаем текущий контекст OpenGL для корректного удаления текстуры спрайта
	TextureManager::getSingleton().makeCurrent();
}

QString Sprite::getFileName() const
{
	return mFileName;
}

void Sprite::setFileName(const QString &fileName)
{
	// устанавливаем новое имя файла и загружаем текстуру
	mFileName = fileName;
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);

	// записываем локализованное имя файла и текстуру для текущего языка
	QString language = Project::getSingleton().getCurrentLanguage();
	mFileNameMap[language] = mFileName;
	mTextureMap[language] = mTexture;
}

QColor Sprite::getColor() const
{
	return mColor;
}

void Sprite::setColor(const QColor &color)
{
	mColor = color;
}

bool Sprite::load(QDataStream &stream)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(stream))
		return false;

	// загружаем данные спрайта
	stream >> mFileNameMap >> mColor;
	if (stream.status() != QDataStream::Ok)
		return false;

	// загружаем локализованные текстуры
	mTextureMap.clear();
	for (StringMap::const_iterator it = mFileNameMap.begin(); it != mFileNameMap.end(); ++it)
		mTextureMap[it.key()] = TextureManager::getSingleton().loadTexture(*it);

	// устанавливаем текущий язык
	setCurrentLanguage(Project::getSingleton().getCurrentLanguage());
	return true;
}

bool Sprite::save(QDataStream &stream)
{
	// сохраняем тип объекта
	stream << QString("Sprite");
	if (stream.status() != QDataStream::Ok)
		return false;

	// сохраняем общие данные игрового объекта
	if (!GameObject::save(stream))
		return false;

	// сохраняем данные спрайта
	stream << mFileNameMap << mColor;
	return stream.status() == QDataStream::Ok;
}

bool Sprite::load(LuaScript &script)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(script))
		return false;

	// загружаем данные спрайта
	int color;
	if (!readStringMap(script, "fileName", mFileNameMap) || !script.getInt("color", color))
		return false;
	mColor = QColor::fromRgba(color);

	// загружаем локализованные текстуры
	mTextureMap.clear();
	for (StringMap::const_iterator it = mFileNameMap.begin(); it != mFileNameMap.end(); ++it)
		mTextureMap[it.key()] = TextureManager::getSingleton().loadTexture(*it);

	// устанавливаем текущий язык
	setCurrentLanguage(Project::getSingleton().getCurrentLanguage());
	return true;
}

bool Sprite::save(QTextStream &stream, int indent)
{
	// делаем отступ и записываем тип объекта
	stream << QString(indent, '\t') << "{type = \"Sprite\", ";

	// сохраняем общие данные игрового объекта
	if (!GameObject::save(stream, indent))
		return false;

	// сохраняем свойства спрайта
	stream << ", fileName = ";
	writeStringMap(stream, mFileNameMap);
	stream << ", color = 0x" << hex << mColor.rgba() << dec << "}";
	return stream.status() == QTextStream::Ok;
}

void Sprite::setCurrentLanguage(const QString &language)
{
	// устанавливаем язык для игрового объекта
	GameObject::setCurrentLanguage(language);

	// устанавливаем новые значения для имени файла и текстуры
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
	mFileName = mFileNameMap[mFileNameMap.contains(language) ? language : defaultLanguage];
	mTexture = mTextureMap[mTextureMap.contains(language) ? language : defaultLanguage];
}

GameObject *Sprite::duplicate(Layer *parent) const
{
	// создаем свою копию и добавляем ее к родительскому слою
	Sprite *sprite = new Sprite(*this);
	if (parent != NULL)
		parent->insertGameObject(0, sprite);
	return sprite;
}

QStringList Sprite::getMissedFiles() const
{
	// возвращаем имена незагруженных текстур
	QStringList missedFiles;
	for (StringMap::const_iterator it = mFileNameMap.begin(); it != mFileNameMap.end(); ++it)
		if (mTextureMap[it.key()] == TextureManager::getSingleton().getDefaultTexture())
			missedFiles.push_back(*it);
	return missedFiles;
}

bool Sprite::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем старую текстуру спрайта на новую
	bool changed = false;
	for (StringMap::const_iterator it = mFileNameMap.begin(); it != mFileNameMap.end(); ++it)
		if (*it == fileName)
		{
			mTextureMap[it.key()] = texture;
			if (it.key() == Project::getSingleton().getCurrentLanguage())
				mTexture = texture;
			changed = true;
		}
	return changed;
}

void Sprite::draw()
{
	// сохраняем матрицу трансформации
	glPushMatrix();

	// задаем новую трансформацию и цвет
	glTranslated(mPosition.x(), mPosition.y(), 0.0);
	glRotated(mRotationAngle, 0.0, 0.0, 1.0);
	glScaled(mSize.width() / mTexture->getWidth(), mSize.height() / mTexture->getHeight(), 1.0);
	glColor4d(mColor.redF(), mColor.greenF(), mColor.blueF(), mColor.alphaF());

	// рисуем квад с текстурой спрайта
	mTexture->draw();

	// восстанавливаем матрицу трансформации
	glPopMatrix();
}
