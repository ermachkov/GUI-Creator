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

	// Сравнение дробных чисел с необходимой точностью
	static bool isEqual(qreal value1, qreal value2, qreal eps = EPS);

	// Дополняет путь прямым слешем
	static QString addTrailingSlash(const QString &path);

	// Проверяет относительный путь к файлу на валидность
	static bool isFileNameValid(const QString &fileName);

	// Проверяет имя файла на валидность и выдает диалоговое окно в случае ошибки
	static bool isFileNameValid(const QString &fileName, const QString &dir, QWidget *parent);

	// Конвертирует правильное имя файла в стиль Camel
	static QString toCamelCase(const QString &fileName);

	// Конвертирует правильное имя файла в стиль Pascal
	static QString toPascalCase(const QString &fileName);

	// Проверяет существование файла с чувствительностью к регистру
	static bool fileExists(const QString &path);

	// Заключает строку в кавычки и экранирует специальные символы обратными слэшами
	static QString quotify(const QString &text);

	// Преобразует QString в std::wstring
	static std::wstring toStdWString(const QString &str);

	// Записывает шапку текстового Lua-файла
	static void writeFileHeader(QTextStream &stream);
};

#endif // UTILS_H
