#ifndef LAYERS_TREE_WIDGET_H
#define LAYERS_TREE_WIDGET_H

#include "base_layer.h"

class Location;

// Дерево слоёв
class LayersTreeWidget : public QTreeWidget
{
	Q_OBJECT

public:

	// номера столбцов
	enum Column
	{
		DATA_COLUMN = 0,
		VISIBLE_COLUMN = 1,
		LOCK_COLUMN = 2
	};

	// Конструктор
	LayersTreeWidget(QWidget *parent = NULL);

	// Деструктор
	virtual ~LayersTreeWidget();

	// пересоздание содержимого окна слоев
	void setCurrentLocation(Location *location);

	Location *getCurrentLocation() const;

	void setPrimaryGLWidget(QGLWidget *primaryGLWidget);

	// проверка совпадения деревьев - игрового и GUI
	void DEBUG_TREES();

signals:

	// Сигнал об изменении локации
	void locationChanged(const QString &commandName);

	// Сигнал об изменении слоя
	void layerChanged();

public slots:

	// обработчик - добавления группы слоев
	void onAddLayerGroup();

	// обработчик - добавления слоя
	void onAddLayer();

	// обработчик - удаления слоев и групп слоев
	void onDelete();

	// Обработчик изменения слоя в окне редактирования
	void onEditorWindowLayerChanged(Location *location, BaseLayer *layer);

	// Обработчик изменения слоя в окне свойств
	void onPropertyWindowLayerChanged(BaseLayer *layer);

protected:

	// обработчик начала перетаскивания
	virtual void dragEnterEvent(QDragEnterEvent *event);

	// обработчик бросания при перетаскивании
	virtual void dropEvent(QDropEvent *event);

	// обработчик нажатия клавиши клавиатуры
	virtual void keyPressEvent(QKeyEvent *event);

	// обработчик вызова контекстного меню
	virtual void contextMenuEvent(QContextMenuEvent *event);

private slots:

	// обработчик смены текущего активного элемента
	void onCurrentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

	// обработчик щелчка по элементу дерева слоёв
	void onItemClicked(QTreeWidgetItem *item, int column);

	// обработчик изменения
	void onItemChanged(QTreeWidgetItem *item, int column);

	// обработчик сворачивания-разворачивания
	void onItemExpanded(QTreeWidgetItem *item);
	void onItemCollapsed(QTreeWidgetItem *item);

	// обработчик выбора пункта контекстного ингю
	void onContextMenuTriggered(QAction *action);

private:

	// ширина и высота иконки предпросмотра
	static const int WIDTH_THUMBNAIL = 32;
	static const int HEIGHT_THUMBNAIL = 32;

	// Делегат для запрещения редактирования названия колонок
	class EditorDelegate : public QItemDelegate
	{
	public:

		EditorDelegate(QObject *parent = NULL);

		// запрещение редактирования такста некоторых колонок
		virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

		// для запрещения введения пустой строки при изменении имя элемента
		virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;

		// для переопределения вертикального расстояния между элементами
		virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	};

	// добавление группы
	QTreeWidgetItem *insertNewLayerGroup(int index, QTreeWidgetItem *parent);

	// добавление слоя
	void insertNewLayer(int index, QTreeWidgetItem *parent);

	// создание части дерева QTreeWidgetItem по дереву BaseLayer
	void createTreeItems(QTreeWidgetItem *item, BaseLayer *base);

	// генерация имени копии по исходному имени
	QString generateCopyName(const QString &name);

	// генерация новых id и objectName для созданных копий объектов
	void adjustObjectNamesAndIds(BaseLayer *baseLayer);

	// поиск GUI слоя по BaseLayer * слоя
	QTreeWidgetItem *findItemByBaseLayer(BaseLayer *layer, QTreeWidgetItem *item = NULL) const;

	// определение максимальной глубины(вложенности) потомков относительно элемента
	int getMaxDepthChilds(QTreeWidgetItem *item, int currentDepth = 0) const;

	// определение глубины(вложенности) (вверх) элемента
	int getLayerDepth(QTreeWidgetItem *item) const;

	// поиск невидимой группы в родителях
	bool hasInvisibleParent(QTreeWidgetItem *item) const;

	// поиск заблокированной группы в родителях
	bool hasLockedParent(QTreeWidgetItem *item) const;

	// смена видимости всем полувидимым потомкам
	void changeChildrenPartiallyVisibleState(QTreeWidgetItem *item, bool state);

	// смена блокировки всем полузаблокированным потомкам
	void changeChildrenPartiallyLockState(QTreeWidgetItem *item, bool state);

	// "аксессоры"
	void setBaseLayer(QTreeWidgetItem *item, BaseLayer *baseLayer);
	BaseLayer *getBaseLayer(QTreeWidgetItem *item) const;
	void setVisibleState(QTreeWidgetItem *item, BaseLayer::VisibleState state);
	BaseLayer::VisibleState getVisibleState(QTreeWidgetItem *item) const;
	void setLockState(QTreeWidgetItem *item, BaseLayer::LockState state);
	BaseLayer::LockState getLockState(QTreeWidgetItem *item) const;

	// определение - группа или слой
	bool isLayer(QTreeWidgetItem *item) const;
	bool isLayerGroup(QTreeWidgetItem *item) const;

	// формирование иконки предпросмотра
	QIcon createLayerIcon(BaseLayer *baseLayer);

	QIcon                  mLayerGroupIcon;       // иконки для группы в окне слоёв
	QIcon                  mLayerVisibleIcons[3]; // иконки для видимости
	QIcon                  mLayerLockedIcons[3];  // иконки для блокировки слоя

	Location              *mCurrentLocation;      // указатель на игровую локацию

	QGLWidget             *mPrimaryGLWidget;      // указатель на OpenGL виджет
	QGLFramebufferObject  *mFrameBuffer;          // фреймбуфер для отрисовки иконки предпросмотра спрайта

	QMenu                 *mContextMenu;          // контекстное меню
	QAction               *mDuplicateAction;      // - пункт меню "Дублировать"
	QAction               *mNewGroupAction;       // - пункт меню "Новая группа"
	QAction               *mNewLayerAction;       // - пункт меню "Новый слой"
	QAction               *mDeleteAction;         // - пункт меню "Удалить"
};

#endif // LAYERS_TREE_WIDGET_H
