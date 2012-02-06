#ifndef UTILS_H
#define UTILS_H

// Класс со вспомогательными функциями
class Utils
{
public:

	// Число Пи
	static const qreal PI;

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

	// Рисует линию с указанными координатами
	static void drawLine(const QPointF &pt1, const QPointF &pt2, const QColor &color);

	// Рисует закрашенный прямоугольник
	static void fillRect(const QRectF &rect, const QColor &color);

	// Рисует прямоугольную рамку
	static void drawRect(const QRectF &rect, const QColor &color);

	// Рисует закрашенный круг
	static void fillCircle(const QPointF &center, qreal radius, const QColor &color);

	// Рисует окружность
	static void drawCircle(const QPointF &center, qreal radius, const QColor &color);
};

#endif
