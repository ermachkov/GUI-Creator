#ifndef UTILS_H
#define UTILS_H

// Класс со вспомогательными функциями
class Utils
{
public:

	// Число Пи
	static const qreal PI;

	// Погрешность расчетов
	static const qreal EPS;

	// Возвращает значение функции y = sign(x)
	static qreal sign(qreal x);

	// Переводит угол из радианов в градусы
	static qreal radToDeg(qreal angle);

	// Переводит угол из градусов в радианы
	static qreal degToRad(qreal angle);

	// Дополняет путь прямым слешем
	static QString addTrailingSlash(const QString &path);

	// Проверяет относительный путь к файлу на валидность
	static bool isFileNameValid(const QString &fileName);

	// Конвертирует правильное имя файла в стиль Camel
	static QString convertToCamelCase(const QString &fileName);

	// Конвертирует правильное имя файла в стиль Pascal
	static QString convertToPascalCase(const QString &fileName);

	// Проверяет существование файла с чувствительностью к регистру
	static bool fileExists(const QString &path);

	// Экранирует специальные символы в строке обратными слэшами
	static QString insertBackslashes(const QString &text);

	// Удаляет обратные слэши из строки
	static QString stripBackslashes(const QString &text);

	// Преобразует QString в std::wstring
	static std::wstring toStdWString(const QString &str);
};

#endif // UTILS_H
