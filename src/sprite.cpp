#include "pch.h"
#include "sprite.h"
#include "layer.h"
#include "lua_script.h"
#include "project.h"
#include "texture_manager.h"
#include "utils.h"

Sprite::Sprite()
: mSizeLocked(true)
{
}

Sprite::Sprite(const QString &name, int id, const QPointF &pos, const QString &fileName, Layer *parent)
: GameObject(name, id, parent), mFileName(fileName), mSizeLocked(true), mColor(Qt::white)
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
	mTextureWidthMap[language] = mTexture->getWidth();
	mTextureHeightMap[language] = mTexture->getHeight();

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
	Q_ASSERT(isLocalized());
	mFileName = fileName;
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);

	// записываем локализованное имя файла и текстуру для текущего языка
	QString language = Project::getSingleton().getCurrentLanguage();
	mFileNameMap[language] = mFileName;
	mTextureMap[language] = mTexture;

	// пересчитываем размер спрайта, если загружена валидная текстура
	if (!mTexture->isDefault())
	{
		QPointF scale(mSize.width() / mTextureWidthMap[language], mSize.height() / mTextureHeightMap[language]);
		setSize(QSizeF(mTexture->getWidth() * scale.x(), mTexture->getHeight() * scale.y()));
		mTextureWidthMap[language] = mTexture->getWidth();
		mTextureHeightMap[language] = mTexture->getHeight();
	}
}

QSizeF Sprite::getTextureSize() const
{
	QString language = isLocalized() ? Project::getSingleton().getCurrentLanguage() : Project::getSingleton().getDefaultLanguage();
	return QSizeF(mTextureWidthMap[language], mTextureHeightMap[language]);
}

bool Sprite::isSizeLocked() const
{
	return mSizeLocked;
}

void Sprite::setSizeLocked(bool locked)
{
	mSizeLocked = locked;
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
	stream >> mFileNameMap >> mTextureWidthMap >> mTextureHeightMap >> mColor;
	if (stream.status() != QDataStream::Ok)
		return false;

	// загружаем локализованные текстуры
	loadTextures();
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
	stream << mFileNameMap << mTextureWidthMap << mTextureHeightMap << mColor;
	return stream.status() == QDataStream::Ok;
}

bool Sprite::load(LuaScript &script)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(script))
		return false;

	// загружаем данные спрайта
	unsigned int color;
	if (!readStringMap(script, "fileName", mFileNameMap) || !readRealMap(script, "textureWidth", mTextureWidthMap)
		|| !readRealMap(script, "textureHeight", mTextureHeightMap) || !script.getUnsignedInt("color", color))
		return false;
	mColor = QColor::fromRgba(color);

	// загружаем локализованные текстуры
	loadTextures();
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
	stream << ", textureWidth = ";
	writeRealMap(stream, mTextureWidthMap);
	stream << ", textureHeight = ";
	writeRealMap(stream, mTextureHeightMap);
	stream << ", color = 0x" << hex << mColor.rgba() << dec << "}";
	return stream.status() == QTextStream::Ok;
}

void Sprite::setCurrentLanguage(const QString &language)
{
	// устанавливаем язык для игрового объекта
	GameObject::setCurrentLanguage(language);

	// устанавливаем новые значения для имени файла и текстуры
	QString currentLanguage = isLocalized() ? language : Project::getSingleton().getDefaultLanguage();
	mFileName = mFileNameMap[currentLanguage];
	mTexture = mTextureMap[currentLanguage];
}

bool Sprite::isLocalized() const
{
	QString language = Project::getSingleton().getCurrentLanguage();
	return GameObject::isLocalized() && mFileNameMap.contains(language) && mTextureMap.contains(language)
		&& mTextureWidthMap.contains(language) && mTextureHeightMap.contains(language);
}

void Sprite::setLocalized(bool localized)
{
	QString currentLanguage = Project::getSingleton().getCurrentLanguage();
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
	Q_ASSERT(currentLanguage != defaultLanguage);

	// устанавливаем локализацию для игрового объекта
	GameObject::setLocalized(localized);

	if (localized)
	{
		// копируем значения локализованных свойств из дефолтного языка
		mFileNameMap[currentLanguage] = mFileNameMap[defaultLanguage];
		mTextureMap[currentLanguage] = mTextureMap[defaultLanguage];
		mTextureWidthMap[currentLanguage] = mTextureWidthMap[defaultLanguage];
		mTextureHeightMap[currentLanguage] = mTextureHeightMap[defaultLanguage];
	}
	else
	{
		// удаляем значения локализованных свойств
		mFileNameMap.remove(currentLanguage);
		mTextureMap.remove(currentLanguage);
		mTextureWidthMap.remove(currentLanguage);
		mTextureHeightMap.remove(currentLanguage);
	}

	// устанавливаем текущий язык
	setCurrentLanguage(Project::getSingleton().getCurrentLanguage());
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
		if (mTextureMap[it.key()]->isDefault())
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
			// сохраняем новую текстуру
			const QString &language = it.key();
			mTextureMap[language] = texture;
			changed = true;

			// заменяем текстуру для текущего языка
			if (language == Project::getSingleton().getCurrentLanguage())
				mTexture = texture;

			// пересчитываем размер спрайта, если загружена валидная текстура
			if (!texture->isDefault())
			{
				// пересчитываем размеры спрайта
				QPointF scale(mWidthMap[language] / mTextureWidthMap[language], mHeightMap[language] / mTextureHeightMap[language]);
				mWidthMap[language] = texture->getWidth() * scale.x();
				mHeightMap[language] = texture->getHeight() * scale.y();
				mTextureWidthMap[language] = texture->getWidth();
				mTextureHeightMap[language] = texture->getHeight();

				// устанавливаем новый размер для текущего языка
				if (language == Project::getSingleton().getCurrentLanguage())
					setSize(QSizeF(mWidthMap[language], mHeightMap[language]));
			}
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

void Sprite::loadTextures()
{
	// загружаем локализованные текстуры
	mTextureMap.clear();
	for (StringMap::const_iterator it = mFileNameMap.begin(); it != mFileNameMap.end(); ++it)
	{
		// загружаем текстуру
		const QString &language = it.key();
		QSharedPointer<Texture> texture = TextureManager::getSingleton().loadTexture(*it);
		mTextureMap[language] = texture;

		// пересчитываем размер спрайта, если загружена валидная текстура
		if (!texture->isDefault())
		{
			// пересчитываем размеры спрайта
			QPointF scale(mWidthMap[language] / mTextureWidthMap[language], mHeightMap[language] / mTextureHeightMap[language]);
			mWidthMap[language] = texture->getWidth() * scale.x();
			mHeightMap[language] = texture->getHeight() * scale.y();
			mTextureWidthMap[language] = texture->getWidth();
			mTextureHeightMap[language] = texture->getHeight();
		}
	}

	// устанавливаем текущий язык
	setCurrentLanguage(Project::getSingleton().getCurrentLanguage());
}
