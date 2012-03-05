#include "pch.h"
#include "label.h"
#include "font_manager.h"
#include "layer.h"
#include "lua_script.h"
#include "utils.h"

Label::Label()
{
}

Label::Label(const QString &name, int id, const QPointF &pos, const QString &fileName, int size, Layer *parent)
: GameObject(name, id, parent), mText(name), mFileName(fileName), mFontSize(size), mAlignment(FTGL::ALIGN_LEFT),
  mVertAlignment(VERT_ALIGN_TOP), mLineSpacing(1.0), mColor(Qt::white)
{
	// загружаем шрифт
	mFont = FontManager::getSingleton().loadFont(mFileName, mFontSize);

	// задаем начальную позицию и размер надписи
	if (!mFont.isNull())
	{
		mSize = QSizeF(qCeil(mFont->Advance(Utils::toStdWString(mText).c_str())), qCeil(mFont->LineHeight()));
		mPosition = QPointF(qFloor(pos.x() - mSize.width() / 2.0), qFloor(pos.y() - mSize.height() / 2.0));
		mRotationCenter = QPointF(mSize.width() / 2.0, mSize.height() / 2.0);
	}

	// обновляем текущую трансформацию
	updateTransform();
}

Label::~Label()
{
	// устанавливаем текущий контекст OpenGL для корректного удаления текстуры шрифта
	FontManager::getSingleton().makeCurrent();
}

QString Label::getText() const
{
	return mText;
}

void Label::setText(const QString &text)
{
	mText = text;
}

QString Label::getFileName() const
{
	return mFileName;
}

void Label::setFileName(const QString &fileName)
{
	mFileName = fileName;
	mFont = FontManager::getSingleton().loadFont(mFileName, mFontSize);
}

int Label::getFontSize() const
{
	return mFontSize;
}

void Label::setFontSize(int size)
{
	mFontSize = size;
	mFont = FontManager::getSingleton().loadFont(mFileName, mFontSize);
}

FTGL::TextAlignment Label::getAlignment() const
{
	return mAlignment;
}

void Label::setAlignment(FTGL::TextAlignment alignment)
{
	mAlignment = alignment;
}

Label::VertAlignment Label::getVertAlignment() const
{
	return mVertAlignment;
}

void Label::setVertAlignment(VertAlignment alignment)
{
	mVertAlignment = alignment;
}

qreal Label::getLineSpacing() const
{
	return mLineSpacing;
}

void Label::setLineSpacing(qreal lineSpacing)
{
	mLineSpacing = lineSpacing;
}

QColor Label::getColor() const
{
	return mColor;
}

void Label::setColor(const QColor &color)
{
	mColor = color;
}

bool Label::load(QDataStream &stream)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(stream))
		return false;

	// загружаем данные надписи
	int alignment, vertAlignment;
	stream >> mText >> mFileName >> mFontSize >> alignment >> vertAlignment >> mLineSpacing >> mColor;
	if (stream.status() != QDataStream::Ok)
		return false;
	mAlignment = static_cast<FTGL::TextAlignment>(alignment);
	mVertAlignment = static_cast<VertAlignment>(vertAlignment);

	// загружаем шрифт надписи
	mFont = FontManager::getSingleton().loadFont(mFileName, mFontSize);
	return !mFont.isNull();
}

bool Label::save(QDataStream &stream)
{
	// сохраняем тип объекта
	stream << QString("Label");
	if (stream.status() != QDataStream::Ok)
		return false;

	// сохраняем общие данные игрового объекта
	if (!GameObject::save(stream))
		return false;

	// сохраняем данные надписи
	stream << mText << mFileName << mFontSize << mAlignment << mVertAlignment << mLineSpacing << mColor;
	return stream.status() == QDataStream::Ok;
}

bool Label::load(LuaScript &script)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(script))
		return false;

	// загружаем данные надписи
	int alignment, vertAlignment, color;
	if (!script.getString("text", mText) || !script.getString("font", mFileName) || !script.getInt("size", mFontSize)
		|| !script.getInt("alignment", alignment) || !script.getInt("vert_alignment", vertAlignment)
		|| !script.getReal("line_spacing", mLineSpacing) || !script.getInt("color", color))
		return false;
	mAlignment = static_cast<FTGL::TextAlignment>(alignment);
	mVertAlignment = static_cast<VertAlignment>(vertAlignment);
	mColor = QColor::fromRgba(color);

	// загружаем шрифт надписи
	mFont = FontManager::getSingleton().loadFont(mFileName, mFontSize);
	return !mFont.isNull();
}

bool Label::save(QTextStream &stream, int indent)
{
	// делаем отступ и записываем тип объекта
	stream << QString(indent, '\t') << "{type = \"Label\", ";

	// сохраняем общие данные игрового объекта
	if (!GameObject::save(stream, indent))
		return false;

	// сохраняем свойства надписи
	stream << ", text = \"" << Utils::insertBackslashes(mText) << "\", font = \"" << Utils::insertBackslashes(mFileName)
		<< "\", size = " << mFontSize << ", alignment = " << mAlignment << ", vert_alignment = " << mVertAlignment
		<< ", line_spacing = " << mLineSpacing << ", color = 0x" << hex << mColor.rgba() << dec << "}";
	return stream.status() == QTextStream::Ok;
}

GameObject *Label::duplicate(Layer *parent) const
{
	// создаем свою копию и добавляем ее к родительскому слою
	Label *label = new Label(*this);
	if (parent != NULL)
		parent->insertGameObject(0, label);
	return label;
}

QStringList Label::getMissedFiles() const
{
	// возвращаем имя шрифта, если он дефолтный
	return mFont == FontManager::getSingleton().getDefaultFont() ? QStringList(mFileName) : QStringList();
}

bool Label::changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture)
{
	return false;
}

void Label::draw()
{
	// сохраняем матрицу трансформации
	glPushMatrix();

	// задаем новую трансформацию и цвет
	QPointF scale(mSize.width() >= 0.0 ? 1.0 : -1.0, mSize.height() >= 0.0 ? 1.0 : -1.0);
	glTranslated(mPosition.x(), mPosition.y() + qRound(mFont->LineHeight() / 1.25) * scale.y(), 0.0);
	glRotated(mRotationAngle, 0.0, 0.0, 1.0);
	glScaled(scale.x(), -scale.y(), 1.0);
	glColor4d(mColor.redF(), mColor.greenF(), mColor.blueF(), mColor.alphaF());

	// рисуем текст
	FTSimpleLayout layout;
	layout.SetFont(mFont.data());
	layout.SetLineLength(qAbs(mSize.width()));
	layout.SetAlignment(mAlignment);
	layout.SetLineSpacing(mLineSpacing);
	layout.Render(Utils::toStdWString(mText).c_str());

	// восстанавливаем матрицу трансформации
	glPopMatrix();
}
