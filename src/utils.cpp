#include "pch.h"
#include "utils.h"

const qreal Utils::PI = 3.14159265358979323846;
const qreal Utils::EPS = 0.0001;

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

bool Utils::fileExists(const QString &path)
{
	// проверяем, что файл существует с точностью до регистра символов
	if (QFile::exists(path))
	{
		QFileInfo fileInfo(path);
		return fileInfo.dir().entryList().contains(fileInfo.fileName());
	}

	return false;
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

std::wstring Utils::toStdWString(const QString &str)
{
#ifdef _MSC_VER
	return std::wstring(reinterpret_cast<const wchar_t *>(str.utf16()));
#else
	return str.toStdWString();
#endif
}
