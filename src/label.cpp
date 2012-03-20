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
: GameObject(name, id, parent), mText(name), mFileName(fileName), mFontSize(size), mHorzAlignment(HORZ_ALIGN_LEFT),
  mVertAlignment(VERT_ALIGN_TOP), mLineSpacing(1.0), mColor(Qt::white)
{
	// загружаем шрифт
	mFont = FontManager::getSingleton().loadFont(mFileName, mFontSize);

	// задаем начальную позицию и размер надписи
	if (!mFont.isNull())
	{
		mSize = QSizeF(qCeil(mFont->Advance(Utils::toStdWString(mText + " ").c_str())), qCeil(mFont->LineHeight()));
		mPosition = QPointF(qFloor(pos.x() - mSize.width() / 2.0), qFloor(pos.y() - mSize.height() / 2.0));
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

Label::HorzAlignment Label::getHorzAlignment() const
{
	return mHorzAlignment;
}

void Label::setHorzAlignment(HorzAlignment alignment)
{
	mHorzAlignment = alignment;
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
	int horzAlignment, vertAlignment;
	stream >> mText >> mFileName >> mFontSize >> horzAlignment >> vertAlignment >> mLineSpacing >> mColor;
	if (stream.status() != QDataStream::Ok)
		return false;
	mHorzAlignment = static_cast<HorzAlignment>(horzAlignment);
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
	stream << mText << mFileName << mFontSize << mHorzAlignment << mVertAlignment << mLineSpacing << mColor;
	return stream.status() == QDataStream::Ok;
}

bool Label::load(LuaScript &script)
{
	// загружаем общие данные игрового объекта
	if (!GameObject::load(script))
		return false;

	// загружаем данные надписи
	int horzAlignment, vertAlignment, color;
	if (!script.getString("text", mText) || !script.getString("fileName", mFileName) || !script.getInt("size", mFontSize)
		|| !script.getInt("horzAlignment", horzAlignment) || !script.getInt("vertAlignment", vertAlignment)
		|| !script.getReal("lineSpacing", mLineSpacing) || !script.getInt("color", color))
		return false;
	mHorzAlignment = static_cast<HorzAlignment>(horzAlignment);
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
	stream << ", text = \"" << Utils::insertBackslashes(mText) << "\", fileName = \"" << Utils::insertBackslashes(mFileName)
		<< "\", size = " << mFontSize << ", horzAlignment = " << mHorzAlignment << ", vertAlignment = " << mVertAlignment
		<< ", lineSpacing = " << mLineSpacing << ", color = 0x" << hex << mColor.rgba() << dec << "}";
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
	glTranslated(mPosition.x(), mPosition.y(), 0.0);
	glRotated(mRotationAngle, 0.0, 0.0, 1.0);
	glColor4d(mColor.redF(), mColor.greenF(), mColor.blueF(), mColor.alphaF());

	// разбиваем текст на строки
	QStringList lines;
	qreal spaceWidth = mFont->Advance(L" ");
	foreach (const QString &line, mText.split("\n"))
	{
		// разбиваем строку на слова
		qreal width = 0.0, oldWidth = 0.0;
		QString str;
		foreach (const QString &word, line.split(" ", QString::SkipEmptyParts))
		{
			// определяем ширину текущего слова в пикселях
			qreal wordWidth = mFont->Advance(Utils::toStdWString(word).c_str());
			width += wordWidth;

			// переносим строку, если ее суммарная ширина превышает ширину текстового прямоугольника
			if (width >= qAbs(mSize.width()) && oldWidth > 0.0)
			{
				lines.push_back(str);
				str.clear();
				width = wordWidth;
			}

			// склеиваем слова в строке, разделяя их пробелами
			str += word + " ";
			width += spaceWidth;
			oldWidth = width;
		}

		// добавляем последнюю строку в список строк
		lines.push_back(str);
	}

	// определяем начальную координату по оси Y с учетом вертикального выравнивания
	qreal y = 0.0;
	qreal height = lines.size() * mFont->LineHeight() * mLineSpacing;
	if (mVertAlignment == VERT_ALIGN_CENTER)
		y = (qAbs(mSize.height()) - height) / 2.0;
	else if (mVertAlignment == VERT_ALIGN_BOTTOM)
		y = qAbs(mSize.height()) - height;

	// выводим текст построчно
	foreach (const QString &line, lines)
	{
		// определяем координату текущей строки по оси X с учетом горизонтального выравнивания
		qreal x = 0.0;
		qreal width = mFont->Advance(Utils::toStdWString(line).c_str());
		if (mHorzAlignment == HORZ_ALIGN_CENTER)
			x = (qAbs(mSize.width()) - width) / 2.0;
		else if (mHorzAlignment == HORZ_ALIGN_RIGHT)
			x = qAbs(mSize.width()) - width;

		// отрисовываем текущую строку
		glPushMatrix();
		glTranslated(qCeil(x) * scale.x(), (qCeil(y) + qRound(mFont->LineHeight() / 1.25)) * scale.y(), 0.0);
		glScaled(scale.x(), -scale.y(), 1.0);
		mFont->Render(Utils::toStdWString(line).c_str());
		glPopMatrix();

		// переходим на следующую строку текста
		y += mFont->LineHeight() * mLineSpacing;
	}

	// восстанавливаем матрицу трансформации
	glPopMatrix();
}
