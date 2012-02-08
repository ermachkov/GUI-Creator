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

	void setPrimaryGLWidget(QGLWidget *primaryGLWidget);

	// проверка совпадения деревьев - игрового и GUI
	void DEBUG_TREES();

signals:

	// Сигнал об изменении локации
	void locationChanged();

	// Сигнал об изменении слоя
	void layerChanged();

public slots:

	// обработчик - добавления группы слоев
	void onAddLayerGroup();

	// обработчик - добавления слоя
	void onAddLayer();

	// обработчик - удаления слоев и групп слоев
	void onDelete();

	// обработчик - произошли изменения в окне редактирования
	void onEditorWindowLayerChanged(Location *location, BaseLayer *layer);

protected:

	// обработчик начала перетаскивания
	virtual void dragEnterEvent(QDragEnterEvent *event);

	// обработчик бросания при перетаскивании
	virtual void dropEvent(QDropEvent *event);

	// обработчик нажатия клавиши клавиатуры
	virtual void keyPressEvent(QKeyEvent *event);

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

private:

	// Максимальное количество вложенных слоев
	static const int MAX_NESTED_LAYERS = 16;

	// Делегат для запрещения редактирования названия колонок
	class EditorDelegate : public QItemDelegate
	{

	public:

		EditorDelegate(QObject *parent = NULL);

		virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;

		virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
	};

	// добавление группы
	QTreeWidgetItem *insertNewLayerGroup(int index, QTreeWidgetItem *parent);

	// добавление слоя
	void insertNewLayer(int index, QTreeWidgetItem *parent);

	// создание части дерева QTreeWidgetItem по дереву BaseLayer
	void createTreeItems(QTreeWidgetItem *item, BaseLayer *base);

	// поиск GUI слоя по BaseLayer * слоя
	QTreeWidgetItem *findByBaseLayer(BaseLayer *layer);

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

	// иконки для группы в окне слоёв
	QIcon mLayerGroupIcon;

	// иконки для видимости
	QIcon mLayerVisibleIcons[3];

	// иконки для блокировки слоя
	QIcon mLayerLockedIcons[3];

	// указатель на игровую локацию
	Location *mCurrentLocation;

	//В общем придумал - в MainWindow передаешь mPrimaryGLWidget в конструктор
	//LayersWindow, а тот в свою очередь передает его в LayersTreeWidget
	//через метод setGLWidget, который его собственно и сохраняет.

	QGLWidget *mPrimaryGLWidget;

	static const int WIDTH_THUMBNAIL = 32;
	static const int HEIGHT_THUMBNAIL = 32;

	QGLFramebufferObject *mFrameBuffer;
};

#endif // LAYERS_TREE_WIDGET_H
