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

	// Возвращает список выделенных объектов
	QList<GameObject *> getSelectedObjects() const;

	// Возвращает координаты центра вращения
	QPointF getRotationCenter() const;

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

	// Возвращает список отсутствующих файлов в локации
	QStringList getMissedFiles() const;

	// Заменяет текстуру в локации
	void changeTexture(const QString &fileName, const QSharedPointer<Texture> &texture);

	// Обновляет прямоугольник выделения и центр вращения
	void updateSelection(const QPointF &rotationCenter);

signals:

	// Сигнал об изменении масштаба
	void zoomChanged(const QString &zoomStr);

	// Сигнал об изменении выделения
	void selectionChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

	// Сигнал об изменении свойств выделенных объектов
	void objectsChanged(const QList<GameObject *> &objects, const QPointF &rotationCenter);

	// Сигнал об изменении локации
	void locationChanged(bool changed);

	// Сигнал об изменении координат мышки
	void mouseMoved(const QPointF &pos);

	// Сигнал об изменении слоя
	void layerChanged(Location *location, BaseLayer *layer);

protected:

	// Вызывается при перерисовке окна
	virtual void paintEvent(QPaintEvent *event);

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

	static const int MIN_GRID_SPACING = 8;      // Минимальный шаг сетки в пикселях
	static const int GRID_SPACING_COEFF = 4;    // Множитель динамического изменения шага сетки
	static const int MARKER_SIZE = 9;           // Размер маркера выделения в пикселях
	static const int CENTER_SIZE = 13;          // Размер перекрестья центра вращения в пикселях
	static const int RULER_SIZE = 20;           // Размер линейки в пикселях
	static const int DIVISION_SIZE = 6;         // Длина одного деления линейки в пикселях
	static const int SNAP_DISTANCE = 4;         // Расстояние привязки к сетке/направляющим в пикселях
	static const int GUIDE_DISTANCE = 3;        // Расстояние до направляющей для начала перетаскивания

	// Состояния редактирования
	enum EditorState
	{
		STATE_IDLE,         // Ничего не делаем
		STATE_SELECT,       // Выделение объектов рамкой
		STATE_MOVE,         // Перемещение объектов
		STATE_RESIZE,       // Изменение размера объектов
		STATE_ROTATE,       // Поворот объектов
		STATE_MOVE_CENTER,  // Перемещение центра вращения
		STATE_HORZ_GUIDE,   // Перемещение горизонтальной направляющей
		STATE_VERT_GUIDE    // Перемещение вертикальной направляющей
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

	// Привязывает координату X к сетке/направляющим
	qreal snapXCoord(qreal x, qreal y1, qreal y2, bool excludeSelection, QLineF *linePtr = NULL, qreal *distancePtr = NULL);

	// Привязывает координату Y к сетке/направляющим
	qreal snapYCoord(qreal y, qreal x1, qreal x2, bool excludeSelection, QLineF *linePtr = NULL, qreal *distancePtr = NULL);

	// Определяет вектор перемещения игрового объекта
	QPointF calcTranslation(const QPointF &direction, bool shift, QVector2D &axis);

	// Определяет масштаб при изменении размеров игрового объекта
	QPointF calcScale(const QPointF &pos, const QPointF &pivot, qreal sx, qreal sy, bool keepProportions);

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
	void drawSelectionMarker(qreal x, qreal y, QPainter &painter);

	// Рисует умную направляющую
	void drawSmartGuide(const QLineF &line, QPainter &painter);

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
	QList<QSizeF>       mOriginalSizes;     // Список исходных размеров объектов
	QList<qreal>        mOriginalAngles;    // Список исходных углов поворота объектов
	bool                mHasRotatedObjects; // Флаг наличия повернутых объектов в текущем выделении
	bool                mFirstTimeSelected; // Флаг выделения объекта в первый раз
	SelectionMarker     mSelectionMarker;   // Текущий маркер выделения
	QRectF              mOriginalRect;      // Исходный ограничивающий прямоугольник выделенных объектов
	QRectF              mSnappedRect;       // Текущий ограничивающий прямоугольник, привязанный к сетке
	QRectF              mSelectionRect;     // Рамка выделения
	qreal               mGuideIndex;        // Индекс текущей направляющей
	QLineF              mHorzSnapLine;      // Горизонтальная линия привязки
	QLineF              mVertSnapLine;      // Вертикальная линия привязки

	bool                mRotationMode;      // Текущий режим поворота
	QPointF             mOriginalCenter;    // Исходный центр вращения
	QPointF             mSnappedCenter;     // Текущий центр вращения, привязанный к сетке
	QVector2D           mRotationVector;    // Начальный вектор вращения
	QCursor             mRotateCursor;      // Курсор поворота
};

#endif // EDITOR_WINDOW_H
