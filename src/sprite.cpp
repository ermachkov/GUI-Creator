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
	mFileName = fileName;
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);

	// записываем локализованное имя файла и текстуру для текущего языка
	QString language = Project::getSingleton().getCurrentLanguage();
	mFileNameMap[language] = mFileName;
	mTextureMap[language] = mTexture;

	// пересчитываем размер спрайта, если загружена валидная текстура
	if (mTexture != TextureManager::getSingleton().getDefaultTexture())
	{
		QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
		qreal textureWidth = mTextureWidthMap[mTextureWidthMap.contains(language) ? language : defaultLanguage];
		qreal textureHeight = mTextureHeightMap[mTextureHeightMap.contains(language) ? language : defaultLanguage];
		setSize(QSizeF(mTexture->getWidth() * mSize.width() / textureWidth, mTexture->getHeight() * mSize.height() / textureHeight));
		mTextureWidthMap[language] = mTexture->getWidth();
		mTextureHeightMap[language] = mTexture->getHeight();
	}
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
	int color;
	if (!readStringMap(script, "fileName", mFileNameMap) || !readRealMap(script, "textureWidth", mTextureWidthMap)
		|| !readRealMap(script, "textureHeight", mTextureHeightMap) || !script.getInt("color", color))
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
			// сохраняем новую текстуру
			const QString &language = it.key();
			mTextureMap[language] = texture;
			changed = true;

			// заменяем текстуру для текущего языка
			if (language == Project::getSingleton().getCurrentLanguage())
				mTexture = texture;

			// пересчитываем размер спрайта, если загружена валидная текстура
			if (texture != TextureManager::getSingleton().getDefaultTexture())
			{
				// пересчитываем ширину спрайта
				QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
				qreal width = mWidthMap[mWidthMap.contains(language) ? language : defaultLanguage];
				qreal textureWidth = mTextureWidthMap[mTextureWidthMap.contains(language) ? language : defaultLanguage];
				mWidthMap[language] = texture->getWidth() * width / textureWidth;
				mTextureWidthMap[language] = texture->getWidth();

				// пересчитываем высоту спрайта
				qreal height = mHeightMap[mHeightMap.contains(language) ? language : defaultLanguage];
				qreal textureHeight = mTextureHeightMap[mTextureHeightMap.contains(language) ? language : defaultLanguage];
				mHeightMap[language] = texture->getHeight() * height / textureHeight;
				mTextureHeightMap[language] = texture->getHeight();

				// устанавливаем новый размер для текущего языка
				if (language == Project::getSingleton().getCurrentLanguage())
				{
					mSize = QSizeF(mWidthMap[language], mHeightMap[language]);
					updateTransform();
				}
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
		mTextureMap[it.key()] = TextureManager::getSingleton().loadTexture(*it);

	// пересчитываем ширину спрайтов
	QStringList languages(mTextureMap.keys() + mWidthMap.keys());
	languages.removeDuplicates();
	QString defaultLanguage = Project::getSingleton().getDefaultLanguage();
	foreach (const QString &language, languages)
	{
		QSharedPointer<Texture> &texture = mTextureMap[mTextureMap.contains(language) ? language : defaultLanguage];
		if (texture != TextureManager::getSingleton().getDefaultTexture())
		{
			qreal width = mWidthMap[mWidthMap.contains(language) ? language : defaultLanguage];
			qreal textureWidth = mTextureWidthMap[mTextureWidthMap.contains(language) ? language : defaultLanguage];
			mWidthMap[language] = texture->getWidth() * width / textureWidth;
			mTextureWidthMap[language] = texture->getWidth();
		}
	}

	// пересчитываем высоту спрайтов
	languages = mTextureMap.keys() + mHeightMap.keys();
	languages.removeDuplicates();
	foreach (const QString &language, languages)
	{
		QSharedPointer<Texture> &texture = mTextureMap[mTextureMap.contains(language) ? language : defaultLanguage];
		if (texture != TextureManager::getSingleton().getDefaultTexture())
		{
			qreal height = mHeightMap[mHeightMap.contains(language) ? language : defaultLanguage];
			qreal textureHeight = mTextureHeightMap[mTextureHeightMap.contains(language) ? language : defaultLanguage];
			mHeightMap[language] = texture->getHeight() * height / textureHeight;
			mTextureHeightMap[language] = texture->getHeight();
		}
	}

	// устанавливаем текущий язык
	setCurrentLanguage(Project::getSingleton().getCurrentLanguage());
}
