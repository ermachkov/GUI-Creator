#include "pch.h"
#include "utils.h"

const qreal Utils::PI = 3.14159265358979323846;

qreal Utils::radToDeg(qreal angle)
{
	return angle * 180.0 / PI;
}

qreal Utils::degToRad(qreal angle)
{
	return angle * PI / 180.0;
}

QString Utils::addTrailingSlash(const QString &path)
{
	return !path.endsWith('/') && !path.endsWith('\\') ? path + '/' : path;
}

bool Utils::isFileNameValid(const QString &fileName)
{
	QRegExp regexp("([a-z][a-z\\d_]*/)*[a-z][a-z\\d_]*\\.[a-z]+");
	return regexp.exactMatch(fileName);
}

QString Utils::convertToCamelCase(const QString &fileName)
{
	// заменяем все вхождения символа '_' и строчной буквы на одну заглавную букву
	QString name = QFileInfo(fileName).baseName();
	QString str;
	for (int i = 0; i < name.size(); ++i)
		str += name[i] == '_' && i < name.size() - 1 ? name[++i].toUpper() : name[i];
	return str;
}

QString Utils::convertToPascalCase(const QString &fileName)
{
	// конвертируем строку в стиль Camel и меняем первую букву на заглавную
	QString str = convertToCamelCase(fileName);
	if (!str.isEmpty())
		str[0] = str[0].toUpper();
	return str;
}

QString Utils::insertBackslashes(const QString &text)
{
	// вставляем обратный слэш перед одинарными и двойными кавычками, а также перед самим обратным слэшем
	QString str;
	foreach (QChar ch, text)
	{
		if (ch == '\'' || ch == '\"' || ch == '\\')
			str += '\\';
		str += ch;
	}
	return str;
}

QString Utils::stripBackslashes(const QString &text)
{
	// удаляем обратные слэши перед одинарными и двойными кавычками, а также перед самими обратными слэшами
	QString str;
	for (int i = 0; i < text.size(); ++i)
		str += text[i] == '\\' && i < text.size() - 1 && (text[i + 1] == '\'' || text[i + 1] == '\"' || text[i + 1] == '\\') ? text[++i] : text[i];
	return str;
}

void Utils::drawLine(const QPointF &pt1, const QPointF &pt2, const QColor &color)
{
	glBegin(GL_LINES);
	glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());
	glVertex2d(pt1.x(), pt1.y());
	glVertex2d(pt2.x(), pt2.y());
	glEnd();
}

void Utils::fillRect(const QRectF &rect, const QColor &color)
{
	glBegin(GL_TRIANGLE_STRIP);
	glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());
	glVertex2d(rect.left(), rect.bottom());
	glVertex2d(rect.right(), rect.bottom());
	glVertex2d(rect.left(), rect.top());
	glVertex2d(rect.right(), rect.top());
	glEnd();
}

void Utils::drawRect(const QRectF &rect, const QColor &color)
{
	glBegin(GL_LINE_LOOP);
	glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());
	glVertex2d(rect.left(), rect.top());
	glVertex2d(rect.left(), rect.bottom());
	glVertex2d(rect.right(), rect.bottom());
	glVertex2d(rect.right(), rect.top());
	glEnd();
}

void Utils::fillCircle(const QPointF &center, qreal radius, const QColor &color)
{
	glBegin(GL_TRIANGLE_FAN);
	glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());
	glVertex2d(center.x(), center.y());
	qreal step = qAtan(0.5 / radius);
	for (qreal angle = 0.0; angle <= 2.0 * PI + step; angle += step)
		glVertex2d(center.x() + radius * qCos(angle), center.y() - radius * qSin(angle));
	glEnd();
}

void Utils::drawCircle(const QPointF &center, qreal radius, const QColor &color)
{
	glBegin(GL_LINE_LOOP);
	glColor4d(color.redF(), color.greenF(), color.blueF(), color.alphaF());
	qreal step = qAtan(0.5 / radius);
	for (qreal angle = 0.0; angle <= 2.0 * PI + step; angle += step)
		glVertex2d(center.x() + radius * qCos(angle), center.y() - radius * qSin(angle));
	glEnd();
}
