#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

class Layer;
class LuaScript;
class Texture;

// Базовый класс игрового объекта
class GameObject
{
public:

	// Конструктор
	GameObject();

	// Конструктор
	GameObject(const QString &name, int id, Layer *parent = NULL);

	// Конструктор копирования
	GameObject(const GameObject &object);

	// Деструктор
	virtual ~GameObject();

	// Возвращает имя объекта
	QString getName() const;

	// Устанавливает имя объекта
	void setName(const QString &name);

	// Возвращает идентификатор объекта
	int getObjectID() const;

	// Устанавливает идентификатор объекта
	void setObjectID(int id);

	// Возвращает координаты объекта
	QPointF getPosition() const;

	// Устанавливает координаты объекта
	void setPosition(const QPointF &position);

	// Возвращает размеры объекта
	QSizeF getSize() const;

	// Устанавливает размеры объекта
	void setSize(const QSizeF &size);

	// Возвращает угол поворота объекта в градусах
	qreal getRotationAngle() const;

	// Устанавливает угол поворота объекта в градусах
	void setRotationAngle(qreal angle);

	// Возвращает центр вращения в мировых координатах
	QPointF getRotationCenter() const;

	// Устанавливает центр вращения в мировых координатах
	void setRotationCenter(const QPointF &center);

	// Возвращает родительский слой
	Layer *getParentLayer() const;

	// Устанавливает родительский слой
	void setParentLayer(Layer *parent);

	// Возвращает ограничивающий прямоугольник объекта
	QRectF getBoundingRect() const;

	// Возвращает true если точка лежит внутри объекта
	bool isContainPoint(const QPointF &pt) const;

	// Возвращает true если объект целиком лежит внутри заданного прямоугольника
	bool isContainedInRect(const QRectF &rect) const;

	// Привязывает координату по оси X к игровому объекту
	void snapXCoord(qreal x, qreal y1, qreal y2, qreal &snappedX, qreal &distance, QLineF &line) const;

	// Привязывает координату по оси Y к игровому объекту
	void snapYCoord(qreal y, qreal x1, qreal x2, qreal &snappedY, qreal &distance, QLineF &line) const;

	// Загружает объект из бинарного потока
	virtual bool load(QDataStream &stream);

	// Сохраняет объект в бинарный поток
	virtual bool save(QDataStream &stream);

	// Загружает объект из Lua скрипта
	virtual bool load(LuaScript &script);

	// Сохраняет объект в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Устанавливает текущий язык для объекта
	virtual void setCurrentLanguage(const QString &language);

	// Загружает переводы из Lua скрипта
	virtual void loadTranslations(LuaScript *script);

	// Дублирует объект
	virtual GameObject *duplicate(Layer *parent = NULL) const = 0;

	// Возвращает список отсутствующих файлов в объекте
	virtual QStringList getMissedFiles() const = 0;

	// Заменяет текстуру в объекте
	virtual bool changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture) = 0;

	// Отрисовывает объект
	virtual void draw() = 0;

protected:

	// Типы для локализованных данных
	typedef QMap<QString, qreal> RealMap;
	typedef QMap<QString, QString> StringMap;

	// Конвертирует координаты из локальных в мировые
	QPointF localToWorld(const QPointF &pt) const;

	// Конвертирует координаты из мировых в локальные
	QPointF worldToLocal(const QPointF &pt) const;

	// Обновляет текущую трансформацию объекта
	void updateTransform();

	// Читает список локализованных вещественных чисел
	bool readRealMap(LuaScript &script, const QString &name, RealMap &map);

	// Записывает список локализованных вещественных чисел
	void writeRealMap(QTextStream &stream, RealMap &map);

	// Читает список локализованных строк
	bool readStringMap(LuaScript &script, const QString &name, StringMap &map);

	// Записывает список локализованных строк
	void writeStringMap(QTextStream &stream, StringMap &map);

	QString     mName;              // Имя (текстовое описание) объекта
	int         mObjectID;          // Идентификатор объекта
	QPointF     mPosition;          // Мировые координаты объекта
	QSizeF      mSize;              // Размеры объекта в пикселях
	qreal       mRotationAngle;     // Угол поворота объекта в градусах по часовой стрелке
	QPointF     mRotationCenter;    // Центр вращения в нормализованных локальных координатах
	Layer       *mParentLayer;      // Указатель на родительский слой

	RealMap     mPositionXMap;      // Список локализованных координат по оси X
	RealMap     mPositionYMap;      // Список локализованных координат по оси Y
	RealMap     mWidthMap;          // Список локализованных ширин
	RealMap     mHeightMap;         // Список локализованных высот

	QTransform  mTransform;         // Результирующая матрица трансформации
	QTransform  mInvTransform;      // Обратная матрица трансформации
	QPointF     mVertices[4];       // Список вершин объекта в мировых координатах
	QRectF      mBoundingRect;      // Ограничивающий прямоугольник объекта
};

#endif // GAME_OBJECT_H
