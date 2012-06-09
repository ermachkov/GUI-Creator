#ifndef SCENE_H
#define SCENE_H

class BaseLayer;
class GameObject;
class Layer;

// Класс сцены
class Scene : public QObject
{
	Q_OBJECT

public:

	// Конструктор
	Scene(QObject *parent);

	// Деструктор
	virtual ~Scene();

	// Загружает сцену из файла
	bool load(const QString &fileName);

	// Сохраняет сцену в файл
	bool save(const QString &fileName);

	// Загружает файл переводов
	bool loadTranslationFile(const QString &fileName);

	// Возвращает корневой слой
	BaseLayer *getRootLayer() const;

	// Возвращает текущий активный слой
	BaseLayer *getActiveLayer() const;

	// Устанавливает текущий активный слой
	void setActiveLayer(BaseLayer *layer);

	// Возвращает доступный активный слой для добавления/вставки объектов
	Layer *getAvailableLayer() const;

	// Возвращает стек отмен сцены
	QUndoStack *getUndoStack() const;

	// Возвращает флаг неизмененной сцены
	bool isClean() const;

	// Устанавливает флаг неизмененной сцены
	void setClean();

	// Помещает в стек отмен новую команду
	void pushCommand(const QString &commandName);

	// Проверяет, можно ли отменить текущую команду
	bool canUndo() const;

	// Проверяет, можно ли повторить текущую команду
	bool canRedo() const;

	// Отменяет текущую команду
	void undo();

	// Повторяет текущую команду
	void redo();

	// Создает новый слой
	BaseLayer *createLayer(BaseLayer *parent = NULL, int index = 0);

	// Создает новую группу слоев
	BaseLayer *createLayerGroup(BaseLayer *parent = NULL, int index = 0);

	// Создает новый спрайт
	GameObject *createSprite(const QPointF &pos, const QString &fileName);

	// Создает новую надпись
	GameObject *createLabel(const QPointF &pos, const QString &fileName, int size);

	// Создает и загружает игровой объект из бинарного потока
	GameObject *loadGameObject(QDataStream &stream);

	// Генерирует имя для копии объекта
	QString generateDuplicateName(const QString &name) const;

	// Генерирует идентификатор для копии объекта
	int generateDuplicateObjectID();

	// Возвращает количество направляющих
	int getNumGuides(bool horz) const;

	// Возвращает координату направляющей
	qreal getGuide(bool horz, int index) const;

	// Устанавливает координату направляющей
	void setGuide(bool horz, int index, qreal coord);

	// Ищет направляющую по координате
	int findGuide(bool horz, qreal coord, qreal &distance) const;

	// Добавляет новую направляющую
	int addGuide(bool horz, qreal coord);

	// Удаляет направляющую
	void removeGuide(bool horz, int index);

signals:

	// Сигнал об изменении текущей команды в стеке отмен
	void undoCommandChanged();

private slots:

	// Обработчик изменения индекса текущей команды в стеке отмен
	void onUndoStackIndexChanged(int index);

private:

	// Класс команды отмены
	class UndoCommand : public QUndoCommand
	{
	public:

		// Конструктор
		UndoCommand(const QString &text, Scene *scene);

		// Восстанавливает ранее сохраненное состояние сцены
		void restore();

	private:

		Scene       *mScene;        // Указатель на объект сцены
		QByteArray  mData;          // Сохраненное состояние сцены
	};

	// Загружает сцену из бинарного потока
	bool load(QDataStream &stream);

	// Сохраняет сцену в бинарный поток
	bool save(QDataStream &stream);

	BaseLayer       *mRootLayer;        // Корневой слой
	BaseLayer       *mActiveLayer;      // Текущий активный слой

	QUndoStack      *mUndoStack;        // Текущий стек отмен
	int             mCommandIndex;      // Индекс текущей команды в стеке отмен
	UndoCommand     *mInitialState;     // Начальное состояние сцены

	int             mObjectIndex;       // Текущий индекс для генерации уникальных идентификаторов объектов
	int             mLayerIndex;        // Текущий индекс для генерации имен слоев
	int             mLayerGroupIndex;   // Текущий индекс для генерации имен групп слоев
	int             mSpriteIndex;       // Текущий индекс для генерации имен спрайтов
	int             mLabelIndex;        // Текущий индекс для генерации имен надписей

	QList<qreal>    mHorzGuides;        // Горизонтальные направляющие
	QList<qreal>    mVertGuides;        // Вертикальные направляющие
};

#endif // SCENE_H
