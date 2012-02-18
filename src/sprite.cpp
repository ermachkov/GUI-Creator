#include "pch.h"
#include "sprite.h"
#include "layer.h"
#include "lua_script.h"
#include "texture_manager.h"
#include "utils.h"

Sprite::Sprite()
{
}

Sprite::Sprite(const QString &name, const QPointF &pos, const QString &fileName, Layer *parent)
: GameObject(name, parent), mFileName(fileName), mColor(Qt::white)
{
	// загружаем текстуру спрайта
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);

	// задаем начальную позицию и размер спрайта
	if (!mTexture.isNull())
	{
		mSize = mTexture->getSize();
		mPosition = QPointF(qFloor(pos.x() - mSize.width() / 2.0), qFloor(pos.y() - mSize.height() / 2.0));
		mRotationCenter = QPointF(mSize.width() / 2.0, mSize.height() / 2.0);
	}

	// обновляем текущую трансформацию
	updateTransform();
}

Sprite::Sprite(const Sprite &sprite)
: GameObject(sprite), mFileName(sprite.mFileName), mTexture(sprite.mTexture), mColor(sprite.mColor)
{
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
	mFileName = fileName;
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);
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
	stream >> mFileName >> mColor;
	if (stream.status() != QDataStream::Ok)
		return false;

	// загружаем текстуру спрайта
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);
	return !mTexture.isNull();
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
	stream << mFileName << mColor;
	return stream.status() == QDataStream::Ok;
}

bool Sprite::load(LuaScript &script)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(script))
		return false;

	// загружаем данные спрайта
	int color;
	if (!script.getString("texture", mFileName) || !script.getInt("color", color))
		return false;
	mColor = QColor::fromRgba(color);

	// загружаем текстуру спрайта
	mTexture = TextureManager::getSingleton().loadTexture(mFileName);
	return !mTexture.isNull();
}

bool Sprite::save(QTextStream &stream, int indent)
{
	// делаем отступ и записываем тип объекта
	stream << QString(indent, '\t') << "{type = \"Sprite\", ";

	// сохраняем общие данные игрового объекта
	if (!GameObject::save(stream, indent))
		return false;

	// сохраняем свойства спрайта
	stream << ", texture = \"" << Utils::insertBackslashes(mFileName) << "\", color = 0x" << hex << mColor.rgba() << dec << "}";
	return stream.status() == QTextStream::Ok;
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
	// возвращаем имя текстуры, если она дефолтная
	return mTexture == TextureManager::getSingleton().getDefaultTexture() ? QStringList(mFileName) : QStringList();
}

bool Sprite::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	// заменяем текстуру спрайта, если она совпадает с текущей
	if (mFileName == fileName)
	{
		mTexture = texture;
		return true;
	}

	return false;
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
