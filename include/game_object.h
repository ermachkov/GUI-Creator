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
	GameObject(const QString &name, const QPointF &position, const QSizeF &size, Layer *parent = NULL);

	// Конструктор копирования
	GameObject(const GameObject &object);

	// Деструктор
	virtual ~GameObject();

	// Возвращает имя объекта
	QString getName() const;

	// Устанавливает имя объекта
	void setName(const QString &name);

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

	// Загружает объект из бинарного потока
	virtual bool load(QDataStream &stream);

	// Сохраняет объект в бинарный поток
	virtual bool save(QDataStream &stream);

	// Загружает объект из Lua скрипта
	virtual bool load(LuaScript &script);

	// Сохраняет объект в текстовый поток
	virtual bool save(QTextStream &stream, int indent);

	// Дублирует объект
	virtual GameObject *duplicate(Layer *parent = NULL) const = 0;

	// Возвращает список отсутствующих файлов в объекте
	virtual QStringList getMissedFiles() const = 0;

	// Заменяет текстуру в объекте
	virtual bool changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture) = 0;

	// Отрисовывает объект
	virtual void draw() = 0;

protected:

	// Конвертирует координаты из локальных в мировые
	QPointF localToWorld(const QPointF &pt) const;

	// Конвертирует координаты из мировых в локальные
	QPointF worldToLocal(const QPointF &pt) const;

	// Обновляет текущую трансформацию объекта
	void updateTransform();

	QString     mName;              // Имя (текстовое описание) объекта
	QPointF     mPosition;          // Мировые координаты объекта
	QSizeF      mSize;              // Размеры объекта в пикселях
	qreal       mRotationAngle;     // Угол поворота объекта в градусах по часовой стрелке
	QPointF     mRotationCenter;    // Центр вращения в локальных координатах
	Layer       *mParentLayer;      // Указатель на родительский слой

	QTransform  mTransform;         // Результирующая матрица трансформации
	QTransform  mInvTransform;      // Обратная матрица трансформации
	QPointF     mVertices[4];       // Список вершин объекта в мировых координатах
	QRectF      mBoundingRect;      // Ограничивающий прямоугольник объекта
};

#endif // GAME_OBJECT_H
