#ifndef EDITOR_WINDOW_H
#define EDITOR_WINDOW_H

class BaseLayer;
class GameObject;
class Location;
class Texture;

// Класс окна редактора
class EditorWindow : public QGLWidget
{
	Q_OBJECT

public:

	// Конструктор
	EditorWindow(QWidget *parent, QGLWidget *shareWidget, const QString &fileName);

	// Загружает локацию из файла
	bool load(const QString &fileName);

	// Сохраняет локацию в файл
	bool save(const QString &fileName);

	// Возвращает указатель на редактируемую игровую локацию
	Location *getLocation() const;

	// Возвращает имя файла локации
	QString getFileName() const;

	// Возвращает флаг изменения локации
	bool isChanged() const;

	// Устанавливает флаг изменения локации
	void setChanged(bool changed);

	// Возвращает флаг сохранения локации
	bool isSaved() const;

	// Возвращает текущий масштаб
	qreal getZoom() const;

	// Устанавливает новый масштаб с центровкой по середине окна
	void setZoom(qreal zoom);

	// Возвращает true, если в окне редактирования есть выделенные объекты
	bool hasSelectedObjects() const;

	// Вырезает выделенные объекты в буфер обмена
	void cut();

	// Копирует выделенные объекты в буфер обмена
	void copy();

	// Вставляет объекты из буфера обмена
	void paste();

	// Удаляет выделенные объекты
	void clear();

	// Выделяет все объекты
	void selectAll();

	// Снимает выделение со всех объектов
	void deselectAll();

	// Перемещает выделенные объекты на передний план в слое
	void bringToFront();

	// Перемещает выделенные объекты на задний план в слое
	void sendToBack();

	// Перемещает выделенные объекты вверх
	void moveUp();

	// Перемещает выделенные объекты вниз
	void moveDown();

	// Возвращает список отсутствующих текстур в локации
	QStringList getMissedTextures() const;

	// Заменяет текстуру в локации
	void changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

signals:

	// Сигнал об изменении масштаба
	void zoomChanged(const QString &zoomStr);

	// Сигнал об изменении выделения
	void selectionChanged(bool selected);

	// Сигнал об изменении локации
	void locationChanged(bool changed);

	// Сигнал об изменении координат мышки
	void mouseMoved(const QPointF &pos);

	// Сигнал об изменении слоя
	void layerChanged(BaseLayer *layer);

protected:

	// Вызывается при создании контекста OpenGL
	virtual void initializeGL();

	// Вызывается при перерисовке окна
	virtual void paintGL();

	// Вызывается при изменении размеров окнаa
	virtual void resizeGL(int width, int height);

	// Обработчик нажатия кнопки мышки
	virtual void mousePressEvent(QMouseEvent *event);

	// Обработчик отпускания кнопки мышки
	virtual void mouseReleaseEvent(QMouseEvent *event);

	// Обработчик перемещения мышки
	virtual void mouseMoveEvent(QMouseEvent *event);

	// Обработчик вращения колесика мышки
	virtual void wheelEvent(QWheelEvent *event);

	// Обработчик нажатия клавиши клавиатуры
	virtual void keyPressEvent(QKeyEvent *event);

	// Вызывается при начале перетаскивания объекта над окном редактора
	virtual void dragEnterEvent(QDragEnterEvent *event);

	// Вызывается при бросании объекта в окно редактора
	virtual void dropEvent(QDropEvent *event);

private:

	static const int MARKER_SIZE = 9;   // Размер маркера выделения в пикселях
	static const int CENTER_SIZE = 13;  // Размер перекрестья центра вращения в пикселях

	// Состояния редактирования
	enum EditorState
	{
		STATE_IDLE,         // Ничего не делаем
		STATE_SELECT,       // Выделение объектов рамкой
		STATE_MOVE,         // Перемещение объектов
		STATE_RESIZE,       // Изменение размера объектов
		STATE_ROTATE,       // Поворот объектов
		STATE_MOVE_CENTER   // Перемещение центра вращения
	};

	// Маркеры выделения
	enum SelectionMarker
	{
		MARKER_NONE,
		MARKER_TOP_LEFT,
		MARKER_CENTER_LEFT,
		MARKER_BOTTOM_LEFT,
		MARKER_BOTTOM_CENTER,
		MARKER_BOTTOM_RIGHT,
		MARKER_CENTER_RIGHT,
		MARKER_TOP_RIGHT,
		MARKER_TOP_CENTER
	};

	// Конвертирует мировые координаты в оконные
	QPointF worldToWindow(const QPointF &pt) const;

	// Конвертирует оконные координаты в мировые
	QPointF windowToWorld(const QPointF &pt) const;

	// Конвертирует мировые координаты прямоугольника в оконные с округлением
	QRectF worldRectToWindow(const QRectF &rect) const;

	// Определяет вектор перемещения
	QPointF getTranslationVector(const QPointF &start, const QPointF &end, bool shift) const;

	// Выделяет игровой объект
	void selectGameObject(GameObject *object);

	// Выделяет игровые объекты
	void selectGameObjects(const QList<GameObject *> &objects);

	// Обновляет курсор мыши
	void updateMouseCursor(const QPointF &pos);

	// Возвращает список родительских слоев выделенных объектов
	QSet<BaseLayer *> getParentLayers() const;

	// Выдает сигналы об изменении локации и заданных слоев
	void emitLayerChangedSignals(const QSet<BaseLayer *> &layers);

	// Ищет маркер выделения
	SelectionMarker findSelectionMarker(const QPointF &pos) const;

	// Рисует маркер выделения
	void drawSelectionMarker(qreal x, qreal y);

	Location            *mLocation;         // Игровая локация
	QString             mFileName;          // Имя файла локации
	bool                mChanged;           // Флаг изменения локации
	bool                mSaved;             // Флаг сохранения локации
	EditorState         mEditorState;       // Текущее состояние редактирования

	QPointF             mCameraPos;         // Текущее положение камеры
	qreal               mZoom;              // Текущий зум
	QPoint              mFirstPos;          // Начальные оконные координаты мыши
	QPoint              mLastPos;           // Последние оконные координаты мыши
	bool                mEnableEdit;        // Флаг разрешения редактирования

	QList<GameObject *> mSelectedObjects;   // Список выделенных объектов, отсортированный по глубине
	QList<QPointF>      mOriginalPositions; // Список исходных координат объектов
	QList<QPointF>      mOriginalScales;    // Список исходных масштабов объектов
	QList<qreal>        mOriginalAngles;    // Список исходных углов поворота объектов
	bool                mHasRotatedObjects; // Флаг наличия повернутых объектов в текущем выделении
	bool                mFirstTimeSelected; // Флаг выделения объекта в первый раз
	SelectionMarker     mSelectionMarker;   // Текущий маркер выделения
	QRectF              mOriginalRect;      // Исходный ограничивающий прямоугольник выделенных объектов
	QRectF              mSnappedRect;       // Текущий ограничивающий прямоугольник, привязанный к сетке
	QRectF              mMoveRect;          // Ограничивающий прямоугольник, используемый при перемещении объектов
	QRectF              mSelectionRect;     // Рамка выделения

	bool                mRotationMode;      // Текущий режим поворота
	QPointF             mOriginalCenter;    // Исходный центр вращения
	QPointF             mSnappedCenter;     // Текущий центр вращения, привязанный к сетке
	QVector2D           mRotationVector;    // Начальный вектор вращения
	QCursor             mRotateCursor;      // Курсор поворота
};

#endif // EDITOR_WINDOW_H
